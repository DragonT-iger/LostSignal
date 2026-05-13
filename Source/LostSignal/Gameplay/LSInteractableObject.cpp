#include "Gameplay/LSInteractableObject.h"

#include "Characters/LSPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/LSInteractHintWidget.h"

ALSInteractableObject::ALSInteractableObject()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetSphereRadius(200.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	InteractionSphere->SetGenerateOverlapEvents(true);
	SetRootComponent(InteractionSphere);

	InteractWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractWidget"));
	InteractWidget->SetupAttachment(RootComponent);
	InteractWidget->SetWidgetSpace(EWidgetSpace::Screen);
	InteractWidget->SetDrawSize(FVector2D(200.f, 60.f));
	InteractWidget->SetHiddenInGame(true);
}

void ALSInteractableObject::BeginPlay()
{
	Super::BeginPlay();
	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ALSInteractableObject::OnSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ALSInteractableObject::OnSphereEndOverlap);
}

void ALSInteractableObject::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshWidgetVisibility();
}

bool ALSInteractableObject::CanInteract_Implementation(APawn* Interactor)
{
	return true;
}

void ALSInteractableObject::Interact_Implementation(APawn* Interactor)
{
}

FText ALSInteractableObject::GetInteractText_Implementation()
{
	return InteractText;
}

void ALSInteractableObject::RefreshWidgetVisibility()
{
	APawn* Pawn = FocusedLocalPawn.Get();
	if (!Pawn)
	{
		Pawn = FindOverlappingLocalPawn();
	}

	ALSPlayerCharacter* LSChar = Cast<ALSPlayerCharacter>(Pawn);
	const bool bInventoryWidgetOpen = LSChar && LSChar->IsInventoryWidgetOpen();
	const bool bShouldShow =
		LSChar &&
		!bInventoryWidgetOpen &&
		LSChar->ResolveBestInteractTarget() == this;

	InteractWidget->SetHiddenInGame(!bShouldShow);
	SetActorTickEnabled(Pawn != nullptr);
}

void ALSInteractableObject::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled()) return;
	if (!CanInteract_Implementation(Pawn)) return;

	FocusedLocalPawn = Pawn;
	UpdateHintWidget(Pawn);
	SetActorTickEnabled(true);
	RefreshWidgetVisibility();
}

void ALSInteractableObject::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !Pawn->IsLocallyControlled()) return;

	if (FocusedLocalPawn.Get() == Pawn)
	{
		FocusedLocalPawn.Reset();
	}

	InteractWidget->SetHiddenInGame(true);
	SetActorTickEnabled(false);
}

APawn* ALSInteractableObject::FindOverlappingLocalPawn() const
{
	TArray<AActor*> Overlapping;
	InteractionSphere->GetOverlappingActors(Overlapping, APawn::StaticClass());

	for (AActor* Actor : Overlapping)
	{
		APawn* Pawn = Cast<APawn>(Actor);
		if (Pawn && Pawn->IsLocallyControlled())
		{
			return Pawn;
		}
	}

	return nullptr;
}

void ALSInteractableObject::UpdateHintWidget(APawn* Pawn)
{
	FText KeyName = FText::FromString(TEXT("?"));

	ALSPlayerCharacter* LSChar = Cast<ALSPlayerCharacter>(Pawn);
	APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;
	if (LSChar && PC)
	{
		UEnhancedInputLocalPlayerSubsystem* Subsystem =
			ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer());

		if (Subsystem && LSChar->GetInteractAction())
		{
			TArray<FKey> Keys = Subsystem->QueryKeysMappedToAction(LSChar->GetInteractAction());
			if (Keys.Num() > 0)
			{
				KeyName = Keys[0].GetDisplayName();
			}
		}
	}

	if (ULSInteractHintWidget* HintWidget = Cast<ULSInteractHintWidget>(InteractWidget->GetWidget()))
	{
		HintWidget->UpdateHintInfo(GetInteractText_Implementation(), KeyName);
	}
}
