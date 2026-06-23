#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "LSChipStatComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class ULSSaveSubsystem;

/**
 * 칩 장착 합산 전투 스탯을 소유 캐릭터의 GAS 어트리뷰트에 적용/갱신하는 컴포넌트.
 * SaveSubsystem(칩 장착 데이터 원본)을 실시간 참조해 무한 지속 GE(ULSGE_ChipStats)를
 * remove & reapply 방식으로 다시 적용한다. 서버 권한에서만 동작.
 */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSChipStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSChipStatComponent();

	// 현재 칩 장착/신호 게이지 상태를 읽어 전투 스탯 GE를 다시 적용한다.
	void RefreshChipStats();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 적용할 칩 스탯 GE. 기본값은 생성자에서 ULSGE_ChipStats로 설정.
	UPROPERTY(EditDefaultsOnly, Category="LS/Chip")
	TSubclassOf<UGameplayEffect> ChipStatEffectClass;

private:
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	ULSSaveSubsystem* GetSaveSubsystem() const;

	// 현재 적용 중인 칩 스탯 GE 핸들. 갱신 시 제거 후 재적용한다.
	FActiveGameplayEffectHandle ChipStatEffectHandle;

	// SaveSubsystem 변경 구독 해제용 핸들.
	FDelegateHandle ChipLoadoutChangedHandle;
};
