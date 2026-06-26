#include "AI/LSMonsterCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Combat/LSHitboxLibrary.h"
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

ULSMonsterCombatComponent::ULSMonsterCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	DamageEffectClass = ULSGE_MonsterBasicDamage::StaticClass();
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

		if (SharedCombatComponent->ApplyDamageEffectToTarget(HitActor, DamageEffectClass, 1.0f, 0.0f, Row->Action_Multiplier, false, BreakPower))
		{
			UniqueTargets.Add(HitActor);
		}
	}
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

void ULSMonsterCombatComponent::PerformActionDash()
{
	ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->HasAuthority())
	{
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

void ULSMonsterCombatComponent::EndActionDash()
{
	if (ActionDashRootMotionSourceID == 0)
	{
		return;
	}

	if (const ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner()))
	{
		if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
		{
			MovementComponent->RemoveRootMotionSourceByID(ActionDashRootMotionSourceID);
		}
	}

	ActionDashRootMotionSourceID = 0;
	bActionDashLandingValid = false;
}

void ULSMonsterCombatComponent::BeginActionTelegraph()
{
	if (!ShouldShowActionTelegraph())
	{
		return;
	}

	const AActor* OwnerActor = GetOwner();
	const FLSMonsterActionRow* Row = GetActiveActionRow();
	ULSSkillPreviewComponent* Preview = GetPreviewComponent();
	if (!OwnerActor || !Row || !Preview)
	{
		return;
	}

	const float X = Row->Hitbox_X;
	const float Y = Row->Hitbox_Y;

	FLSSkillAreaPreviewSpec Spec;
	Spec.LocationMode = ELSSkillPreviewLocationMode::CasterOrigin;
	switch (Row->Hitbox_Shape)
	{
	case ELSHitboxShape::Box:
		Spec.Shape = ELSSkillAreaShape::Box;
		Spec.BoxLength = X;
		Spec.BoxWidth = Y;
		Spec.Material = TelegraphBoxMaterial;
		break;
	case ELSHitboxShape::Cone:
		Spec.Shape = ELSSkillAreaShape::Circle;
		Spec.Radius = X;
		Spec.Degrees = Y;
		Spec.Material = TelegraphCircleMaterial;
		break;
	case ELSHitboxShape::Circle:
	default:
		Spec.Shape = ELSSkillAreaShape::Circle;
		Spec.Radius = X;
		Spec.Degrees = 360.0f;
		Spec.Material = TelegraphCircleMaterial;
		break;
	}

	if (Preview->BeginAreaPreview(Spec))
	{
		// 도약 액션이면 착지 예정 지점/방향에 표시(비-도약은 몬스터 현재 위치·방향).
		FVector PreviewOrigin;
		FVector PreviewDirection;
		ComputeActionOriginAndDirection(*Row, PreviewOrigin, PreviewDirection);
		const FRotator PreviewRotation = PreviewDirection.IsNearlyZero()
			? OwnerActor->GetActorRotation()
			: PreviewDirection.Rotation();
		Preview->UpdateAreaPreview(PreviewOrigin, PreviewRotation);
	}
}

void ULSMonsterCombatComponent::EndActionTelegraph()
{
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
