#pragma once

#include "Characters/Enemys/LSEnemyCharacter.h"
#include "LSTrainingDummyCharacter.generated.h"

/** AI 없이 피격 테스트만 수행하며, 자체 Recovery로 회복하고 사망하지 않는 훈련용 허수아비. */
UCLASS()
class LOSTSIGNAL_API ALSTrainingDummyCharacter : public ALSEnemyCharacter
{
	GENERATED_BODY()

public:
	ALSTrainingDummyCharacter();

protected:
	virtual void BeginPlay() override;

	/** 초당 회복 체력(HP/s). DT_MonsterStat 스키마와 분리된 허수아비 전용 수치다. */
	UPROPERTY(EditDefaultsOnly, Category="LS/TrainingDummy", meta=(ClampMin="0.0"))
	float RecoveryPerSecond = 1000.0f;

private:
	void InitializeTrainingDummyRecovery();
};
