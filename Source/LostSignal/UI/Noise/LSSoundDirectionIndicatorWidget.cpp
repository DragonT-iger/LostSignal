#include "UI/Noise/LSSoundDirectionIndicatorWidget.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Image.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

void ULSSoundDirectionIndicatorWidget::InitializeSoundDirectionIndicator(APawn* InObservedPawn)
{
	ObservedPawn = InObservedPawn;
}

void ULSSoundDirectionIndicatorWidget::ShowSoundDirection(const FVector SoundWorldLocation, const float DurationSeconds, const float Strength)
{
	ShowSoundDirectionFromActor(nullptr, SoundWorldLocation, DurationSeconds, Strength);
}

void ULSSoundDirectionIndicatorWidget::ShowSoundDirectionFromActor(
	AActor* SoundSourceActor,
	const FVector FallbackSoundWorldLocation,
	const float DurationSeconds,
	const float Strength)
{
	ActiveSoundSourceActor = SoundSourceActor;
	ActiveSoundWorldLocation = SoundSourceActor ? SoundSourceActor->GetActorLocation() : FallbackSoundWorldLocation;
	ActiveDurationSeconds = FMath::Max(DurationSeconds, KINDA_SMALL_NUMBER);
	ActiveElapsedSeconds = 0.0f;
	ActiveStrength = FMath::Max(0.0f, Strength);
	bIndicatorActive = true;

	UE_LOG(LogLS, Warning, TEXT("[SoundIndicator] Show. Indicator=%s SourceActor=%s Location=%s Duration=%.2f Strength=%.2f Image=%s MaterialInstance=%s"),
		*GetNameSafe(this),
		*GetNameSafe(SoundSourceActor),
		*ActiveSoundWorldLocation.ToCompactString(),
		ActiveDurationSeconds,
		ActiveStrength,
		*GetNameSafe(IndicatorImage),
		*GetNameSafe(IndicatorMaterialInstance));

	if (IndicatorImage)
	{
		IndicatorImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	RefreshIndicatorMaterial(1.0f);
}

void ULSSoundDirectionIndicatorWidget::ShowNoiseEventDirection(const FLSNoiseEvent& NoiseEvent, const float DurationSeconds)
{
	const float Strength = NoiseEvent.RadiusCm > 0.0f ? NoiseEvent.RadiusCm : 1.0f;
	ShowSoundDirectionFromActor(NoiseEvent.NoiseInstigator, NoiseEvent.Location, DurationSeconds, Strength);
}

void ULSSoundDirectionIndicatorWidget::SetPreviewSoundDirectionParameters(
	const FVector2D CenterUV,
	const float DirectionAngle,
	const float AspectRatio,
	const float Opacity,
	const float Strength)
{
	bUsePreviewParameters = true;
	PreviewCenterUV = CenterUV;
	PreviewDirectionAngle = DirectionAngle;
	PreviewAspectRatio = FMath::Max(AspectRatio, KINDA_SMALL_NUMBER);
	PreviewOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	PreviewStrength = FMath::Max(0.0f, Strength);
	ApplyPreviewSoundDirectionParameters();
}

void ULSSoundDirectionIndicatorWidget::SetPreviewSoundDirectionAspectRatio(const float AspectRatio)
{
	PreviewAspectRatio = FMath::Max(AspectRatio, KINDA_SMALL_NUMBER);
}

void ULSSoundDirectionIndicatorWidget::ApplyPreviewSoundDirectionParameters()
{
	bUsePreviewParameters = true;
	bIndicatorActive = false;

	if (IndicatorImage)
	{
		IndicatorImage->SetVisibility(PreviewOpacity > 0.0f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	RefreshIndicatorMaterial(PreviewOpacity);
}

void ULSSoundDirectionIndicatorWidget::ClearPreviewSoundDirectionParameters()
{
	bUsePreviewParameters = false;
	HideSoundDirection();
}

void ULSSoundDirectionIndicatorWidget::HideSoundDirection()
{
	bIndicatorActive = false;
	ActiveElapsedSeconds = 0.0f;
	ActiveSoundSourceActor.Reset();

	if (IndicatorMaterialInstance)
	{
		IndicatorMaterialInstance->SetScalarParameterValue(OpacityParameterName, 0.0f);
	}

	if (IndicatorImage)
	{
		IndicatorImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ULSSoundDirectionIndicatorWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!IndicatorImage)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required sound direction widget binding: IndicatorImage."), *GetNameSafe(this));
		return;
	}

	InitializeIndicatorMaterial();
	if (bUsePreviewParameters)
	{
		ApplyPreviewSoundDirectionParameters();
	}
	else
	{
		HideSoundDirection();
	}
}

float ULSSoundDirectionIndicatorWidget::GetSoundDirectionRemainingTime() const
{
	return bIndicatorActive ? FMath::Max(ActiveDurationSeconds - ActiveElapsedSeconds, 0.0f) : 0.0f;
}

void ULSSoundDirectionIndicatorWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bUsePreviewParameters)
	{
		return;
	}

	if (!bIndicatorActive)
	{
		return;
	}

	ActiveElapsedSeconds += InDeltaTime;
	const float LifeAlpha = 1.0f - FMath::Clamp(ActiveElapsedSeconds / ActiveDurationSeconds, 0.0f, 1.0f);
	if (LifeAlpha <= 0.0f)
	{
		HideSoundDirection();
		return;
	}

	RefreshIndicatorMaterial(LifeAlpha);
}

void ULSSoundDirectionIndicatorWidget::InitializeIndicatorMaterial()
{
	if (!IndicatorImage)
	{
		return;
	}

	if (IndicatorMaterial)
	{
		IndicatorMaterialInstance = UMaterialInstanceDynamic::Create(IndicatorMaterial, this);
		IndicatorImage->SetBrushFromMaterial(IndicatorMaterialInstance);
		return;
	}

	IndicatorMaterialInstance = IndicatorImage->GetDynamicMaterial();
	if (!IndicatorMaterialInstance)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot initialize sound direction material. Set IndicatorMaterial or assign a material brush to IndicatorImage."),
			*GetNameSafe(this));
	}
}

void ULSSoundDirectionIndicatorWidget::RefreshIndicatorMaterial(const float Alpha)
{
	if (!IndicatorMaterialInstance)
	{
		return;
	}

	FVector2D CenterUV = FVector2D::ZeroVector;
	float DirectionAngle = 0.0f;
	float AspectRatio = 1.0f;
	if (!ResolveIndicatorParams(CenterUV, DirectionAngle, AspectRatio))
	{
		IndicatorMaterialInstance->SetScalarParameterValue(OpacityParameterName, 0.0f);
		return;
	}

	IndicatorMaterialInstance->SetVectorParameterValue(
		CenterUVParameterName,
		FLinearColor(CenterUV.X, CenterUV.Y, 0.0f, 0.0f));
	IndicatorMaterialInstance->SetScalarParameterValue(DirectionAngleParameterName, DirectionAngle);
	IndicatorMaterialInstance->SetScalarParameterValue(AspectRatioParameterName, AspectRatio);
	IndicatorMaterialInstance->SetScalarParameterValue(OpacityParameterName, Alpha);
	IndicatorMaterialInstance->SetScalarParameterValue(StrengthParameterName, ActiveStrength);
}

bool ULSSoundDirectionIndicatorWidget::ResolveIndicatorParams(FVector2D& OutCenterUV, float& OutDirectionAngle, float& OutAspectRatio) const
{
	if (bUsePreviewParameters)
	{
		OutCenterUV = PreviewCenterUV;
		OutDirectionAngle = FMath::DegreesToRadians(PreviewDirectionAngle);
		OutAspectRatio = FMath::Max(PreviewAspectRatio, KINDA_SMALL_NUMBER);
		return true;
	}

	APlayerController* PlayerController = GetOwningPlayer();
	const APawn* Pawn = ResolveObservedPawn();
	if (!PlayerController || !Pawn)
	{
		UE_LOG(LogLS, Warning, TEXT("[SoundIndicator] Resolve failed: missing controller or pawn. Indicator=%s PC=%s Pawn=%s"),
			*GetNameSafe(this),
			*GetNameSafe(PlayerController),
			*GetNameSafe(Pawn));
		return false;
	}

	FVector2D ListenerWidgetPosition = FVector2D::ZeroVector;
	FVector2D SoundWidgetPosition = FVector2D::ZeroVector;
	const FVector SoundWorldLocation = ActiveSoundSourceActor.IsValid()
		? ActiveSoundSourceActor->GetActorLocation()
		: ActiveSoundWorldLocation;
	const bool bProjectedListener = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PlayerController,
		Pawn->GetActorLocation(),
		ListenerWidgetPosition,
		true);
	const bool bProjectedSound = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(
		PlayerController,
		SoundWorldLocation,
		SoundWidgetPosition,
		true);
	if (!bProjectedListener || !bProjectedSound)
	{
		UE_LOG(LogLS, Warning, TEXT("[SoundIndicator] Resolve failed: projection. Indicator=%s ListenerProjected=%d SoundProjected=%d SoundLocation=%s"),
			*GetNameSafe(this),
			bProjectedListener,
			bProjectedSound,
			*SoundWorldLocation.ToCompactString());
		return false;
	}

	FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const float ViewportScale = FMath::Max(UWidgetLayoutLibrary::GetViewportScale(this), KINDA_SMALL_NUMBER);
	ViewportSize /= ViewportScale;
	if (ViewportSize.X <= KINDA_SMALL_NUMBER || ViewportSize.Y <= KINDA_SMALL_NUMBER)
	{
		UE_LOG(LogLS, Warning, TEXT("[SoundIndicator] Resolve failed: viewport size. Indicator=%s Size=%s Scale=%.2f"),
			*GetNameSafe(this),
			*ViewportSize.ToString(),
			ViewportScale);
		return false;
	}

	const FVector2D ToSound = SoundWidgetPosition - ListenerWidgetPosition;
	if (ToSound.IsNearlyZero())
	{
		UE_LOG(LogLS, Warning, TEXT("[SoundIndicator] Resolve failed: zero direction. Indicator=%s Listener=%s Sound=%s"),
			*GetNameSafe(this),
			*ListenerWidgetPosition.ToString(),
			*SoundWidgetPosition.ToString());
		return false;
	}

	OutAspectRatio = ViewportSize.X / ViewportSize.Y;
	OutCenterUV = FVector2D(
		ListenerWidgetPosition.X / ViewportSize.X,
		ListenerWidgetPosition.Y / ViewportSize.Y);
	OutDirectionAngle = FMath::Atan2(ToSound.X, -ToSound.Y);
	return true;
}

APawn* ULSSoundDirectionIndicatorWidget::ResolveObservedPawn() const
{
	if (ObservedPawn.IsValid())
	{
		return ObservedPawn.Get();
	}

	return GetOwningPlayerPawn();
}
