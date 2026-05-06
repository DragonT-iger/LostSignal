#include "Gameplay/LSExtractionZone.h"
#include "Core/LSFarmingGameMode.h"
#include "Characters/LSPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "LostSignal.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LSExtractionZone"

ALSExtractionZone::ALSExtractionZone()
{
	PrimaryActorTick.bCanEverTick = false;

	ExtractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionBox"));
	SetRootComponent(ExtractionBox);
	ExtractionBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	ExtractionBox->SetCollisionProfileName(TEXT("Trigger"));

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(ExtractionBox);
	MarkerMesh->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	MarkerMesh->SetRelativeScale3D(FVector(2.8f, 2.8f, 0.12f));
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetGenerateOverlapEvents(false);
	MarkerMesh->SetCustomDepthStencilValue(252);
	MarkerMesh->SetRenderCustomDepth(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CylinderMesh.Object);
	}

	MarkerText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MarkerText"));
	MarkerText->SetupAttachment(ExtractionBox);
	MarkerText->SetRelativeLocation(FVector(0.f, 0.f, 260.f));
	MarkerText->SetRelativeRotation(FRotator(60.f, 0.f, 0.f));
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
}

void ALSExtractionZone::BeginPlay()
{
	Super::BeginPlay();
	ExtractionBox->OnComponentBeginOverlap.AddDynamic(this, &ALSExtractionZone::OnOverlapBegin);
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
