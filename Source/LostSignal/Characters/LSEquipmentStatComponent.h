#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "LSEquipmentStatComponent.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class ULSSaveSubsystem;

/**
 * 장착 무기/방어구 합산 전투 스탯을 소유 캐릭터의 GAS 어트리뷰트에 적용/갱신하는 컴포넌트.
 * SaveSubsystem(장비 장착 데이터 원본)을 실시간 참조해 무한 지속 GE(ULSGE_EquipmentStats)를
 * remove & reapply 방식으로 다시 적용한다. 서버 권한에서만 동작.
 */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSEquipmentStatComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSEquipmentStatComponent();

	// 현재 장비 장착 상태를 읽어 전투 스탯 GE를 다시 적용한다.
	// bRestoreFullHealth: true면 현재 체력을 새 최대 체력으로 채운다(캐릭터 스폰 직후 초기 적용 전용).
	// false면 기존 체력을 보존하되 새 최대 체력으로 클램프만 한다(레이드 중 장비 교체 등).
	void RefreshEquipmentStats(bool bRestoreFullHealth);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// 적용할 장비 스탯 GE. 기본값은 생성자에서 ULSGE_EquipmentStats로 설정.
	UPROPERTY(EditDefaultsOnly, Category="LS/Equipment")
	TSubclassOf<UGameplayEffect> EquipmentStatEffectClass;

private:
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;
	ULSSaveSubsystem* GetSaveSubsystem() const;

	// 현재 적용 중인 장비 스탯 GE 핸들. 갱신 시 제거 후 재적용한다.
	FActiveGameplayEffectHandle EquipmentStatEffectHandle;

	// SaveSubsystem 변경 구독 해제용 핸들.
	FDelegateHandle EquipmentChangedHandle;
};
