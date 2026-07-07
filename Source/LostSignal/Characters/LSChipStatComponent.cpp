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
		// 델리게이트 경유 갱신(장착 변경·신호 게이지 감소)은 체력을 회복시키지 않는다 — 레이드 중 풀피 방지.
		ChipLoadoutChangedHandle = SaveSubsystem->OnChipLoadoutChanged.AddUObject(this, &ULSChipStatComponent::RefreshChipStats, false);
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

void ULSChipStatComponent::RefreshChipStats(bool bRestoreFullHealth)
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
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_SkillHaste, PercentValue(TEXT("Chip_Skill_Haste")));
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Chip_ArmorPenetration, PercentValue(TEXT("Chip_Defense_Penetration")));

	const float PreviousHealth = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());
	const float PreviousMaxHealth = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetMaxHealthAttribute());

	if (ChipStatEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(ChipStatEffectHandle);
		ChipStatEffectHandle.Invalidate();
	}
	ChipStatEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	// 초기 적용(스폰 직후)만 풀피로 시작한다. 이후 갱신은 잃은 체력량을 보존한다 —
	// 최대 체력이 늘면 그 증가분만큼 현재 체력도 같이 올리고(신호 유실 보너스 반영),
	// 줄면 새 최대 체력으로 클램프만 해서 재적용이 회복 수단이 되지 않게 한다.
	const float NewMaxHealth = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetMaxHealthAttribute());
	const float MaxHealthGain = FMath::Max(0.0f, NewMaxHealth - PreviousMaxHealth);
	const float NewHealth = bRestoreFullHealth ? NewMaxHealth : FMath::Min(PreviousHealth + MaxHealthGain, NewMaxHealth);
	ASC->SetNumericAttributeBase(ULSCombatAttributeSet::GetCurrentHealthAttribute(), NewHealth);
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
