#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "LSRatPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

/**
 * 미니게임 전용 입력 (03_Controls). 본편 IMC와 분리, 게임패드 미지원(키보드 전용).
 * IMC/IA 에셋이 미할당이면 원작 키(WASD·방향키 / Z / X / C / Esc)를 코드로 구성한다.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

	void HandleMove(const FInputActionValue& Value);
	void HandleSteal(const FInputActionValue& Value);
	void HandleSlotNext(const FInputActionValue& Value);
	void HandleThrow(const FInputActionValue& Value);
	void HandlePause(const FInputActionValue& Value);

	/** 결과 화면에서 Enter/Space → 본편 복귀 (버튼 없이도 루프 완결) */
	void HandleConfirm(const FInputActionValue& Value);

	/** 에셋이 비어 있을 때 런타임에서 원작 키 매핑을 생성 */
	void BuildDefaultInputAssets();

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Input")
	TObjectPtr<UInputMappingContext> RatMappingContext;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Input")
	TObjectPtr<UInputAction> StealAction;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Input")
	TObjectPtr<UInputAction> SlotNextAction;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Input")
	TObjectPtr<UInputAction> ThrowAction;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Input")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Input")
	TObjectPtr<UInputAction> ConfirmAction;

	/** 버리기 연속 입력 간격 (원작 throwTime 0.2s) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float ThrowInterval = 0.2f;

private:
	float LastThrowTime = -1.f;
};
