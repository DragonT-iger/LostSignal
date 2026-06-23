#include "Characters/LSChipStatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Data/LSChipStats.h"
#include "GAS/Effects/LSGE_ChipStats.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"

ULSChipStatComponent::ULSChipStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	ChipStatEffectClass = ULSGE_ChipStats::StaticClass();
}

void ULSChipStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// 칩 장착/신호 게이지 변경 시 자동 재적용하도록 구독. (초기 적용은 캐릭터 BeginPlay에서 ASC 준비 후 호출)
	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		ChipLoadoutChangedHandle = SaveSubsystem->OnChipLoadoutChanged.AddUObject(this, &ULSChipStatComponent::RefreshChipStats);
	}
}

void ULSChipStatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ChipLoadoutChangedHandle.IsValid())
	{
		if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
		{
			SaveSubsystem->OnChipLoadoutChanged.Remove(ChipLoadoutChangedHandle);
		}
		ChipLoadoutChangedHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void ULSChipStatComponent::RefreshChipStats()
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStat] AbilitySystemComponent가 없어 칩 스탯을 적용할 수 없습니다. Owner=%s"), *GetNameSafe(OwnerActor));
		return;
	}

	if (!ChipStatEffectClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStat] ChipStatEffectClass가 미설정이라 칩 스탯을 적용할 수 없습니다. Owner=%s"), *GetNameSafe(OwnerActor));
		return;
	}

	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStat] SaveSubsystem이 없어 칩 스탯을 적용할 수 없습니다. Owner=%s"), *GetNameSafe(OwnerActor));
		return;
	}

	const float SignalPercent = SaveSubsystem->GetChipSignalGaugePercent();
	const int32 InactiveSlotCount = LSChipStats::ResolveInactiveSignalSlotCount(SignalPercent);
	const TMap<FName, int32> Effective = LSChipStats::ComputeEffectiveChipStatTotals(SaveSubsystem->GetChipEquipmentSlots(), InactiveSlotCount);

	// 평탄 스탯은 값 그대로, 배율/비율 스탯은 ÷100 가산.
	auto FlatValue = [&Effective](const FName Key)
	{
		const int32* Value = Effective.Find(Key);
		return Value ? static_cast<float>(*Value) : 0.0f;
	};
	auto PercentValue = [&Effective](const FName Key)
	{
		const int32* Value = Effective.Find(Key);
		return Value ? static_cast<float>(*Value) / 100.0f : 0.0f;
	};

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ChipStatEffectClass, 1.0f, ContextHandle);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		UE_LOG(LogLS, Warning, TEXT("[ChipStat] 칩 스탯 GE Spec 생성 실패. Owner=%s"), *GetNameSafe(OwnerActor));
		return;
	}

	FGameplayEffectSpec& Spec = *SpecHandle.Data;
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_Attack, FlatValue(TEXT("Chip_Attack")));
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_Health, FlatValue(TEXT("Chip_Health")));
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_Defense, FlatValue(TEXT("Chip_Defense")));
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_Recovery, FlatValue(TEXT("Chip_Recovery")));
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_AttackSpeed, PercentValue(TEXT("Chip_Attack_Speed")));
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_MoveSpeed, PercentValue(TEXT("Chip_Move_Speed")));
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_CritDamage, PercentValue(TEXT("Chip_Critical_Damage")));
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_CritRate, PercentValue(TEXT("Chip_Critical_Rate")));

	// [DEBUG] 칩 스탯 적용 전 체력 상태 스냅샷.
	const float MaxHealthBefore = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetMaxHealthAttribute());
	const float CurrentHealthBefore = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());

	if (ChipStatEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ChipStatEffectHandle);
		ChipStatEffectHandle.Invalidate();
	}
	ChipStatEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	// [DEBUG] 칩 GE 재적용 직후(풀피 보정 전) 체력 상태.
	const float MaxHealthAfterGE = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetMaxHealthAttribute());
	const float CurrentHealthAfterGE = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());

	// 최초 1회 적용 시에만 늘어난 최대 체력에 맞춰 풀피로 시작한다. (갱신 시엔 현재 체력 유지)
	const bool bDidFullHeal = !bHasAppliedOnce;
	if (!bHasAppliedOnce)
	{
		bHasAppliedOnce = true;
		const float NewMaxHealth = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetMaxHealthAttribute());
		ASC->SetNumericAttributeBase(ULSCombatAttributeSet::GetCurrentHealthAttribute(), NewMaxHealth);
	}

	// [DEBUG] 칩 스탯 재적용 흐름 추적용. 체력 100 초기화 원인 진단 후 제거할 것.
	UE_LOG(LogLS, Warning,
		TEXT("[ChipStat][DEBUG] Owner=%s Signal=%.2f InactiveSlots=%d ChipHealth=%.0f | MaxHealth %.0f->%.0f CurrentHealth %.0f->%.0f | FirstApply(FullHeal)=%d -> CurrentHealthFinal=%.0f"),
		*GetNameSafe(OwnerActor),
		SignalPercent,
		InactiveSlotCount,
		FlatValue(TEXT("Chip_Health")),
		MaxHealthBefore, MaxHealthAfterGE,
		CurrentHealthBefore, CurrentHealthAfterGE,
		bDidFullHeal ? 1 : 0,
		ASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute()));
}

UAbilitySystemComponent* ULSChipStatComponent::GetOwnerAbilitySystemComponent() const
{
	const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(GetOwner());
	return AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
}

ULSSaveSubsystem* ULSChipStatComponent::GetSaveSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}
