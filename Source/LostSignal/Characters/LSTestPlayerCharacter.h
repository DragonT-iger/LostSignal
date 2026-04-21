#pragma once

#include "CoreMinimal.h"
#include "Characters/LSPlayerCharacter.h"
#include "LSTestPlayerCharacter.generated.h"

class ULSCharacterAttributeSet;
struct FLSCharacterStatRow;

/**
 * 테스트 플레이어 캐릭터.
 * ALSPlayerCharacter를 상속하며 GAS 수치값을 오버라이드한다.
 *
 * 수치 우선순위:
 *   DataTable 행이 존재하면 → DataTable 값 사용
 *   행이 없으면            → Blueprint Details 패널의 기본값(Base* 프로퍼티) 사용
 */
UCLASS()
class LOSTSIGNAL_API ALSTestPlayerCharacter : public ALSPlayerCharacter
{
	GENERATED_BODY()

public:
	ALSTestPlayerCharacter();

	virtual void BeginPlay() override;

protected:
	// ── DataTable 연결 ────────────────────────────────────────────────────────
	// Content Browser에서 만든 DataTable 에셋과 조회할 행 이름을 할당
	UPROPERTY(EditDefaultsOnly, Category="Stats|DataTable")
	TObjectPtr<UDataTable> CharacterStatTable;

	UPROPERTY(EditDefaultsOnly, Category="Stats|DataTable")
	FName CharacterRowName;

	// ── Blueprint 기본값 (DataTable 행이 없을 때 사용) ──────────────────────
	// 아래 값은 PIE 중 Details 패널에서 실시간 편집 가능
	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="공격력")
	int32 BaseAttack = 100;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="공격속도")
	float BaseAttackSpeed = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="가속력(쿨타임)")
	float BaseCooldownReduction = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="치명타 확률")
	float BaseCritChance = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="치명타 배율")
	float BaseCritDamage = 1.5f;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="방관통")
	float BaseArmorPenetration = 0.0f;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="체력")
	int32 BaseHealth = 100;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="방어")
	int32 BaseDefence = 10;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="회복력")
	int32 BaseRecovery = 5;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="스태미나")
	int32 BaseStamina = 100;

	UPROPERTY(EditDefaultsOnly, Category="Stats|Default Values", DisplayName="이동속도")
	float BaseMoveSpeed = 1.0f;

private:
	UPROPERTY(VisibleAnywhere, Category="GAS")
	TObjectPtr<ULSCharacterAttributeSet> CharacterAttributeSet;

	const FLSCharacterStatRow* FindStatRow() const;
	void InitializeStats();
};
