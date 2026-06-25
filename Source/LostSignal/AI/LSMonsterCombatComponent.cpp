#include "AI/LSMonsterCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Combat/LSHitboxLibrary.h"
#include "Data/LSMonsterActionRow.h"
#include "Data/LSMonsterArchetypeRow.h"
#include "Engine/World.h"
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
		return false;
	}

	const float Distance = Target
		? FVector::Dist2D(OwnerActor->GetActorLocation(), Target->GetActorLocation())
		: 0.0f;

	const FName RowName = SelectActionForDistance(Distance);
	if (RowName.IsNone())
	{
		return false;
	}

	ActiveActionRowName = RowName;
	ActiveTarget = Target;

	const bool bActivated = RequestAbilityByTag(LSGameplayTags::Ability_MonsterAction);
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

	const float X = Row->Hitbox_X;
	const float Y = Row->Hitbox_Y;
	if (X <= 0.0f)
	{
		return;
	}

	// 원점은 몬스터 위치, 조준은 전방. Dash(도약) 액션은 몽타주 루트모션으로 접근한 뒤 타격 프레임에 판정한다.
	const FVector Origin = OwnerCharacter->GetActorLocation();
	const FVector AimDir = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
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
		Preview->UpdateAreaPreview(OwnerActor->GetActorLocation(), OwnerActor->GetActorRotation());
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
	// Action_Group 순서대로: 현재 거리가 사거리대에 맞고 쿨다운이 준비된 첫 액션을 선택한다.
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

		return RowName;
	}

	return NAME_None;
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
