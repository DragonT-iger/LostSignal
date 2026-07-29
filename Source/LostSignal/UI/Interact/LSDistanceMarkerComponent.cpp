#include "UI/Interact/LSDistanceMarkerComponent.h"

#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "UI/Interact/LSDistanceMarkerWidget.h"
#include "UI/Interact/LSInteractMarkerSettings.h"

ULSDistanceMarkerComponent::ULSDistanceMarkerComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);

	// 기본은 감지 off. BeginPlay에서 마커가 유효할 때만 콜리전을 켠다.
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SetGenerateOverlapEvents(false);
}

void ULSDistanceMarkerComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!IsMarkerFeatureEnabled())
	{
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetComponentTickEnabled(false);
		return;
	}

	// 위젯 클래스·머티리얼은 전역 설정에서 받아 쓴다. 클래스 미설정이면 마커를 켤 수 없다.
	if (const ULSInteractMarkerSettings* Settings = GetDefault<ULSInteractMarkerSettings>())
	{
		ResolvedMarkerWidgetClass = Settings->DistanceMarkerWidgetClass.LoadSynchronous();
		ResolvedMarkerMaterial = Settings->DistanceMarkerWidgetMaterial.LoadSynchronous();
	}
	if (!ResolvedMarkerWidgetClass)
	{
		if (!bWarnedMissingWidgetClass)
		{
			UE_LOG(LogLS, Warning, TEXT("%s: 거리 마커가 켜졌지만 LS Interact Marker Settings의 DistanceMarkerWidgetClass가 비어 있어 표시할 수 없습니다."),
				*GetNameSafe(GetOwner()));
			bWarnedMissingWidgetClass = true;
		}
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SetComponentTickEnabled(false);
		return;
	}

	// 정적 박스 + 탑다운 고정 카메라라 매 프레임 갱신이 불필요하므로 주기적으로만 돈다.
	PrimaryComponentTick.TickInterval = MarkerUpdateInterval;

	ConfigureDetectionCollision();
	CreateWidgetComponent();
	SetComponentTickEnabled(false);
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

	if (!MarkerWidgetComponent)
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

bool ULSDistanceMarkerComponent::IsMarkerFeatureEnabled() const
{
	// 데디케이티드 서버거나 이 오브젝트가 마커를 끈 상태면 기능 전체를 끈다(무비용).
	const UWorld* World = GetWorld();
	const bool bIsDedicatedServer = World && World->GetNetMode() == NM_DedicatedServer;
	return !bIsDedicatedServer && bEnableMarker;
}

void ULSDistanceMarkerComponent::ConfigureDetectionCollision()
{
	// 캐릭터 MarkerActivationSphere(ECC_WorldDynamic)만 이 콜라이더를 Overlap하도록 InteractMarker 채널로 둔다.
	SetSphereRadius(DetectionRadius);
	SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SetCollisionObjectType(ECC_GameTraceChannel1);
	SetCollisionResponseToAllChannels(ECR_Ignore);
	SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	SetGenerateOverlapEvents(true);
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
	// 카메라를 향해 도는 빌보드라 뒷면은 안 보인다. 평면 노멀(+X)이 항상 카메라를 향하므로 단면으로 충분.
	MarkerWidgetComponent->SetTwoSided(false);
	MarkerWidgetComponent->SetRelativeLocation(WidgetOffset);
	MarkerWidgetComponent->SetWidgetClass(ResolvedMarkerWidgetClass);
	MarkerWidgetComponent->InitWidget();

	// 뎁스 테스트 off 등 커스텀 렌더가 필요하면 바깥 쿼드 머티리얼을 교체한다(SlateUI 렌더타깃 바인딩은 유지됨).
	if (ResolvedMarkerMaterial)
	{
		MarkerWidgetComponent->SetMaterial(0, ResolvedMarkerMaterial);
	}

	MarkerWidgetComponent->SetHiddenInGame(true);
}

void ULSDistanceMarkerComponent::SetActivatedByProximity(const bool bInActivated)
{
	if (bActivatedByProximity == bInActivated)
	{
		return;
	}

	bActivatedByProximity = bInActivated;
	RefreshActiveState();
}

void ULSDistanceMarkerComponent::SetMarkerSuppressed(const bool bInSuppressed)
{
	bSuppressed = bInSuppressed;

	// 억제되면 감지 콜리전까지 꺼 캐릭터 스피어가 다시 활성화하지 못하게 한다(예: 열린 룻박스).
	if (bSuppressed && GetCollisionEnabled() != ECollisionEnabled::NoCollision)
	{
		SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	RefreshActiveState();
}

void ULSDistanceMarkerComponent::RefreshActiveState()
{
	const bool bActive = bActivatedByProximity && !bSuppressed && MarkerWidgetComponent != nullptr;
	SetComponentTickEnabled(bActive);
	if (!bActive)
	{
		SetMarkerVisible(false);
	}
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
