#include "Gameplay/LSRaidEntranceZone.h"
#include "Core/LSLobbyGameMode.h"
#include "Characters/LSPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "LostSignal.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "LSRaidEntranceZone"

ALSRaidEntranceZone::ALSRaidEntranceZone()
{
	PrimaryActorTick.bCanEverTick = false;

	EntranceBox = CreateDefaultSubobject<UBoxComponent>(TEXT("EntranceBox"));
	SetRootComponent(EntranceBox);
	EntranceBox->SetBoxExtent(FVector(200.f, 200.f, 200.f));
	EntranceBox->SetCollisionProfileName(TEXT("Trigger"));

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	MarkerMesh->SetupAttachment(EntranceBox);
	MarkerMesh->SetRelativeLocation(FVector(0.f, 0.f, 20.f));
	MarkerMesh->SetRelativeScale3D(FVector(2.8f, 2.8f, 0.12f));
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MarkerMesh->SetGenerateOverlapEvents(false);
	MarkerMesh->SetCustomDepthStencilValue(251);
	MarkerMesh->SetRenderCustomDepth(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		MarkerMesh->SetStaticMesh(CylinderMesh.Object);
	}

	MarkerText = CreateDefaultSubobject<UTextRenderComponent>(TEXT("MarkerText"));
	MarkerText->SetupAttachment(EntranceBox);
	MarkerText->SetRelativeLocation(FVector(0.f, 0.f, 260.f));
	MarkerText->SetRelativeRotation(FRotator(60.f, 0.f, 0.f));
	MarkerText->SetHorizontalAlignment(EHTA_Center);
	MarkerText->SetVerticalAlignment(EVRTA_TextCenter);
	MarkerText->SetText(LOCTEXT("RaidEntranceMarkerText", "RAID"));
	MarkerText->SetTextRenderColor(FColor(80, 180, 255));
	MarkerText->SetWorldSize(56.f);
	MarkerText->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	MarkerLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("MarkerLight"));
	MarkerLight->SetupAttachment(EntranceBox);
	MarkerLight->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	MarkerLight->SetLightColor(FLinearColor(0.15f, 0.45f, 1.f));
	MarkerLight->SetIntensity(1200.f);
	MarkerLight->SetAttenuationRadius(450.f);
}

void ALSRaidEntranceZone::BeginPlay()
{
	Super::BeginPlay();
	EntranceBox->OnComponentBeginOverlap.AddDynamic(this, &ALSRaidEntranceZone::OnOverlapBegin);
}

void ALSRaidEntranceZone::OnOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!OtherActor->IsA<ALSPlayerCharacter>()) return;

	ALSLobbyGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSLobbyGameMode>();
	if (!GameMode) return;

	UE_LOG(LogLS, Log, TEXT("[RaidEntrance] 플레이어 레이드 입장 감지"));
	GameMode->StartRaid();
}

#undef LOCTEXT_NAMESPACE
