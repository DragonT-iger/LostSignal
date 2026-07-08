#include "Gameplay/LSExtractionZone.h"
#include "Core/LSFarmingGameMode.h"
#include "Characters/LSPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "LostSignal.h"
#include "Minimap/LSMinimapMarkerComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LSExtractionZone"

ALSExtractionZone::ALSExtractionZone()
{
	// 링 상승·라이트 펄스 애니메이션 때문에 틱을 켠다(서버/클라 공용 시각 효과라 권한 무관).
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	ExtractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionBox"));
	SetRootComponent(ExtractionBox);
	ExtractionBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	ExtractionBox->SetCollisionProfileName(TEXT("Trigger"));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(ExtractionBox);
	MarkerMesh->SetRelativeLocation(FVector(0.f, 0.f, -195.f));
	// 트리거(반경 ~200)에 맞춰 바닥 원반을 키운다 — 위에서 봐도 발판 범위가 한눈에 들어오게.
	MarkerMesh->SetRelativeScale3D(FVector(4.0f, 4.0f, 0.1f));
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetGenerateOverlapEvents(false);
	MarkerMesh->SetCustomDepthStencilValue(252);
	MarkerMesh->SetRenderCustomDepth(true);
	if (CylinderMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CylinderMesh.Object);
	}

	// 바닥에서 위로 솟아오르며 퍼지는 링들(연기 상승 임시 표현). 실제 위치·스케일은 Tick이 순환 구동한다.
	RisingRings.Reserve(RisingRingCount);
	for (int32 RingIndex = 0; RingIndex < RisingRingCount; ++RingIndex)
	{
		const FName RingName(*FString::Printf(TEXT("RisingRing_%d"), RingIndex));
		UStaticMeshComponent* Ring = CreateDefaultSubobject<UStaticMeshComponent>(RingName);
		Ring->SetupAttachment(ExtractionBox);
		Ring->SetRelativeScale3D(FVector(1.2f, 1.2f, 0.06f));
		Ring->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Ring->SetGenerateOverlapEvents(false);
		Ring->SetCustomDepthStencilValue(252);
		Ring->SetRenderCustomDepth(true);
		if (CylinderMesh.Succeeded())
		{
			Ring->SetStaticMesh(CylinderMesh.Object);
		}
		RisingRings.Add(Ring);
	}

	SmokeEffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SmokeEffectComponent"));
	SmokeEffectComponent->SetupAttachment(ExtractionBox);
	SmokeEffectComponent->SetRelativeLocation(FVector(0.f, 0.f, -195.f));
	// 에셋은 아트가 BP에서 매핑한다. 미할당이면 아무것도 스폰하지 않으니 임시 링 애니메이션만 보인다.
	SmokeEffectComponent->SetAutoActivate(false);

	MarkerText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MarkerText"));
	MarkerText->SetupAttachment(ExtractionBox);
	MarkerText->SetRelativeLocation(FVector(0.f, 0.f, 260.f));
	MarkerText->SetRelativeRotation(FRotator(60.f, 180.f, 0.f));
	MarkerText->SetRelativeScale3D(FVector(3.f, 3.f, 3.f));
	MarkerText->SetHorizontalAlignment(EHTA_Center);
	MarkerText->SetVerticalAlignment(EVRTA_TextCenter);
	MarkerText->SetText(LOCTEXT("ExtractionMarkerText", "EXTRACTION"));
	MarkerText->SetTextRenderColor(FColor(80, 255, 120));
	MarkerText->SetWorldSize(56.f);
	MarkerText->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MarkerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MarkerLight"));
	MarkerLight->SetupAttachment(ExtractionBox);
	MarkerLight->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	MarkerLight->SetLightColor(FLinearColor(0.2f, 1.f, 0.35f));
	MarkerLight->SetIntensity(1200.f);
	MarkerLight->SetAttenuationRadius(450.f);

	MinimapMarkerComponent = CreateDefaultSubobject<ULSMinimapMarkerComponent>(TEXT("MinimapMarkerComponent"));
	MinimapMarkerComponent->SetMarkerType(ELSMinimapMarkerType::Extraction);
	MinimapMarkerComponent->SetMarkerColor(FLinearColor(0.28f, 1.0f, 0.45f, 1.0f));
}

void ALSExtractionZone::BeginPlay()
{
	Super::BeginPlay();
	ExtractionBox->OnComponentBeginOverlap.AddDynamic(this, &ALSExtractionZone::OnOverlapBegin);

	// 아트가 연기 Niagara 에셋을 매핑했으면 그걸 재생한다(임시 링 애니메이션과 함께 보임).
	if (SmokeEffectComponent && ExtractionSmokeEffect)
	{
		SmokeEffectComponent->SetAsset(ExtractionSmokeEffect);
		SmokeEffectComponent->Activate(true);
	}
}

void ALSExtractionZone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	MarkerAnimTime += DeltaSeconds;

	// 링을 바닥(-195)에서 위(+225)로 순환 상승시키고, 오르면서 반경을 키운다(연기가 퍼지며 올라가는 느낌).
	constexpr float RisePeriodSeconds = 2.4f;
	constexpr float RiseBaseZ = -195.f;
	constexpr float RiseHeight = 420.f;
	constexpr float RingStartScaleXY = 1.2f;
	constexpr float RingEndScaleXY = 4.0f;
	for (int32 RingIndex = 0; RingIndex < RisingRings.Num(); ++RingIndex)
	{
		UStaticMeshComponent* Ring = RisingRings[RingIndex];
		if (!Ring)
		{
			continue;
		}

		// 링마다 위상을 어긋나게 해 연속으로 올라오는 것처럼 보이게 한다.
		const float PhaseOffset = (RisePeriodSeconds / FMath::Max(1, RisingRings.Num())) * RingIndex;
		const float Alpha = FMath::Fmod(MarkerAnimTime + PhaseOffset, RisePeriodSeconds) / RisePeriodSeconds;

		const float RingScaleXY = FMath::Lerp(RingStartScaleXY, RingEndScaleXY, Alpha);
		Ring->SetRelativeLocation(FVector(0.f, 0.f, RiseBaseZ + Alpha * RiseHeight));
		Ring->SetRelativeScale3D(FVector(RingScaleXY, RingScaleXY, 0.06f));
	}

	// 탈출 지점을 살아있게 — 초록 라이트를 은은하게 맥동시킨다.
	if (MarkerLight)
	{
		MarkerLight->SetIntensity(900.f + 400.f * FMath::Sin(MarkerAnimTime * 3.f));
	}
}

void ALSExtractionZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!OtherActor->IsA<ALSPlayerCharacter>()) return;

	ALSFarmingGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSFarmingGameMode>();
	if (!GameMode) return;

	UE_LOG(LogLS, Log, TEXT("[ExtractionZone] 플레이어 탈출 감지"));
	GameMode->OnExtraction();
}

#undef LOCTEXT_NAMESPACE
