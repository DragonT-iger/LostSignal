#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LSNoiseDissolveWidget.generated.h"

class URetainerBox;
class UMaterialInstanceDynamic;

// 소멸 연출(좌우 노이즈 글리치)이 끝난 시점에 브로드캐스트. 실제 후처리(블러 토글 등)는 이 시점에 건다.
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FLSNoiseDissolveFinished);

// 좌우 노이즈로 사라지는 소멸 연출을 제공하는 공용 위젯 베이스.
// WBP에서 루트 콘텐츠를 RetainerBox(이름 DissolveRetainer)로 감싸고, 그 이펙트 머티리얼에
// 스칼라 파라미터 DissolveAmount(0=정상, 1=완전 소멸)를 노출한 머티리얼을 지정한다.
// StartDissolveOut()을 호출하면 DissolveAmount를 0->1로 구동하고, 끝나면 스스로 Collapsed 후 OnDissolveFinished.
UCLASS(Abstract, BlueprintType, Blueprintable)
class LOSTSIGNAL_API ULSNoiseDissolveWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// 소멸 연출 시작. 이미 진행 중이면 무시. 리테이너/머티리얼 미할당이면 경고 후 즉시 완료 처리(Collapsed + 델리게이트).
	UFUNCTION(BlueprintCallable, Category="LS/UI|Dissolve")
	void StartDissolveOut();

	// 표시 재개 시 DissolveAmount를 0으로 되돌리고 진행 상태를 초기화한다. 진행 중이던 연출도 취소된다.
	UFUNCTION(BlueprintCallable, Category="LS/UI|Dissolve")
	void ResetDissolve();

	bool IsDissolvingOut() const { return bDissolvingOut; }

	// 소멸 연출 완료(스스로 Collapsed 처리 직후) 시점에 브로드캐스트.
	UPROPERTY(BlueprintAssignable, Category="LS/UI|Dissolve")
	FLSNoiseDissolveFinished OnDissolveFinished;

protected:
	// 아트가 WBP 루트 콘텐츠를 감싸는 RetainerBox. 이펙트 머티리얼에 DissolveAmount 스칼라 파라미터가 필요하다.
	UPROPERTY(meta=(BindWidget), BlueprintReadOnly, Category="LS/UI|Dissolve")
	TObjectPtr<URetainerBox> DissolveRetainer;

	// 리테이너 이펙트 머티리얼의 소멸량 스칼라 파라미터 이름(0=정상, 1=완전 소멸).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Dissolve")
	FName DissolveAmountParameterName = TEXT("DissolveAmount");

	// 소멸 연출 지속 시간(초).
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Dissolve", meta=(ClampMin="0.01"))
	float DissolveOutDuration = 0.35f;

private:
	void InitializeDissolveMaterial();
	void ApplyDissolveAmount(float Amount);
	void FinishDissolveOut();

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DissolveMaterialInstance;

	float DissolveElapsedSeconds = 0.0f;
	bool bDissolvingOut = false;
};
