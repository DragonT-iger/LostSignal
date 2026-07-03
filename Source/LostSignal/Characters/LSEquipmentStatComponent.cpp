#include "Characters/LSEquipmentStatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Data/LSEquipmentStats.h"
#include "GAS/Effects/LSGE_EquipmentStats.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"

ULSEquipmentStatComponent::ULSEquipmentStatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	EquipmentStatEffectClass = ULSGE_EquipmentStats::StaticClass();
}

void ULSEquipmentStatComponent::BeginPlay()
{
	Super::BeginPlay();

	// 장비 장착 변경 시 자동 재적용하도록 구독. (초기 적용은 캐릭터 BeginPlay에서 ASC 준비 후 호출)
	if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
	{
		// 델리게이트 경유 갱신(장비 교체)은 체력을 회복시키지 않는다 — 레이드 중 교체가 회복 수단이 되지 않게.
		EquipmentChangedHandle = SaveSubsystem->OnEquipmentChanged.AddUObject(this, &ULSEquipmentStatComponent::RefreshEquipmentStats, false);
	}
}

void ULSEquipmentStatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (EquipmentChangedHandle.IsValid())
	{
		if (ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem())
		{
			SaveSubsystem->OnEquipmentChanged.Remove(EquipmentChangedHandle);
		}
		EquipmentChangedHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void ULSEquipmentStatComponent::RefreshEquipmentStats(bool bRestoreFullHealth)
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogLS, Warning, TEXT("[EquipStat] AbilitySystemComponent가 없어 장비 스탯을 적용할 수 없습니다. Owner=%s"), *GetNameSafe(OwnerActor));
		return;
	}

	if (!EquipmentStatEffectClass)
	{
		UE_LOG(LogLS, Warning, TEXT("[EquipStat] EquipmentStatEffectClass가 미설정이라 장비 스탯을 적용할 수 없습니다. Owner=%s"), *GetNameSafe(OwnerActor));
		return;
	}

	ULSSaveSubsystem* SaveSubsystem = GetSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("[EquipStat] SaveSubsystem이 없어 장비 스탯을 적용할 수 없습니다. Owner=%s"), *GetNameSafe(OwnerActor));
		return;
	}

	const FLSEquipmentStatTotals Totals = LSEquipmentStats::ComputeEquipmentStatTotals(SaveSubsystem->GetEquipmentSlots());

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EquipmentStatEffectClass, 1.0f, ContextHandle);
	if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid())
	{
		UE_LOG(LogLS, Warning, TEXT("[EquipStat] 장비 스탯 GE Spec 생성 실패. Owner=%s"), *GetNameSafe(OwnerActor));
		return;
	}

	FGameplayEffectSpec& Spec = *SpecHandle.Data;
	// 평탄 스탯은 값 그대로 가산.
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Equip_Attack, Totals.Attack);
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Equip_Health, Totals.Health);
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Equip_Defense, Totals.Defense);
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Equip_Recovery, Totals.Recovery);
	// 비율 스탯은 ÷100 환산 후 가산 (칩 스탯 환산 규칙과 동일).
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Equip_AttackSpeed, Totals.AttackSpeed / 100.0f);
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Equip_SkillHaste, Totals.SkillHaste / 100.0f);
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Equip_CritDamage, Totals.CritDamage / 100.0f);
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Equip_CritRate, Totals.CritRate / 100.0f);
	Spec.SetSetByCallerMagnitude(LSGameplayTags::Data_Equip_ArmorPenetration, Totals.DefensePenetration / 100.0f);

	const float PreviousHealth = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetCurrentHealthAttribute());

	if (EquipmentStatEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(EquipmentStatEffectHandle);
		EquipmentStatEffectHandle.Invalidate();
	}
	EquipmentStatEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(Spec);

	// 초기 적용(스폰 직후)만 풀피로 시작한다. 이후 갱신(장비 교체)은 기존 체력을 보존하고
	// 새 최대 체력으로 클램프만 해서, 교체가 회복 수단이 되지 않게 한다.
	const float NewMaxHealth = ASC->GetNumericAttribute(ULSCombatAttributeSet::GetMaxHealthAttribute());
	const float NewHealth = bRestoreFullHealth ? NewMaxHealth : FMath::Min(PreviousHealth, NewMaxHealth);
	ASC->SetNumericAttributeBase(ULSCombatAttributeSet::GetCurrentHealthAttribute(), NewHealth);
}

UAbilitySystemComponent* ULSEquipmentStatComponent::GetOwnerAbilitySystemComponent() const
{
	const IAbilitySystemInterface* AbilityOwner = Cast<IAbilitySystemInterface>(GetOwner());
	return AbilityOwner ? AbilityOwner->GetAbilitySystemComponent() : nullptr;
}

ULSSaveSubsystem* ULSEquipmentStatComponent::GetSaveSubsystem() const
{
	const UWorld* World = GetWorld();
	UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
}
