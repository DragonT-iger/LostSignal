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

	bool ShouldApplyOverrideSelfEffect(const FLSCharacterSkillRow& Row, bool bHasRow)
	{
		return !bHasRow
			|| Row.Effect_Target == ELSCharacterSkillEffectTarget::None
			|| Row.Effect_Target == ELSCharacterSkillEffectTarget::Self;
	}
}

ULSGA_Override::ULSGA_Override()
{
	DamageEffectClass = ULSGE_PlayerBasicDamage::StaticClass();

	ActivationBlockedTags.AddTag(LSGameplayTags::State_Dead);
	ActivationBlockedTags.AddTag(LSGameplayTags::State_Stunned);
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

	const FLSCharacterSkillRow* Row = SkillContext.bHasSkillRow ? &SkillContext.SkillRow : nullptr;
	const ULSOverrideSkillDataAsset* OverrideData = Cast<ULSOverrideSkillDataAsset>(SkillContext.SkillData);
	const ELSCharacterSkillRangeShape ResolvedShape = Row && Row->Range_Shape != ELSCharacterSkillRangeShape::None
		? Row->Range_Shape
		: OverrideData ? OverrideData->FallbackRangeShape : ELSCharacterSkillRangeShape::Circle;
	const float RowRangeX = Row ? Row->Range_X : 0.0f;
	const float RowRangeY = Row ? Row->Range_Y : 0.0f;
	const float Radius = ResolveOverrideDataFloat(RowRangeX, 0.0f, OverrideData ? OverrideData->FallbackRadius : 0.0f, FallbackRadius);
	const float Length = ResolveOverrideDataFloat(RowRangeX, 0.0f, OverrideData ? OverrideData->FallbackLength : 0.0f, Radius);
	const float Width = ResolveOverrideDataFloat(RowRangeY, 0.0f, OverrideData ? OverrideData->FallbackWidth : 0.0f, Radius);
	const float ConeAngleDegrees = ResolveOverrideDataFloat(RowRangeY, 0.0f, OverrideData ? OverrideData->FallbackConeAngleDegrees : 0.0f, 60.0f);
	const float ResolvedAttackCoefficient = ResolveOverrideDataFloat(
		Row ? Row->Skill_Multiplier : 0.0f,
		0.0f,
		OverrideData ? OverrideData->FallbackAttackCoefficient : 0.0f,
		FallbackAttackCoefficient);
	const ELSBreakPowerTier ResolvedBreakPower = Row ? ToOverrideAbilityBreakPowerTier(Row->Skill_Impact, BreakPower) : BreakPower;
	const float ResolvedKnockbackDuration = Row && Row->Skill_Time > 0.0f
		? Row->Skill_Time
		: OverrideData ? OverrideData->FallbackKnockbackDuration : FallbackKnockbackDuration;
	const float ResolvedKnockbackSpeed = Row && Row->CC_Value > 0.0f
		? Row->CC_Value
		: OverrideData && OverrideData->FallbackKnockbackSpeed > 0.0f ? OverrideData->FallbackKnockbackSpeed : KnockbackSpeed;
	const float ResolvedKnockbackUpSpeed = OverrideData ? OverrideData->FallbackKnockbackUpSpeed : KnockbackUpSpeed;
	const ELSCharacterSkillCrowdControlType ResolvedCCType = Row && Row->CC_Type != ELSCharacterSkillCrowdControlType::None
		? Row->CC_Type
		: ELSCharacterSkillCrowdControlType::KnockBack;
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
			0.0f,
			ResolvedAttackCoefficient,
			bCanCrit,
			ResolvedBreakPower))
		{
			continue;
		}

		++ValidHitCount;

		// 스킬 row에 정의된 상태이상(스탯 디버프 등)을 명중 대상/자신에게 적용한다. CC_Type 처리와는 별개.
		if (Row)
		{
			SourceCombatComponent->ApplyStatusEffectFromRow(Row->Status_ID, Row->Effect_Target, Row->Skill_Effect_Duration, TargetActor);
			SourceCombatComponent->ApplyStatusEffectFromRow(Row->Status_ID_2, Row->Effect_Target_2, Row->Skill_Effect_Duration_2, TargetActor);
		}

		if (ResolvedCCType == ELSCharacterSkillCrowdControlType::None)
		{
			continue;
		}

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
		if (ResolvedCCType == ELSCharacterSkillCrowdControlType::Pull)
		{
			KnockbackDirection = SourceLocation - TargetActor->GetActorLocation();
		}
		else if (ResolvedShape == ELSCharacterSkillRangeShape::Circle)
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

	if (OverrideData && OverrideData->bApplyAttackSpeedBuff && (!Row || ShouldApplyOverrideSelfEffect(*Row, true)))
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		const float AttackSpeedBonus = OverrideData->FallbackAttackSpeedBonus;
		const float AttackSpeedDuration = Row && Row->Skill_Effect_Duration > 0.0f ? Row->Skill_Effect_Duration : OverrideData->FallbackAttackSpeedDuration;
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
			TEXT("[GA_Override] Source=%s Shape=%d QueryRadius=%.2f Radius=%.2f Length=%.2f Width=%.2f RawTargets=%d ValidHits=%d Knockbacks=%d Coef=%.2f BreakPower=%d"),
			*GetNameSafe(SourceActor),
			static_cast<int32>(ResolvedShape),
			QueryRadius,
			Radius,
			Length,
			Width,
			OverlappedActors.Num(),
			ValidHitCount,
			KnockbackCount,
			ResolvedAttackCoefficient,
			static_cast<int32>(ResolvedBreakPower));
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
