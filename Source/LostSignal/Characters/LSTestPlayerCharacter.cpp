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
	LoadStatsFromDataTable();
	ApplyStatsToAttributeSet();
}

#if WITH_EDITOR
void ALSTestPlayerCharacter::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (AbilitySystemComponent && CharacterAttributeSet)
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
		UE_LOG(LogLS, Log, TEXT("%s: DataTable 행 없음 → Base* 기본값 유지"), *GetNameSafe(this));
		return;
	}

	UE_LOG(LogLS, Log, TEXT("%s: DataTable 행 '%s' → Base* 덮어쓰기"), *GetNameSafe(this), *CharacterRowName.ToString());

	BaseAttack           = Row->Char_Attack;
	BaseAttackSpeed      = Row->Char_Atkspead;
	BaseCooldownReduction= Row->Char_Cal;
	BaseCritChance       = Row->Char_Crit;
	BaseCritDamage       = Row->Char_CritDmg;
	BaseArmorPenetration = Row->Char_ArmorPen;
	BaseHealth           = Row->Char_Health;
	BaseDefence          = Row->Char_Defence;
	BaseRecovery         = Row->Char_Recovery;
	BaseStamina          = Row->Char_Stamina;
	BaseMoveSpeed        = Row->Char_Speed;
	BaseDashSpeed        = Row->Char_DashSpeed;
}

void ALSTestPlayerCharacter::ApplyStatsToAttributeSet()
{
	UE_LOG(LogLS, Warning, TEXT("Player Initialize"));

	if (!AbilitySystemComponent || !CharacterAttributeSet)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: ASC 또는 AttributeSet이 없음"), *GetNameSafe(this));
		return;
	}

	CharacterAttributeSet->InitAttack(BaseAttack);
	CharacterAttributeSet->InitAttackSpeed(BaseAttackSpeed);
	CharacterAttributeSet->InitCooldownReduction(BaseCooldownReduction);
	CharacterAttributeSet->InitCritChance(BaseCritChance);
	CharacterAttributeSet->InitCritDamage(BaseCritDamage);
	CharacterAttributeSet->InitArmorPenetration(BaseArmorPenetration);
	CharacterAttributeSet->InitMaxHealth(BaseHealth);
	CharacterAttributeSet->InitCurrentHealth(BaseHealth);
	CharacterAttributeSet->InitDefence(BaseDefence);
	CharacterAttributeSet->InitRecovery(BaseRecovery);
	CharacterAttributeSet->InitMaxStamina(BaseStamina);
	CharacterAttributeSet->InitMoveSpeed(BaseMoveSpeed);
	CharacterAttributeSet->InitDashSpeed(BaseDashSpeed);
}
