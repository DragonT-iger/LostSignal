#include "Characters/LSTestPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Data/LSCharacterStatRow.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSCombatAttributeSet.h"
#include "LostSignal.h"

ALSTestPlayerCharacter::ALSTestPlayerCharacter()
{
}

void ALSTestPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	LoadStatsFromDataTable();
	ApplyStatsToAttributeSet();
}

#if WITH_EDITOR
void ALSTestPlayerCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (AbilitySystemComponent && GetPlayerAttributeSet() && GetCombatAttributeSet())
	{
		ApplyStatsToAttributeSet();
	}
}
#endif

const FLSCharacterStatRow* ALSTestPlayerCharacter::FindStatRow() const
{
	if (!CharacterStatTable || CharacterRowName.IsNone())
	{
		return nullptr;
	}

	return CharacterStatTable->FindRow<FLSCharacterStatRow>(CharacterRowName, TEXT("LSTestPlayerCharacter"));
}

void ALSTestPlayerCharacter::LoadStatsFromDataTable()
{
	const FLSCharacterStatRow* Row = FindStatRow();
	if (!Row)
	{
		UE_LOG(LogLS, Log, TEXT("%s: missing stat row, using defaults."), *GetNameSafe(this));
		return;
	}

	BaseAttack = Row->Char_Attack;
	BaseAttackSpeed = Row->Char_Atkspeed;
	BaseCooldownReduction = Row->Char_Cal;
	BaseCritChance = Row->Char_Crit;
	BaseCritDamage = Row->Char_CritDmg;
	BaseArmorPenetration = Row->Char_ArmorPen;
	BaseHealth = Row->Char_Health;
	BaseDefence = Row->Char_Defence;
	BaseRecovery = Row->Char_Recovery;
	BaseStamina = Row->Char_Stamina;
	BaseMoveSpeed = Row->Char_Speed;
	BaseDashSpeed = Row->Char_DashSpeed;
	BaseDashDuration = Row->Char_DashDuration;
	BaseDashCooldown = Row->Char_DashCooldown;
}

void ALSTestPlayerCharacter::ApplyStatsToAttributeSet()
{
	ULSCharacterAttributeSet* LocalPlayerAttributeSet = GetPlayerAttributeSet();
	ULSCombatAttributeSet* LocalCombatAttributeSet = GetCombatAttributeSet();
	if (!AbilitySystemComponent || !LocalPlayerAttributeSet || !LocalCombatAttributeSet)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: missing ASC or attribute set."), *GetNameSafe(this));
		return;
	}

	LocalPlayerAttributeSet->InitAttack(BaseAttack);
	LocalPlayerAttributeSet->InitAttackSpeed(BaseAttackSpeed);
	LocalPlayerAttributeSet->InitCooldownReduction(BaseCooldownReduction);
	LocalPlayerAttributeSet->InitCritChance(BaseCritChance);
	LocalPlayerAttributeSet->InitCritDamage(BaseCritDamage);
	LocalPlayerAttributeSet->InitArmorPenetration(BaseArmorPenetration);
	LocalPlayerAttributeSet->InitDefence(BaseDefence);
	LocalPlayerAttributeSet->InitRecovery(BaseRecovery);
	LocalPlayerAttributeSet->InitMaxStamina(BaseStamina);
	LocalPlayerAttributeSet->InitMoveSpeed(BaseMoveSpeed);
	LocalPlayerAttributeSet->InitDashSpeed(BaseDashSpeed);
	LocalPlayerAttributeSet->InitDashDuration(BaseDashDuration);
	LocalPlayerAttributeSet->InitDashCooldown(BaseDashCooldown);

	LocalCombatAttributeSet->InitMaxHealth(BaseHealth);
	LocalCombatAttributeSet->InitCurrentHealth(BaseHealth);
}
