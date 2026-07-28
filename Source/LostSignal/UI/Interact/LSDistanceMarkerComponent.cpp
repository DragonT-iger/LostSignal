#include "UI/Interact/LSDistanceMarkerComponent.h"

#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "UI/Interact/LSDistanceMarkerWidget.h"

ULSDistanceMarkerComponent::ULSDistanceMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(false);
}

void ULSDistanceMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 데디케이티드 서버나 위젯 클래스 미지정이면 마커 기능 전체를 끈다(무비용).
	const UWorld* World = GetWorld();
	const bool bIsDedicatedServer = World && World->GetNetMode() == NM_DedicatedServer;
	if (bIsDedicatedServer || !MarkerWidgetClass)
	{
		SetComponentTickEnabled(false);
		return;
	}

	// 정적 박스 + 탑다운 고정 카메라라 매 프레임 갱신이 불필요하므로 주기적으로만 돈다.
	PrimaryComponentTick.TickInterval = MarkerUpdateInterval;

	CreateWidgetComponent();
	SetMarkerVisible(false);
}

void ULSDistanceMarkerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (MarkerWidgetComponent)
	{
		MarkerWidgetComponent->DestroyComponent();
		MarkerWidgetComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ULSDistanceMarkerComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MarkerWidgetComponent || bSuppressed)
	{
		SetMarkerVisible(false);
		return;
	}

	APlayerController* LocalPlayerController = FindLocalPlayerController();
	if (!LocalPlayerController)
	{
		SetMarkerVisible(false);
		return;
	}

	UpdateMarker(LocalPlayerController);
}

void ULSDistanceMarkerComponent::CreateWidgetComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	MarkerWidgetComponent = NewObject<UWidgetComponent>(Owner, TEXT("DistanceMarkerWidgetComponent"));
	if (!MarkerWidgetComponent)
	{
		return;
	}

	MarkerWidgetComponent->SetupAttachment(this);
	MarkerWidgetComponent->RegisterComponent();
	ConfigureWidgetComponent();
}

void ULSDistanceMarkerComponent::ConfigureWidgetComponent()
{
	if (!MarkerWidgetComponent)
	{
		return;
	}

	MarkerWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerWidgetComponent->SetGenerateOverlapEvents(false);
	MarkerWidgetComponent->SetWidgetSpace(EWidgetSpace::World);
	MarkerWidgetComponent->SetDrawSize(DrawSize);
	MarkerWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	MarkerWidgetComponent->SetTwoSided(true);
	MarkerWidgetComponent->SetRelativeLocation(WidgetOffset);
	MarkerWidgetComponent->SetWidgetClass(MarkerWidgetClass);
	MarkerWidgetComponent->InitWidget();
	MarkerWidgetComponent->SetHiddenInGame(true);
}

void ULSDistanceMarkerComponent::UpdateMarker(APlayerController* LocalPlayerController)
{
	APlayerCameraManager* CameraManager = LocalPlayerController->PlayerCameraManager;
	if (!CameraManager)
	{
		SetMarkerVisible(false);
		return;
	}

	const FVector MarkerLocation = MarkerWidgetComponent->GetComponentLocation();
	const FVector CameraLocation = CameraManager->GetCameraLocation();

	// 거리 게이팅은 캐릭터 기준(없으면 카메라)으로 판단해 게임플레이 감각과 일치시킨다.
	const APawn* LocalPawn = LocalPlayerController->GetPawn();
	const FVector ReferenceLocation = LocalPawn ? LocalPawn->GetActorLocation() : CameraLocation;
	const float Distance = FVector::Dist(ReferenceLocation, MarkerLocation);

	if (Distance > MaxVisibleDistance)
	{
		SetMarkerVisible(false);
		return;
	}

	SetMarkerVisible(true);

	// Ratio: 0=FadeInDistance 이하(근거리), 1=MaxVisibleDistance(원거리).
	const float FadeRange = MaxVisibleDistance - FadeInDistance;
	const float Ratio = FadeRange > KINDA_SMALL_NUMBER
		? FMath::Clamp((Distance - FadeInDistance) / FadeRange, 0.0f, 1.0f)
		: 0.0f;
	const float Opacity = 1.0f - Ratio;
	const float Scale = FMath::Lerp(NearScale, FarScale, Ratio);

	if (UUserWidget* UserWidget = MarkerWidgetComponent->GetUserWidgetObject())
	{
		UserWidget->SetRenderOpacity(Opacity);
		if (ULSDistanceMarkerWidget* MarkerWidget = Cast<ULSDistanceMarkerWidget>(UserWidget))
		{
			MarkerWidget->OnDistanceRatioUpdated(Ratio);
		}
	}

	MarkerWidgetComponent->SetRelativeScale3D(FVector(Scale));

	if (bFaceCamera)
	{
		const FVector ToCamera = (CameraLocation - MarkerLocation).GetSafeNormal();
		if (!ToCamera.IsNearlyZero())
		{
			MarkerWidgetComponent->SetWorldRotation(ToCamera.Rotation());
		}
	}
}

void ULSDistanceMarkerComponent::SetMarkerVisible(const bool bShouldBeVisible)
{
	if (bMarkerVisible == bShouldBeVisible)
	{
		return;
	}

	bMarkerVisible = bShouldBeVisible;
	if (MarkerWidgetComponent)
	{
		MarkerWidgetComponent->SetHiddenInGame(!bShouldBeVisible);
		MarkerWidgetComponent->SetVisibility(bShouldBeVisible);
	}
}

void ULSDistanceMarkerComponent::SetMarkerSuppressed(const bool bInSuppressed)
{
	bSuppressed = bInSuppressed;
	if (bSuppressed)
	{
		SetMarkerVisible(false);
	}

	// 억제되면 틱까지 멈춰 완전 idle로 만든다(예: 열린 룻박스). 위젯이 없으면 애초에 비활성 유지.
	SetComponentTickEnabled(!bSuppressed && MarkerWidgetComponent != nullptr);
}

APlayerController* ULSDistanceMarkerComponent::FindLocalPlayerController() const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController->IsLocalPlayerController())
		{
			return PlayerController;
		}
	}
	return nullptr;
}
