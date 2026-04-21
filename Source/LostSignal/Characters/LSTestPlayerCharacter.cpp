#include "Characters/LSTestPlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "Data/LSCharacterStatRow.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "LostSignal.h"

ALSTestPlayerCharacter::ALSTestPlayerCharacter()
{
	// AttributeSet을 서브오브젝트로 생성 → ASC가 InitAbilityActorInfo 시 자동 등록
	CharacterAttributeSet = CreateDefaultSubobject<ULSCharacterAttributeSet>(TEXT("CharacterAttributeSet"));
}

void ALSTestPlayerCharacter::BeginPlay()
{
	Super::BeginPlay(); // LSCharacterBase::BeginPlay → InitAbilityActorInfo 호출
	InitializeStats();
}

const FLSCharacterStatRow* ALSTestPlayerCharacter::FindStatRow() const
{
	if (!CharacterStatTable || CharacterRowName.IsNone())
	{
		return nullptr;
	}
	return CharacterStatTable->FindRow<FLSCharacterStatRow>(CharacterRowName, TEXT("LSTestPlayerCharacter"));
}

void ALSTestPlayerCharacter::InitializeStats()
{
	if (!AbilitySystemComponent || !CharacterAttributeSet)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: ASC 또는 AttributeSet이 없음"), *GetNameSafe(this));
		return;
	}

	const FLSCharacterStatRow* Row = FindStatRow();
	if (Row)
	{
		UE_LOG(LogLS, Log, TEXT("%s: DataTable 행 '%s' 로드 성공"), *GetNameSafe(this), *CharacterRowName.ToString());

		CharacterAttributeSet->InitAttack(Row->Char_Attack);
		CharacterAttributeSet->InitAttackSpeed(Row->Char_Atkspead);
		CharacterAttributeSet->InitCooldownReduction(Row->Char_Cal);
		CharacterAttributeSet->InitCritChance(Row->Char_Crit);
		CharacterAttributeSet->InitCritDamage(Row->Char_CritDmg);
		CharacterAttributeSet->InitArmorPenetration(Row->Char_ArmorPen);
		CharacterAttributeSet->InitMaxHealth(Row->Char_Health);
		CharacterAttributeSet->InitDefence(Row->Char_Defence);
		CharacterAttributeSet->InitRecovery(Row->Char_Recovery);
		CharacterAttributeSet->InitMaxStamina(Row->Char_Stamina);
		CharacterAttributeSet->InitMoveSpeed(Row->Char_Speed);
	}
	else
	{
		UE_LOG(LogLS, Log, TEXT("%s: DataTable 없음 → Blueprint 기본값 사용"), *GetNameSafe(this));

		CharacterAttributeSet->InitAttack(BaseAttack);
		CharacterAttributeSet->InitAttackSpeed(BaseAttackSpeed);
		CharacterAttributeSet->InitCooldownReduction(BaseCooldownReduction);
		CharacterAttributeSet->InitCritChance(BaseCritChance);
		CharacterAttributeSet->InitCritDamage(BaseCritDamage);
		CharacterAttributeSet->InitArmorPenetration(BaseArmorPenetration);
		CharacterAttributeSet->InitMaxHealth(BaseHealth);
		CharacterAttributeSet->InitDefence(BaseDefence);
		CharacterAttributeSet->InitRecovery(BaseRecovery);
		CharacterAttributeSet->InitMaxStamina(BaseStamina);
		CharacterAttributeSet->InitMoveSpeed(BaseMoveSpeed);
	}
}
