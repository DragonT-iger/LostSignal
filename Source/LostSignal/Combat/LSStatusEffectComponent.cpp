#include "Combat/LSStatusEffectComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSStatusEffectRow.h"
#include "Engine/GameInstance.h"
#include "GameFramework/Actor.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GameplayEffect.h"
#include "LostSignal.h"
#include "TimerManager.h"

namespace
{
	// CombatAcceleration과 같은 규칙: Percent 입력이 1보다 크면 퍼센트 포인트로 보고 0.01을 곱해 비율로 환산한다.
	float NormalizeModifierFraction(float Value)
	{
		if (FMath::Abs(Value) > 1.0f)
		{
			return Value * 0.01f;
		}

		return Value;
	}

	const ULSGameDataSubsystem* ResolveGameDataSubsystem(const AActor* OwnerActor)
	{
		const UWorld* World = OwnerActor ? OwnerActor->GetWorld() : nullptr;
		const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
		return GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	}
}

ULSStatusEffectComponent::ULSStatusEffectComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* ULSStatusEffectComponent::GetAbilitySystemComponent() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

FGameplayAttribute ULSStatusEffectComponent::ResolveAttribute(const FName& TargetStat)
{
	const FString Stat = TargetStat.ToString();
	if (Stat.Equals(TEXT("Char_Attack"), ESearchCase::IgnoreCase) || Stat.Equals(TEXT("Mon_Attack"), ESearchCase::IgnoreCase))
	{
		return ULSCharacterAttributeSet::GetAttackAttribute();
	}
	if (Stat.Equals(TEXT("Char_Atkspeed"), ESearchCase::IgnoreCase))
	{
		return ULSCharacterAttributeSet::GetAttackSpeedAttribute();
	}
	if (Stat.Equals(TEXT("Char_Speed"), ESearchCase::IgnoreCase) || Stat.Equals(TEXT("Mon_Speed"), ESearchCase::IgnoreCase))
	{
		return ULSCharacterAttributeSet::GetMoveSpeedAttribute();
	}
	if (Stat.Equals(TEXT("Char_Defence"), ESearchCase::IgnoreCase) || Stat.Equals(TEXT("Mon_Defence"), ESearchCase::IgnoreCase))
	{
		return ULSCharacterAttributeSet::GetDefenceAttribute();
	}

	// 자원 ID(예: "2002") 등 어트리뷰트로 매핑되지 않는 stat은 무효 어트리뷰트로 돌려보내 호출부가 스킵한다.
	return FGameplayAttribute();
}

void ULSStatusEffectComponent::GatherModifiers(const FLSStatusEffectRow& Row, TArray<FResolvedModifier>& OutModifiers) const
{
	auto AddModifier = [this, &OutModifiers](const FName& Stat, ELSStatusEffectMathType MathType, float Value)
	{
		if (MathType == ELSStatusEffectMathType::None || FMath::IsNearlyZero(Value))
		{
			return;
		}

		const FGameplayAttribute Attribute = ResolveAttribute(Stat);
		if (!Attribute.IsValid())
		{
			UE_LOG(LogLS, Warning, TEXT("StatusEffect: 매핑되지 않은 Target_Stat=%s (스킵). 자원/태그 stat은 아직 미지원."), *Stat.ToString());
			return;
		}

		OutModifiers.Add({Attribute, MathType, Value});
	};

	// Stat_Modifiers 배열을 단일 출처로 본다. CSV가 평면 컬럼(Target_Stat/_2)에 같은 값을 중복 보유하므로,
	// 배열이 있으면 평면 컬럼은 무시해 이중 집계를 막는다. 배열이 비었을 때만 평면 컬럼을 fallback으로 쓴다.
	if (Row.Stat_Modifiers.Num() > 0)
	{
		for (const FLSStatusEffectStatModifier& Modifier : Row.Stat_Modifiers)
		{
			AddModifier(Modifier.Target_Stat, Modifier.Math_Type, Modifier.Mod_Value);
		}
	}
	else
	{
		AddModifier(Row.Target_Stat, Row.Math_Type, Row.Mod_Value);
		AddModifier(Row.Target_Stat_2, Row.Math_Type_2, Row.Mod_Value_2);
	}
}

FActiveGameplayEffectHandle ULSStatusEffectComponent::BuildAndApplyEffect(
	UAbilitySystemComponent& ASC,
	const TArray<FResolvedModifier>& Modifiers,
	int32 StackCount,
	float Duration,
	AActor* Instigator,
	int32 StatusID) const
{
	const FName EffectName = MakeUniqueObjectName(
		GetTransientPackage(),
		UGameplayEffect::StaticClass(),
		FName(*FString::Printf(TEXT("LSStatusEffect_%d"), StatusID)));
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), EffectName);

	Effect->DurationPolicy = Duration > 0.0f
		? EGameplayEffectDurationType::HasDuration
		: EGameplayEffectDurationType::Infinite;
	if (Duration > 0.0f)
	{
		Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Duration));
	}

	for (const FResolvedModifier& Modifier : Modifiers)
	{
		FGameplayModifierInfo ModifierInfo;
		ModifierInfo.Attribute = Modifier.Attribute;

		if (Modifier.MathType == ELSStatusEffectMathType::Flat)
		{
			// 고정값은 스택만큼 합산한다.
			ModifierInfo.ModifierOp = EGameplayModOp::Additive;
			ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Modifier.Value * StackCount));
		}
		else
		{
			// 퍼센트는 (1 + 비율*스택) 배율로 곱연산한다. 음수면 디버프(예: -30% -> x0.7).
			const float Fraction = NormalizeModifierFraction(Modifier.Value);
			ModifierInfo.ModifierOp = EGameplayModOp::Multiplicitive;
			ModifierInfo.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(1.0f + Fraction * StackCount));
		}

		Effect->Modifiers.Add(ModifierInfo);
	}

	FGameplayEffectContextHandle EffectContext = ASC.MakeEffectContext();
	EffectContext.AddSourceObject(Instigator);

	FGameplayEffectSpec Spec(Effect, EffectContext, 1.0f);
	return ASC.ApplyGameplayEffectSpecToSelf(Spec);
}

bool ULSStatusEffectComponent::ApplyStatusEffectByID(int32 StatusID, AActor* Instigator, float DurationOverride)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	const ULSGameDataSubsystem* GameDataSubsystem = ResolveGameDataSubsystem(OwnerActor);
	const FLSStatusEffectRow* Row = GameDataSubsystem
		? GameDataSubsystem->FindStatusEffectRowByID(StatusID, TEXT("StatusEffectComponent.Apply"))
		: nullptr;
	if (!Row)
	{
		UE_LOG(LogLS, Warning, TEXT("StatusEffect: Status_ID=%d row를 찾지 못해 적용을 건너뜀."), StatusID);
		return false;
	}

	TArray<FResolvedModifier> Modifiers;
	GatherModifiers(*Row, Modifiers);
	if (Modifiers.Num() == 0)
	{
		// stat modifier가 없는 CC/Tag 그룹이거나 매핑되지 않은 stat뿐인 경우. 태그 부여 정책은 별도 확장.
		UE_LOG(LogLS, Verbose, TEXT("StatusEffect: Status_ID=%d 적용 가능한 stat modifier가 없어 건너뜀(Group=%d)."),
			StatusID, static_cast<int32>(Row->Status_Group));
		return false;
	}

	FLSActiveStatusEffect* Existing = ActiveStatusEffects.Find(StatusID);

	int32 NewStackCount = 1;
	switch (Row->Stack_Rule)
	{
	case ELSStatusEffectStackRule::None:
	case ELSStatusEffectStackRule::Ignore:
		// 이미 적용 중이면 무시(재적용/갱신 없음).
		if (Existing)
		{
			return false;
		}
		NewStackCount = 1;
		break;

	case ELSStatusEffectStackRule::Refresh:
		// 스택은 1로 유지하되 지속시간만 갱신한다.
		NewStackCount = 1;
		break;

	case ELSStatusEffectStackRule::Add:
		NewStackCount = Existing ? FMath::Min(Existing->StackCount + 1, FMath::Max(1, Row->Max_Stack)) : 1;
		break;

	default:
		NewStackCount = 1;
		break;
	}

	UWorld* World = GetWorld();

	// 기존 효과를 제거하고 새 스택/지속시간으로 재적용한다(동적 GE는 네이티브 스태킹을 쓰지 않음).
	if (Existing)
	{
		ASC->RemoveActiveGameplayEffect(Existing->EffectHandle);
		if (World)
		{
			World->GetTimerManager().ClearTimer(Existing->ExpiryTimerHandle);
		}
	}

	const FActiveGameplayEffectHandle Handle = BuildAndApplyEffect(*ASC, Modifiers, NewStackCount, DurationOverride, Instigator, StatusID);
	if (!Handle.IsValid())
	{
		ActiveStatusEffects.Remove(StatusID);
		return false;
	}

	FLSActiveStatusEffect& Entry = ActiveStatusEffects.FindOrAdd(StatusID);
	Entry.StackCount = NewStackCount;
	Entry.EffectHandle = Handle;

	if (DurationOverride > 0.0f && World)
	{
		World->GetTimerManager().ClearTimer(Entry.ExpiryTimerHandle);
		FTimerDelegate ExpiryDelegate = FTimerDelegate::CreateUObject(this, &ULSStatusEffectComponent::HandleStatusEffectExpired, StatusID);
		World->GetTimerManager().SetTimer(Entry.ExpiryTimerHandle, ExpiryDelegate, DurationOverride, false);
	}

	UE_LOG(LogLS, Log, TEXT("StatusEffect: %s에 Status_ID=%d 적용 (Stack=%d/%d, Duration=%.2f, Modifiers=%d)."),
		*GetNameSafe(OwnerActor), StatusID, NewStackCount, FMath::Max(1, Row->Max_Stack), DurationOverride, Modifiers.Num());

	return true;
}

bool ULSStatusEffectComponent::RemoveStatusEffectByID(int32 StatusID)
{
	FLSActiveStatusEffect* Existing = ActiveStatusEffects.Find(StatusID);
	if (!Existing)
	{
		return false;
	}

	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveActiveGameplayEffect(Existing->EffectHandle);
	}
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(Existing->ExpiryTimerHandle);
	}

	ActiveStatusEffects.Remove(StatusID);
	return true;
}

bool ULSStatusEffectComponent::HasStatusEffect(int32 StatusID) const
{
	return ActiveStatusEffects.Contains(StatusID);
}

void ULSStatusEffectComponent::HandleStatusEffectExpired(int32 StatusID)
{
	// GE 자체 지속시간이 어트리뷰트 modifier를 되돌린다. 여기서는 스택 부기만 정리한다.
	ActiveStatusEffects.Remove(StatusID);
}
