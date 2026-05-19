#include "GAS/Abilities/Character1/LSGA_Override.h"

#include "AbilitySystemComponent.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Data/LSCharacterSkillRow.h"
#include "Engine/EngineTypes.h"
#include "GAS/Effects/LSGE_PlayerBasicDamage.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Skills/LSOverrideSkillDataAsset.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSSkillDataAsset.h"

namespace
{
	ELSBreakPowerTier ToOverrideAbilityBreakPowerTier(int32 Value, ELSBreakPowerTier Fallback)
	{
		if (Value >= static_cast<int32>(ELSBreakPowerTier::HardCrowdControl))
		{
			return ELSBreakPowerTier::HardCrowdControl;
		}

		if (Value >= static_cast<int32>(ELSBreakPowerTier::SpecialAttack))
		{
			return ELSBreakPowerTier::SpecialAttack;
		}

		if (Value >= static_cast<int32>(ELSBreakPowerTier::NormalAttack))
		{
			return ELSBreakPowerTier::NormalAttack;
		}

		return Fallback;
	}

	float ResolveOverrideDataFloat(float RowValue, float SkillDataValue, float VariantValue, float AbilityFallback)
	{
		if (RowValue > 0.0f)
		{
			return RowValue;
		}

		if (SkillDataValue > 0.0f)
		{
			return SkillDataValue;
		}

		return VariantValue > 0.0f ? VariantValue : AbilityFallback;
	}

	FVector ResolveOverrideAimDirection(const AActor* SourceActor, const FLSSkillActivationContext& SkillContext)
	{
		if (!SourceActor)
		{
			return FVector::ForwardVector;
		}

		FVector AimDirection = SkillContext.TargetLocation - SourceActor->GetActorLocation();
		AimDirection.Z = 0.0f;
		if (AimDirection.IsNearlyZero())
		{
			AimDirection = FRotator(0.0f, SkillContext.AimYaw, 0.0f).Vector();
		}

		AimDirection = AimDirection.GetSafeNormal2D();
		return AimDirection.IsNearlyZero() ? SourceActor->GetActorForwardVector().GetSafeNormal2D() : AimDirection;
	}

	bool IsOverrideTargetInsideRange(
		const AActor* SourceActor,
		const AActor* TargetActor,
		ELSCharacterSkillRangeShape Shape,
		const FVector& AimDirection,
		float Radius,
		float Length,
		float Width,
		float ConeAngleDegrees)
	{
		if (!SourceActor || !TargetActor)
		{
			return false;
		}

		FVector ToTarget = TargetActor->GetActorLocation() - SourceActor->GetActorLocation();
		ToTarget.Z = 0.0f;
		const float Distance = ToTarget.Size2D();
		if (Distance <= KINDA_SMALL_NUMBER)
		{
			return true;
		}

		switch (Shape)
		{
		case ELSCharacterSkillRangeShape::Cone:
			{
				if (Distance > Radius)
				{
					return false;
				}

				const float Dot = FVector::DotProduct(AimDirection, ToTarget.GetSafeNormal2D());
				const float HalfAngle = FMath::Clamp(ConeAngleDegrees * 0.5f, 0.0f, 180.0f);
				return Dot >= FMath::Cos(FMath::DegreesToRadians(HalfAngle));
			}

		case ELSCharacterSkillRangeShape::Box:
			{
				const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, AimDirection).GetSafeNormal();
				const float ForwardDistance = FVector::DotProduct(ToTarget, AimDirection);
				const float RightDistance = FVector::DotProduct(ToTarget, RightDirection);
				return ForwardDistance >= 0.0f &&
					ForwardDistance <= Length &&
					FMath::Abs(RightDistance) <= Width * 0.5f;
			}

		case ELSCharacterSkillRangeShape::Circle:
		default:
			return Distance <= Radius;
		}
	}
}

ULSGA_Override::ULSGA_Override()
{
	DamageEffectClass = ULSGE_PlayerBasicDamage::StaticClass();

	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::Combat_Attacking);
	ActivationOwnedTags.AddTag(LSGameplayTags::Combat_Attacking);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void ULSGA_Override::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	AActor* SourceActor = GetAvatarActorFromActorInfo();
	ULSPlayerSkillComponent* SkillComponent = SourceActor ? SourceActor->FindComponentByClass<ULSPlayerSkillComponent>() : nullptr;
	if (!SourceActor || !SourceActor->HasAuthority() || !SkillComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FLSSkillActivationContext SkillContext;
	if (!SkillComponent->ConsumePendingAbilityContext(GetClass(), SkillContext) || !SkillContext.SkillData)
	{
		UE_LOG(LogLS, Warning, TEXT("%s Override ability missing pending skill context."), *GetNameSafe(SourceActor));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const TSubclassOf<UGameplayEffect> ResolvedDamageEffectClass = SkillContext.SkillData->DamageEffectClass ? SkillContext.SkillData->DamageEffectClass : DamageEffectClass;
	ULSCharacterCombatComponent* SourceCombatComponent = SourceActor->FindComponentByClass<ULSCharacterCombatComponent>();
	if (!SourceCombatComponent || !ResolvedDamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	SkillComponent->ApplySkillCooldown(SkillContext.SkillData);

	FLSCharacterSkillRow Row;
	const bool bHasRow = SkillContext.SkillData->TryGetSkillRow(Row);
	const ULSOverrideSkillDataAsset* OverrideData = Cast<ULSOverrideSkillDataAsset>(SkillContext.SkillData);
	const ELSCharacterSkillRangeShape ResolvedShape = bHasRow && Row.Range_Shape != ELSCharacterSkillRangeShape::None
		? Row.Range_Shape
		: OverrideData ? OverrideData->FallbackRangeShape : ELSCharacterSkillRangeShape::Circle;
	const float Radius = ResolveOverrideDataFloat(Row.Range_X, 0.0f, OverrideData ? OverrideData->FallbackRadius : 0.0f, FallbackRadius);
	const float Length = ResolveOverrideDataFloat(Row.Range_X, 0.0f, OverrideData ? OverrideData->FallbackLength : 0.0f, Radius);
	const float Width = ResolveOverrideDataFloat(Row.Range_Y, 0.0f, OverrideData ? OverrideData->FallbackWidth : 0.0f, Radius);
	const float ConeAngleDegrees = ResolveOverrideDataFloat(Row.Range_Y, 0.0f, OverrideData ? OverrideData->FallbackConeAngleDegrees : 0.0f, 60.0f);
	const float ResolvedAttackCoefficient = ResolveOverrideDataFloat(
		bHasRow ? Row.Skill_Multiplier : 0.0f,
		SkillContext.SkillData->AttackCoefficient,
		OverrideData ? OverrideData->FallbackAttackCoefficient : 0.0f,
		FallbackAttackCoefficient);
	const float ResolvedFixedDamage = SkillContext.SkillData->FixedDamage > 0.0f
		? SkillContext.SkillData->FixedDamage
		: OverrideData ? OverrideData->FallbackFixedDamage : FixedDamage;
	const ELSBreakPowerTier ResolvedBreakPower = bHasRow ? ToOverrideAbilityBreakPowerTier(Row.Skill_Impact, SkillContext.SkillData->BreakPower) : SkillContext.SkillData->BreakPower;
	const float ResolvedKnockbackDuration = bHasRow && Row.Skill_Time > 0.0f
		? Row.Skill_Time
		: OverrideData ? OverrideData->FallbackKnockbackDuration : FallbackKnockbackDuration;
	const float ResolvedKnockbackSpeed = OverrideData && OverrideData->FallbackKnockbackSpeed > 0.0f ? OverrideData->FallbackKnockbackSpeed : KnockbackSpeed;
	const float ResolvedKnockbackUpSpeed = OverrideData ? OverrideData->FallbackKnockbackUpSpeed : KnockbackUpSpeed;
	const FVector AimDirection = ResolveOverrideAimDirection(SourceActor, SkillContext);
	const float QueryRadius = ResolvedShape == ELSCharacterSkillRangeShape::Box
		? FMath::Sqrt(FMath::Square(Length) + FMath::Square(Width * 0.5f))
		: Radius;
	if (QueryRadius <= 0.0f || AimDirection.IsNearlyZero())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(SourceActor);
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	const FVector SourceLocation = SourceActor->GetActorLocation();
	UKismetSystemLibrary::SphereOverlapActors(
		SourceActor->GetWorld(),
		SourceLocation,
		QueryRadius,
		ObjectTypes,
		nullptr,
		ActorsToIgnore,
		OverlappedActors);

	int32 ValidHitCount = 0;
	int32 KnockbackCount = 0;
	for (AActor* TargetActor : OverlappedActors)
	{
		if (!TargetActor)
		{
			continue;
		}

		if (!IsOverrideTargetInsideRange(SourceActor, TargetActor, ResolvedShape, AimDirection, Radius, Length, Width, ConeAngleDegrees))
		{
			continue;
		}

		if (!SourceCombatComponent->ApplyDamageEffectToTarget(
			TargetActor,
			ResolvedDamageEffectClass,
			1.0f,
			ResolvedFixedDamage,
			ResolvedAttackCoefficient,
			SkillContext.SkillData->bCanCrit,
			ResolvedBreakPower))
		{
			continue;
		}

		++ValidHitCount;

		ULSCharacterCombatComponent* TargetCombatComponent = TargetActor->FindComponentByClass<ULSCharacterCombatComponent>();
		if (TargetCombatComponent && !TargetCombatComponent->CanApplyCrowdControl(ResolvedBreakPower))
		{
			continue;
		}

		if (!TargetCombatComponent)
		{
			continue;
		}

		FVector KnockbackDirection = AimDirection;
		if (ResolvedShape == ELSCharacterSkillRangeShape::Circle)
		{
			KnockbackDirection = TargetActor->GetActorLocation() - SourceLocation;
			KnockbackDirection.Z = 0.0f;
			if (KnockbackDirection.IsNearlyZero())
			{
				KnockbackDirection = SourceActor->GetActorForwardVector();
			}
		}

		KnockbackDirection = KnockbackDirection.GetSafeNormal2D();
		if (TargetCombatComponent->ApplyKnockback(KnockbackDirection, ResolvedKnockbackSpeed, ResolvedKnockbackDuration, ResolvedKnockbackUpSpeed))
		{
			++KnockbackCount;
		}
	}

	if (OverrideData && OverrideData->bApplyAttackSpeedBuff)
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		const float AttackSpeedBonus = bHasRow && Row.Skill_Effect_Value > 0.0f ? Row.Skill_Effect_Value : OverrideData->FallbackAttackSpeedBonus;
		const float AttackSpeedDuration = bHasRow && Row.Skill_Effect_Duration > 0.0f ? Row.Skill_Effect_Duration : OverrideData->FallbackAttackSpeedDuration;
		if (ASC && OverrideData->AttackSpeedBuffEffectClass && AttackSpeedBonus > 0.0f && AttackSpeedDuration > 0.0f)
		{
			FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
			EffectContext.AddSourceObject(SkillContext.SkillData);

			const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(OverrideData->AttackSpeedBuffEffectClass, 1.0f, EffectContext);
			if (SpecHandle.IsValid())
			{
				SpecHandle.Data->SetDuration(AttackSpeedDuration, true);
				SpecHandle.Data->SetSetByCallerMagnitude(LSGameplayTags::Data_Buff_AttackSpeed, AttackSpeedBonus);
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}

	if (bEnableDebugLog || (OverrideData && OverrideData->bEnableDebugLog))
	{
		UE_LOG(
			LogLS,
			Log,
			TEXT("[GA_Override] Source=%s Shape=%d QueryRadius=%.2f Radius=%.2f Length=%.2f Width=%.2f RawTargets=%d ValidHits=%d Knockbacks=%d Fixed=%.2f Coef=%.2f BreakPower=%d"),
			*GetNameSafe(SourceActor),
			static_cast<int32>(ResolvedShape),
			QueryRadius,
			Radius,
			Length,
			Width,
			OverlappedActors.Num(),
			ValidHitCount,
			KnockbackCount,
			ResolvedFixedDamage,
			ResolvedAttackCoefficient,
			static_cast<int32>(ResolvedBreakPower));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
