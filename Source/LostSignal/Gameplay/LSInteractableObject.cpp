#include "Gameplay/LSInteractableObject.h"

#include "Characters/LSPlayerCharacter.h"
#include "Components/MeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "TimerManager.h"
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
	InteractionSphere->UpdateOverlaps();
	GatherOutlineMeshes();
	RefreshWidgetVisibility();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(this, &ALSInteractableObject::RefreshWidgetVisibility);
	}
}

void ALSInteractableObject::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RefreshWidgetVisibility();
}

void ALSInteractableObject::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 하이라이트 중 파괴·레벨 전환 시 커스텀뎁스가 켜진 채 남지 않게 확실히 끈다.
	ApplyOutlineState(false);
	Super::EndPlay(EndPlayReason);
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

void ALSInteractableObject::HandleLocalPawnEndOverlap(APawn* Pawn)
{
}

void ALSInteractableObject::SetProximityOutlineEnabled(bool bEnabled)
{
	bEnableProximityOutline = bEnabled;
	if (!bEnabled)
	{
		ApplyOutlineState(false);
	}
	else
	{
		RefreshWidgetVisibility();
	}
}

void ALSInteractableObject::GatherOutlineMeshes()
{
	OutlineMeshes.Reset();

	TArray<UMeshComponent*> Meshes;
	GetComponents<UMeshComponent>(Meshes);
	for (UMeshComponent* Mesh : Meshes)
	{
		if (!Mesh || Mesh->IsA<UWidgetComponent>())
		{
			// 힌트 위젯(UWidgetComponent도 UMeshComponent를 상속)은 아웃라인 대상이 아니다.
			continue;
		}
		if (Mesh->bRenderCustomDepth)
		{
			// 이미 커스텀뎁스를 쓰는 메시(예: 마커 스텐실 253)는 건드리지 않는다.
			UE_LOG(LogLS, Log,
				TEXT("[Outline] %s의 %s는 이미 CustomDepth 사용(Stencil=%d) — 근접 아웃라인 대상에서 제외."),
				*GetNameSafe(this), *GetNameSafe(Mesh), Mesh->CustomDepthStencilValue);
			continue;
		}
		OutlineMeshes.Add(Mesh);
	}
}

void ALSInteractableObject::ApplyOutlineState(bool bWantOutline)
{
	const bool bTarget = bWantOutline && bEnableProximityOutline;
	if (bTarget == bOutlineActive)
	{
		// 매 틱 렌더 플래그를 다시 세팅하지 않도록 전이 시에만 적용한다.
		return;
	}
	bOutlineActive = bTarget;

	for (const TWeakObjectPtr<UMeshComponent>& WeakMesh : OutlineMeshes)
	{
		if (UMeshComponent* Mesh = WeakMesh.Get())
		{
			if (bTarget)
			{
				Mesh->SetCustomDepthStencilValue(OutlineStencilValue);
			}
			Mesh->SetRenderCustomDepth(bTarget);
		}
	}
}

void ALSInteractableObject::RefreshWidgetVisibility()
{
	APawn* Pawn = FocusedLocalPawn.Get();
	if (!Pawn)
	{
		Pawn = FindOverlappingLocalPawn();
		if (Pawn)
		{
			FocusedLocalPawn = Pawn;
			UpdateHintWidget(Pawn);
		}
	}

	ALSPlayerCharacter* LSChar = Cast<ALSPlayerCharacter>(Pawn);
	const bool bInventoryWidgetOpen = LSChar && LSChar->IsInventoryWidgetOpen();
	const bool bShouldShow =
		LSChar &&
		!bInventoryWidgetOpen &&
		LSChar->ResolveBestInteractTarget() == this;

	InteractWidget->SetHiddenInGame(!bShouldShow);
	ApplyOutlineState(bShouldShow);
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

	HandleLocalPawnEndOverlap(Pawn);

	if (FocusedLocalPawn.Get() == Pawn)
	{
		FocusedLocalPawn.Reset();
	}

	InteractWidget->SetHiddenInGame(true);
	// 이 경로는 RefreshWidgetVisibility를 호출하지 않으므로 아웃라인을 명시적으로 끈다.
	ApplyOutlineState(false);
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
	if (!InteractWidget)
	{
		if (!bLoggedMissingInteractWidget)
		{
			UE_LOG(LogLS, Warning, TEXT("InteractWidget is not bound on %s."), *GetNameSafe(this));
			bLoggedMissingInteractWidget = true;
		}
		return;
	}

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
		return;
	}

	if (!bLoggedInvalidInteractWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("InteractWidget on %s does not contain ULSInteractHintWidget. WidgetClass=%s"),
			*GetNameSafe(this),
			*GetNameSafe(InteractWidget->GetWidgetClass()));
		bLoggedInvalidInteractWidget = true;
	}
}
