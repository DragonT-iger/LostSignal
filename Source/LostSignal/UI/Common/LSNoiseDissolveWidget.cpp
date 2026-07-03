#include "UI/Common/LSNoiseDissolveWidget.h"

#include "Components/RetainerBox.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"

void ULSNoiseDissolveWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitializeDissolveMaterial();
	ApplyDissolveAmount(0.0f);
}

void ULSNoiseDissolveWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bDissolvingOut)
	{
		return;
	}

	DissolveElapsedSeconds += InDeltaTime;
	const float Amount = FMath::Clamp(DissolveElapsedSeconds / DissolveOutDuration, 0.0f, 1.0f);
	ApplyDissolveAmount(Amount);

	if (Amount >= 1.0f)
	{
		FinishDissolveOut();
	}
}

void ULSNoiseDissolveWidget::StartDissolveOut()
{
	if (bDissolvingOut)
	{
		return;
	}

	// 연출 머티리얼이 없으면(리테이너/머티리얼 미할당) 기존 동작대로 즉시 사라지게 한다.
	if (!DissolveMaterialInstance)
	{
		FinishDissolveOut();
		return;
	}

	bDissolvingOut = true;
	DissolveElapsedSeconds = 0.0f;
	ApplyDissolveAmount(0.0f);

	// 연출 중에는 입력을 받지 않도록 하되(자식 포함), Collapsed 전까지는 계속 그려지고 틱한다.
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void ULSNoiseDissolveWidget::ResetDissolve()
{
	bDissolvingOut = false;
	DissolveElapsedSeconds = 0.0f;
	ApplyDissolveAmount(0.0f);
}

void ULSNoiseDissolveWidget::InitializeDissolveMaterial()
{
	if (!DissolveRetainer)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: DissolveRetainer(RetainerBox)가 바인딩되지 않아 노이즈 소멸 연출을 건너뜁니다."), *GetNameSafe(this));
		return;
	}

	// RetainerBox가 디자이너에 지정된 이펙트 머티리얼로부터 만든 다이나믹 인스턴스를 돌려준다.
	DissolveMaterialInstance = DissolveRetainer->GetEffectMaterial();
	if (!DissolveMaterialInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("%s: DissolveRetainer에 이펙트 머티리얼이 지정되지 않아 노이즈 소멸 연출을 건너뜁니다."), *GetNameSafe(this));
	}
}

void ULSNoiseDissolveWidget::ApplyDissolveAmount(const float Amount)
{
	if (DissolveMaterialInstance)
	{
		DissolveMaterialInstance->SetScalarParameterValue(DissolveAmountParameterName, Amount);
	}
}

void ULSNoiseDissolveWidget::FinishDissolveOut()
{
	bDissolvingOut = false;
	DissolveElapsedSeconds = 0.0f;
	ApplyDissolveAmount(1.0f);
	SetVisibility(ESlateVisibility::Collapsed);
	OnDissolveFinished.Broadcast();
}
