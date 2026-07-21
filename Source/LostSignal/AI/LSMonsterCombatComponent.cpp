#include "AI/LSMonsterCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Characters/LSPlayerCharacter.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Combat/LSHitboxLibrary.h"
#include "Components/CapsuleComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Data/LSMonsterActionRow.h"
#include "Data/LSMonsterArchetypeRow.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/Effects/LSGE_MonsterBasicDamage.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/Preview/LSSkillPreviewComponent.h"
#include "LostSignal.h"
#include "TimerManager.h"

ULSMonsterCombatComponent::ULSMonsterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DamageEffectClass = ULSGE_MonsterBasicDamage::StaticClass();
}

void ULSMonsterCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	if (OwnerCharacter && OwnerCharacter->HasAuthority())
	{
		OwnerCharacter->GetCapsuleComponent()->OnComponentHit.AddDynamic(this, &ULSMonsterCombatComponent::HandleOwnerCapsuleHit);
	}
}

void ULSMonsterCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CancelActionCharge();

	if (ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner()))
	{
		OwnerCharacter->GetCapsuleComponent()->OnComponentHit.RemoveDynamic(this, &ULSMonsterCombatComponent::HandleOwnerCapsuleHit);
	}

	Super::EndPlay(EndPlayReason);
}

void ULSMonsterCombatComponent::ApplyArchetype(const FLSMonsterArchetypeRow& Row)
{
	bCombatArchetypeApplied = true;
	AlertMoveSpeedMultiplier = FMath::Max(0.0f, Row.Chase_Speed);
	ActionGroup = Row.Action_Group;

	// 진단: 공격 선택에 필요한 설정이 제대로 들어왔는지 초기화 때 1회 덤프.
	UE_LOG(LogLS, Log, TEXT("ApplyArchetype %s: MonsterActionTable=%s, ActionGroup수=%d"),
		*GetNameSafe(GetOwner()), *GetNameSafe(MonsterActionTable), ActionGroup.Num());
	for (const FName& RowName : ActionGroup)
	{
		const FLSMonsterActionRow* ActionRow = FindActionRow(RowName);
		if (!ActionRow)
		{
			UE_LOG(LogLS, Warning, TEXT("  - 액션 row '%s' 를 MonsterActionTable에서 못 찾음(테이블 미할당/row 이름 불일치)."), *RowName.ToString());
			continue;
		}

		UE_LOG(LogLS, Log, TEXT("  - %s: 사거리 %.0f~%.0f, 쿨다운 %.1f"),
			*RowName.ToString(), ActionRow->Action_Range_Min, ActionRow->Action_Range_Max, ActionRow->Action_Cooldown);
	}
}

bool ULSMonsterCombatComponent::RequestAbilityByTag(FGameplayTag AbilityTag) const
{
	if (!bCombatArchetypeApplied)
	{
		UE_LOG(LogLS, Warning, TEXT("%s monster ability request blocked because no DataTable archetype was applied."), *GetNameSafe(GetOwner()));
		return false;
	}

	if (!AbilityTag.IsValid())
	{
		return false;
	}

	const ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	if (ASC->HasMatchingGameplayTag(LSGameplayTags::State_Dead) ||
		ASC->HasMatchingGameplayTag(LSGameplayTags::State_Stunned))
	{
		return false;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(AbilityTag);
	return ASC->TryActivateAbilitiesByTag(AbilityTags);
}

void ULSMonsterCombatComponent::CancelAbilityByTag(FGameplayTag AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return;
	}

	const ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(AbilityTag);
	ASC->CancelAbilities(&AbilityTags);
}

bool ULSMonsterCombatComponent::IsAbilityActiveByTag(FGameplayTag AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return false;
	}

	const ALSCharacterBase* Character = Cast<ALSCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = Character ? Character->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return false;
	}

	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(AbilityTag);
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(AbilityTags, MatchingSpecs, false);
	for (const FGameplayAbilitySpec* Spec : MatchingSpecs)
	{
		if (Spec && Spec->IsActive())
		{
			return true;
		}
	}

	return false;
}

bool ULSMonsterCombatComponent::RequestAction(AActor* Target)
{
	const AActor* OwnerActor = GetOwner();
	if (!bCombatArchetypeApplied || !OwnerActor)
	{
		UE_LOG(LogLS, Warning, TEXT("RequestAction aborted: ArchetypeApplied=%d Owner=%s (몬스터 스탯/액션 그룹 미적용?)"),
			bCombatArchetypeApplied ? 1 : 0, *GetNameSafe(OwnerActor));
		return false;
	}

	const float Distance = Target
		? FVector::Dist2D(OwnerActor->GetActorLocation(), Target->GetActorLocation())
		: 0.0f;

	const FName RowName = SelectActionForDistance(Distance);
	if (RowName.IsNone())
	{
		UE_LOG(LogLS, Log, TEXT("RequestAction: %s 거리 %.0f에 발동 가능한 액션 없음(사거리/쿨다운/액션그룹 확인)."),
			*GetNameSafe(OwnerActor), Distance);
		return false;
	}

	ActiveActionRowName = RowName;
	ActiveTarget = Target;
	bActionDashLandingValid = false; // 새 액션 시작 — 이전 도약 착지 좌표 무효화.
	bActionUsesContactHit = false;

	// 공격 시작 직전 타겟 방향으로 yaw 즉시 스냅 — 연속 공격이 이전 공격 방향을 물려받지 않게 한다.
	// (공격 중 회전 잠금은 이 스냅된 방향을 고정하는 것으로 유지)
	if (Target)
	{
		AActor* MutableOwner = GetOwner();
		FVector ToTarget = Target->GetActorLocation() - MutableOwner->GetActorLocation();
		ToTarget.Z = 0.0f;
		if (!ToTarget.IsNearlyZero())
		{
			FRotator SnapRotation = MutableOwner->GetActorRotation();
			SnapRotation.Yaw = ToTarget.Rotation().Yaw;
			MutableOwner->SetActorRotation(SnapRotation);
		}
	}

	const bool bActivated = RequestAbilityByTag(LSGameplayTags::Ability_MonsterAction);
	UE_LOG(LogLS, Log, TEXT("RequestAction: %s 거리 %.0f -> row=%s, 어빌리티 활성=%d"),
		*GetNameSafe(OwnerActor), Distance, *RowName.ToString(), bActivated ? 1 : 0);
	if (bActivated)
	{
		if (const FLSMonsterActionRow* Row = FindActionRow(RowName))
		{
			StartActionCooldown(RowName, Row->Action_Cooldown);
		}
	}

	return bActivated;
}

bool ULSMonsterCombatComponent::HasUsableActionInRange(float Distance) const
{
	return !SelectActionForDistance(Distance).IsNone();
}

void ULSMonsterCombatComponent::PerformActionHit()
{
	ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	ULSCharacterCombatComponent* SharedCombatComponent = OwnerCharacter->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SharedCombatComponent || !DamageEffectClass || SharedCombatComponent->IsDead())
	{
		return;
	}

	const FLSMonsterActionRow* Row = GetActiveActionRow();
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: PerformActionHit skipped, active action row missing (%s)."), *GetNameSafe(OwnerCharacter), *ActiveActionRowName.ToString());
		return;
	}
	if (bActionUsesContactHit)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: 충돌형 돌진 중 LSAN_MonsterActionHit 호출을 무시합니다. ChargeHit/ChargeMiss 섹션에서 Hit Notify를 제거하세요."), *GetNameSafe(OwnerCharacter));
		return;
	}

	// 공격 타격 시점에 사용한 액션 row 출력.
	UE_LOG(LogLS, Log, TEXT("PerformActionHit %s: action row=%s (%s), 계수=%.2f"),
		*GetNameSafe(OwnerCharacter), *ActiveActionRowName.ToString(), *Row->Action_Name.ToString(), Row->Action_Multiplier);

	const float X = Row->Hitbox_X;
	const float Y = Row->Hitbox_Y;
	if (X <= 0.0f)
	{
		return;
	}

	// 원점은 몬스터 위치, 조준은 전방. 단 도약 액션은 착지 예정 지점/방향으로 판정해
	// 타격 프레임이 착지보다 일러도 데미지가 착지 위치에 들어가게 한다.
	FVector Origin = OwnerCharacter->GetActorLocation();
	FVector AimDir = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	if (bActionDashLandingValid)
	{
		Origin = ActionDashLandingLocation;
		if (!ActionDashDirection.IsNearlyZero())
		{
			AimDir = ActionDashDirection;
		}
	}
	const float QueryRadius = Row->Hitbox_Shape == ELSHitboxShape::Box
		? FMath::Sqrt(FMath::Square(X) + FMath::Square(Y * 0.5f))
		: X;

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	UKismetSystemLibrary::SphereOverlapActors(GetWorld(), Origin, QueryRadius, ObjectTypes, nullptr, ActorsToIgnore, OverlappedActors);

	const ELSBreakPowerTier BreakPower = ToBreakPowerTier(Row->Action_Impact);
	TSet<AActor*> UniqueTargets;
	for (AActor* HitActor : OverlappedActors)
	{
		if (!HitActor || UniqueTargets.Contains(HitActor))
		{
			continue;
		}

		if (!ULSHitboxLibrary::IsTargetInsideHitbox(Origin, AimDir, HitActor->GetActorLocation(), Row->Hitbox_Shape, X, X, Y, Y))
		{
			continue;
		}

		if (TryApplyActionDamage(*Row, HitActor, Origin, AimDir, BreakPower))
		{
			UniqueTargets.Add(HitActor);
		}
	}
}

bool ULSMonsterCombatComponent::TryApplyActionDamage(const FLSMonsterActionRow& Row, AActor* HitActor, const FVector& Origin, const FVector& AimDir, ELSBreakPowerTier BreakPower) const
{
	const ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	ULSCharacterCombatComponent* SharedCombatComponent = OwnerCharacter
		? OwnerCharacter->FindComponentByClass<ULSCharacterCombatComponent>()
		: nullptr;
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority() || !SharedCombatComponent || !DamageEffectClass || SharedCombatComponent->IsDead())
	{
		return false;
	}

	if (!SharedCombatComponent->ApplyDamageEffectToTarget(HitActor, DamageEffectClass, 1.0f, 0.0f, Row.Action_Multiplier, false, BreakPower))
	{
		return false;
	}

	ApplyActionCrowdControl(Row, HitActor, Origin, AimDir, BreakPower);
	return true;
}

void ULSMonsterCombatComponent::ApplyActionCrowdControl(const FLSMonsterActionRow& Row, AActor* HitActor, const FVector& Origin, const FVector& AimDir, ELSBreakPowerTier BreakPower) const
{
	if (Row.CC_Type == ELSCharacterSkillCrowdControlType::None || Row.CC_Value <= 0.0f || !HitActor)
	{
		return;
	}

	// 캐릭터 스킬(LSGA_Override)과 같은 경로: 강인도 게이트 통과 시 ApplyKnockback(속도=CC_Value).
	ULSCharacterCombatComponent* TargetCombatComponent = HitActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!TargetCombatComponent || !TargetCombatComponent->CanApplyCrowdControl(BreakPower))
	{
		return;
	}

	// 방향 규칙도 캐릭터 스킬과 동일 — Pull: 판정 원점 쪽 / KnockBack: 원형은 원점 반대쪽, 그 외 조준 방향.
	FVector Direction = AimDir;
	if (Row.CC_Type == ELSCharacterSkillCrowdControlType::Pull)
	{
		Direction = Origin - HitActor->GetActorLocation();
	}
	else if (Row.Hitbox_Shape == ELSHitboxShape::Circle)
	{
		Direction = HitActor->GetActorLocation() - Origin;
		Direction.Z = 0.0f;
		if (Direction.IsNearlyZero())
		{
			Direction = AimDir;
		}
	}

	TargetCombatComponent->ApplyKnockback(Direction.GetSafeNormal2D(), Row.CC_Value);
}

void ULSMonsterCombatComponent::ComputeActionOriginAndDirection(const FLSMonsterActionRow& Row, FVector& OutOrigin, FVector& OutDirection) const
{
	const AActor* OwnerActor = GetOwner();
	const FVector OwnerLocation = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
	FVector Direction = OwnerActor ? OwnerActor->GetActorForwardVector().GetSafeNormal2D() : FVector::ForwardVector;
	FVector Origin = OwnerLocation;

	// 도약 액션은 착지 예정 지점을 원점으로 — 타겟 방향으로, 타겟보다 멀리 가지 않게 클램프.
	if (Row.Dash_Distance > 0.0f)
	{
		float DashDistance = Row.Dash_Distance;
		if (const AActor* Target = ActiveTarget.Get())
		{
			FVector ToTarget = Target->GetActorLocation() - OwnerLocation;
			ToTarget.Z = 0.0f;
			const float DistToTarget = ToTarget.Size();
			if (DistToTarget > KINDA_SMALL_NUMBER)
			{
				Direction = ToTarget / DistToTarget;
				DashDistance = FMath::Min(Row.Dash_Distance, DistToTarget);
			}
		}
		Origin = OwnerLocation + Direction * DashDistance;
	}

	OutOrigin = Origin;
	OutDirection = Direction;
}

void ULSMonsterCombatComponent::ComputeActionTelegraphOriginAndDirection(
	ELSMonsterTelegraphOrigin OriginMode,
	FVector& OutOrigin,
	FVector& OutDirection) const
{
	const AActor* OwnerActor = GetOwner();
	OutOrigin = OwnerActor ? OwnerActor->GetActorLocation() : FVector::ZeroVector;
	OutDirection = OwnerActor ? OwnerActor->GetActorForwardVector().GetSafeNormal2D() : FVector::ForwardVector;

	const AActor* Target = OriginMode == ELSMonsterTelegraphOrigin::Target ? ActiveTarget.Get() : nullptr;
	if (!Target)
	{
		return;
	}

	FVector ToTarget = Target->GetActorLocation() - OutOrigin;
	ToTarget.Z = 0.0f;
	if (!ToTarget.IsNearlyZero())
	{
		OutDirection = ToTarget.GetSafeNormal();
	}
	OutOrigin = Target->GetActorLocation();
}

void ULSMonsterCombatComponent::PerformActionDash()
{
	ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}
	if (bActionUsesContactHit)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: 충돌형 돌진에 LSAN_MonsterActionDash가 함께 호출되어 기존 도약을 무시합니다."), *GetNameSafe(OwnerCharacter));
		return;
	}

	const FLSMonsterActionRow* Row = GetActiveActionRow();
	if (!Row || Row->Dash_Distance <= 0.0f || Row->Duration <= 0.0f)
	{
		// 도약이 아닌 액션(Dash_Distance=0)은 전진 이동 없음.
		return;
	}

	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (!MovementComponent)
	{
		return;
	}

	// 착지 지점/방향은 텔레그래프·타격과 동일하게 계산한다(공유 헬퍼).
	const FVector OwnerLocation = OwnerCharacter->GetActorLocation();
	FVector LandingLocation;
	FVector DashDir;
	ComputeActionOriginAndDirection(*Row, LandingLocation, DashDir);
	const float DashDistance = (LandingLocation - OwnerLocation).Size();
	if (DashDir.IsNearlyZero() || DashDistance <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	// 이전 도약 소스/착지정보 정리 후 새로 적용(LSGA_Dash와 동일한 ConstantForce 패턴).
	EndActionDash();

	// 착지 예정 지점/방향 저장 — PerformActionHit이 타격 프레임 타이밍과 무관하게 착지 위치에 판정한다.
	ActionDashLandingLocation = LandingLocation;
	ActionDashDirection = DashDir;
	bActionDashLandingValid = true;

	const float DashSpeed = DashDistance / Row->Duration;
	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("MonsterActionDash");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 6; // 넉백(8)보다 낮아 피격 시 도약 중단, 일반 대쉬(5)보다는 높음.
	RootMotion->Force = DashDir * DashSpeed;
	RootMotion->Duration = Row->Duration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	ActionDashRootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);
}

void ULSMonsterCombatComponent::BeginActionCharge()
{
	ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	const FLSMonsterActionRow* Row = GetActiveActionRow();
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
		return;
	}

	CancelActionCharge();
	bActionUsesContactHit = true;
	if (!Row || Row->Dash_Distance <= 0.0f || Row->Duration <= 0.0f)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: 충돌형 돌진 시작 실패. 활성 row의 Dash_Distance/Duration을 확인하세요."), *GetNameSafe(OwnerCharacter));
		ActionChargeFinishedDelegate.Broadcast(false);
		return;
	}

	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	ActionDashDirection = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	if (!MovementComponent || ActionDashDirection.IsNearlyZero())
	{
		ActionChargeFinishedDelegate.Broadcast(false);
		return;
	}

	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("MonsterActionCharge");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 6;
	RootMotion->Force = ActionDashDirection * (Row->Dash_Distance / Row->Duration);
	RootMotion->Duration = Row->Duration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;
	ActionDashRootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);

	bActionChargeActive = ActionDashRootMotionSourceID != 0;
	if (!bActionChargeActive)
	{
		ActionChargeFinishedDelegate.Broadcast(false);
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(ActionChargeTimerHandle, this, &ULSMonsterCombatComponent::HandleActionChargeTimeout, Row->Duration, false);
	// [돌진 진단] 시작 상태 덤프.
	UE_LOG(LogLS, Log, TEXT("[돌진] Begin %s: 소스ID=%u, Duration=%.2f, Dist=%.0f, 속도=%.0f, 방향=%s"),
		*GetNameSafe(OwnerCharacter), ActionDashRootMotionSourceID, Row->Duration, Row->Dash_Distance,
		Row->Dash_Distance / Row->Duration, *ActionDashDirection.ToString());
	ActionChargeStartedDelegate.Broadcast();
}

void ULSMonsterCombatComponent::CancelActionCharge()
{
	bActionChargeActive = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActionChargeTimerHandle);
	}
	EndActionDash();
}

void ULSMonsterCombatComponent::FinishActionCharge(bool bHit)
{
	if (!bActionChargeActive)
	{
		return;
	}

	bActionChargeActive = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ActionChargeTimerHandle);
	}
	EndActionDash();
	ActionChargeFinishedDelegate.Broadcast(bHit);
}

void ULSMonsterCombatComponent::HandleActionChargeTimeout()
{
	// [돌진 진단] Duration 만료로 종료된 경우.
	UE_LOG(LogLS, Log, TEXT("[돌진] Timeout 종료 %s"), *GetNameSafe(GetOwner()));
	FinishActionCharge(false);
}

void ULSMonsterCombatComponent::HandleOwnerCapsuleHit(
	UPrimitiveComponent* HitComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	FVector NormalImpulse,
	const FHitResult& Hit)
{
	ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	if (!bActionChargeActive || !OwnerCharacter || !OwnerCharacter->HasAuthority() || !OtherActor || OtherActor == OwnerCharacter)
	{
		return;
	}

	// [돌진 진단] 돌진 중 발생한 캡슐 충돌 전부 기록.
	{
		UCharacterMovementComponent* DiagMovement = OwnerCharacter->GetCharacterMovement();
		const bool bWalkable = Hit.bBlockingHit && DiagMovement && DiagMovement->IsWalkable(Hit);
		UE_LOG(LogLS, Log, TEXT("[돌진] CapsuleHit: Other=%s(%s), Comp=%s, Blocking=%d, Walkable=%d, Normal=%s"),
			*GetNameSafe(OtherActor), *OtherActor->GetClass()->GetName(), *GetNameSafe(OtherComponent),
			Hit.bBlockingHit ? 1 : 0, bWalkable ? 1 : 0, *Hit.ImpactNormal.ToString());
	}

	if (ALSPlayerCharacter* PlayerCharacter = Cast<ALSPlayerCharacter>(OtherActor))
	{
		ULSCharacterCombatComponent* TargetCombatComponent = PlayerCharacter->GetCharacterCombatComponent();
		if (TargetCombatComponent && !TargetCombatComponent->IsDead())
		{
			if (const FLSMonsterActionRow* Row = GetActiveActionRow())
			{
				TryApplyActionDamage(*Row, PlayerCharacter, OwnerCharacter->GetActorLocation(), ActionDashDirection, ToBreakPowerTier(Row->Action_Impact));
				FinishActionCharge(true);
				return;
			}
		}

		FinishActionCharge(false);
		return;
	}

	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (Hit.bBlockingHit && (!MovementComponent || !MovementComponent->IsWalkable(Hit)))
	{
		FinishActionCharge(false);
	}
}

void ULSMonsterCombatComponent::EndActionDash()
{
	if (ActionDashRootMotionSourceID != 0)
	{
		if (const ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner()))
		{
			if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
			{
				MovementComponent->RemoveRootMotionSourceByID(ActionDashRootMotionSourceID);
			}
		}
	}

	ActionDashRootMotionSourceID = 0;
	bActionDashLandingValid = false;
}

void ULSMonsterCombatComponent::BeginActionTelegraph(float Duration, ELSMonsterTelegraphOrigin OriginMode)
{
	if (!ShouldShowActionTelegraph())
	{
		return;
	}

	// fill 차오름 기준 시간(윈드업 NotifyState 윈도우). 매 Tick 경과 비율로 0→1.
	TelegraphDuration = Duration;
	TelegraphElapsed = 0.0f;

	const AActor* OwnerActor = GetOwner();
	const FLSMonsterActionRow* Row = GetActiveActionRow();
	ULSSkillPreviewComponent* Preview = GetPreviewComponent();
	if (!OwnerActor || !Row || !Preview)
	{
		return;
	}

	const float X = Row->Hitbox_X;
	const float Y = Row->Hitbox_Y;

	// 텔레그래프는 위험 표시용 전용 재질을 오버라이드로 넘긴다(프리뷰 컴포넌트 기본 재질 미사용).
	FLSSkillAreaPreviewSpec Spec;
	Spec.LocationMode = ELSSkillPreviewLocationMode::CasterOrigin;
	UMaterialInterface* TelegraphMaterial = TelegraphCircleMaterial;
	switch (Row->Hitbox_Shape)
	{
	case ELSHitboxShape::Box:
		Spec.Shape = ELSSkillAreaShape::Box;
		Spec.BoxLength = X;
		Spec.BoxWidth = Y;
		Spec.OutlineThickness = 0.2f;
		TelegraphMaterial = TelegraphBoxMaterial;
		break;
	case ELSHitboxShape::Cone:
		Spec.Shape = ELSSkillAreaShape::Circle;
		Spec.Radius = X;
		Spec.Degrees = Y;
		break;
	case ELSHitboxShape::Circle:
	default:
		Spec.Shape = ELSSkillAreaShape::Circle;
		Spec.Radius = X;
		Spec.Degrees = 360.0f;
		break;
	}

	if (Preview->BeginAreaPreview(Spec, TelegraphMaterial))
	{
		// NotifyState가 선택한 시전자/타겟의 Begin 시점 위치에 한 번 배치한다.
		FVector PreviewOrigin;
		FVector PreviewDirection;
		ComputeActionTelegraphOriginAndDirection(OriginMode, PreviewOrigin, PreviewDirection);

		// 박스 히트박스는 원점(뒷변)에서 전방으로 뻗지만 프리뷰 메시는 중심 정렬이라,
		// 전방 절반만큼 밀어 실제 판정과 표시를 맞춘다(플레이어 LocationOffset.X=Range_X*0.5와 동일).
		if (Row->Hitbox_Shape == ELSHitboxShape::Box && !PreviewDirection.IsNearlyZero())
		{
			PreviewOrigin += PreviewDirection * (X * 0.5f);
		}

		const FRotator PreviewRotation = PreviewDirection.IsNearlyZero()
			? OwnerActor->GetActorRotation()
			: PreviewDirection.Rotation();
		Preview->UpdateAreaPreview(PreviewOrigin, PreviewRotation);
		Preview->SetAreaFillAmount(0.0f); // 채움 시작점.
	}
}

void ULSMonsterCombatComponent::UpdateActionTelegraphFill(float DeltaSeconds)
{
	ULSSkillPreviewComponent* Preview = GetPreviewComponent();
	if (!Preview || !Preview->IsAreaPreviewActive())
	{
		return;
	}

	TelegraphElapsed += DeltaSeconds;
	const float Alpha = TelegraphDuration > 0.0f
		? FMath::Clamp(TelegraphElapsed / TelegraphDuration, 0.0f, 1.0f)
		: 1.0f;
	Preview->SetAreaFillAmount(Alpha);
}

void ULSMonsterCombatComponent::EndActionTelegraph()
{
	TelegraphDuration = 0.0f;
	TelegraphElapsed = 0.0f;

	if (ULSSkillPreviewComponent* Preview = GetPreviewComponent())
	{
		Preview->EndAreaPreview();
	}
}

const FLSMonsterActionRow* ULSMonsterCombatComponent::GetActiveActionRow() const
{
	return FindActionRow(ActiveActionRowName);
}

bool ULSMonsterCombatComponent::HasValidDamageEffect() const
{
	return bCombatArchetypeApplied && DamageEffectClass != nullptr;
}

const FLSMonsterActionRow* ULSMonsterCombatComponent::FindActionRow(FName RowName) const
{
	if (!MonsterActionTable || RowName.IsNone())
	{
		return nullptr;
	}

	return MonsterActionTable->FindRow<FLSMonsterActionRow>(RowName, TEXT("LSMonsterCombatComponent"));
}

FName ULSMonsterCombatComponent::SelectActionForDistance(float Distance) const
{
	// 현재 거리가 사거리대에 맞고 쿨다운이 준비된 후보 중, 쿨다운이 가장 긴(=강한) 액션을 우선한다.
	// 동률이면 Action_Group 순서가 앞선 것을 유지한다.
	FName BestRowName = NAME_None;
	float BestCooldown = -1.0f;
	for (const FName& RowName : ActionGroup)
	{
		const FLSMonsterActionRow* Row = FindActionRow(RowName);
		if (!Row)
		{
			continue;
		}

		if (Distance < Row->Action_Range_Min || Distance > Row->Action_Range_Max)
		{
			continue;
		}

		if (IsActionOnCooldown(RowName))
		{
			continue;
		}

		if (Row->Action_Cooldown > BestCooldown)
		{
			BestCooldown = Row->Action_Cooldown;
			BestRowName = RowName;
		}
	}

	return BestRowName;
}

bool ULSMonsterCombatComponent::IsActionOnCooldown(FName RowName) const
{
	const double* EndTime = ActionCooldownEndTimes.Find(RowName);
	if (!EndTime)
	{
		return false;
	}

	const UWorld* World = GetWorld();
	return World && World->GetTimeSeconds() < *EndTime;
}

void ULSMonsterCombatComponent::StartActionCooldown(FName RowName, float Cooldown)
{
	if (Cooldown <= 0.0f)
	{
		return;
	}

	if (const UWorld* World = GetWorld())
	{
		ActionCooldownEndTimes.Add(RowName, World->GetTimeSeconds() + Cooldown);
	}
}

ULSSkillPreviewComponent* ULSMonsterCombatComponent::GetPreviewComponent() const
{
	const AActor* OwnerActor = GetOwner();
	return OwnerActor ? OwnerActor->FindComponentByClass<ULSSkillPreviewComponent>() : nullptr;
}

bool ULSMonsterCombatComponent::ShouldShowActionTelegraph() const
{
	// 확장점: 추후 전투 프로토콜 레벨/관전 조건에 따라 false를 반환하도록 게이팅할 수 있다.
	return true;
}

ELSBreakPowerTier ULSMonsterCombatComponent::ToBreakPowerTier(int32 Impact)
{
	if (Impact >= static_cast<int32>(ELSBreakPowerTier::HardCrowdControl))
	{
		return ELSBreakPowerTier::HardCrowdControl;
	}

	if (Impact >= static_cast<int32>(ELSBreakPowerTier::SpecialAttack))
	{
		return ELSBreakPowerTier::SpecialAttack;
	}

	return ELSBreakPowerTier::NormalAttack;
}
