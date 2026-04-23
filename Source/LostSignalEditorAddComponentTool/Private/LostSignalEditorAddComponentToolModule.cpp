#include "LostSignalEditorAddComponentToolModule.h"

#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMeshActor.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "Vision/LSRoofFadeComponent.h"
#include "Vision/LSStencilMarkerComponent.h"
#include "Vision/LSVisionOccluderComponent.h"
#include "Vision/LSVisionSurfaceComponent.h"

#define LOCTEXT_NAMESPACE "LostSignalEditorAddComponentTool"

namespace
{
	ULSStencilMarkerComponent* EnsureStencilMarkerComponent(AStaticMeshActor* StaticMeshActor, UStaticMeshComponent* StaticMeshComponent)
	{
		if (StaticMeshActor == nullptr || StaticMeshComponent == nullptr)
		{
			return nullptr;
		}

		ULSStencilMarkerComponent* StencilMarkerComponent = StaticMeshActor->FindComponentByClass<ULSStencilMarkerComponent>();
		if (StencilMarkerComponent == nullptr)
		{
			StencilMarkerComponent = NewObject<ULSStencilMarkerComponent>(
				StaticMeshActor,
				ULSStencilMarkerComponent::StaticClass(),
				TEXT("StencilMarkerComponent"),
				RF_Transactional);

			if (StencilMarkerComponent != nullptr)
			{
				StaticMeshActor->AddInstanceComponent(StencilMarkerComponent);
				StencilMarkerComponent->TargetPrimitives.AddUnique(StaticMeshComponent);
				StencilMarkerComponent->OnComponentCreated();
				StencilMarkerComponent->RegisterComponent();
				StencilMarkerComponent->Modify();
			}
		}
		else
		{
			StencilMarkerComponent->Modify();
			StencilMarkerComponent->TargetPrimitives.AddUnique(StaticMeshComponent);
			StencilMarkerComponent->ApplyStencilSettings();
		}

		return StencilMarkerComponent;
	}
}

void FLostSignalEditorAddComponentToolModule::StartupModule()
{
	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FLostSignalEditorAddComponentToolModule::RegisterMenus));
}

void FLostSignalEditorAddComponentToolModule::ShutdownModule()
{
	if (UToolMenus* ToolMenus = UToolMenus::TryGet())
	{
		UToolMenus::UnRegisterStartupCallback(this);
		ToolMenus->UnregisterOwner(this);
	}
}

void FLostSignalEditorAddComponentToolModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.ActorContextMenu");
	FToolMenuSection& Section = Menu->AddSection("LostSignal", LOCTEXT("LostSignalSection", "LostSignal"));

	Section.AddMenuEntry(
		"AddVisionSetup",
		LOCTEXT("AddVisionSetupLabel", "Add Vision Setup"),
		LOCTEXT("AddVisionSetupTooltip", "Add VisionOccluder, VisionSurface, and StencilMarker components to selected static mesh actors."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLostSignalEditorAddComponentToolModule::AddVisionSetupToSelectedActors))
	);

	Section.AddMenuEntry(
		"AddRoofSetup",
		LOCTEXT("AddRoofSetupLabel", "Add Roof Setup"),
		LOCTEXT("AddRoofSetupTooltip", "Add RoofFadeComponent and StencilMarker to selected static mesh actors."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLostSignalEditorAddComponentToolModule::AddRoofSetupToSelectedActors))
	);
}

void FLostSignalEditorAddComponentToolModule::AddVisionSetupToSelectedActors()
{
	if (GEditor == nullptr)
	{
		return;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (SelectedActors == nullptr || SelectedActors->Num() == 0)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddVisionSetupTransaction", "Add Vision Setup"));

	for (FSelectionIterator SelectionIt(*SelectedActors); SelectionIt; ++SelectionIt)
	{
		AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(*SelectionIt);
		if (StaticMeshActor == nullptr)
		{
			continue;
		}

		UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
		if (StaticMeshComponent == nullptr)
		{
			continue;
		}

		StaticMeshActor->Modify();
		StaticMeshComponent->Modify();

		ULSVisionOccluderComponent* OccluderComponent = StaticMeshActor->FindComponentByClass<ULSVisionOccluderComponent>();
		if (OccluderComponent == nullptr)
		{
			OccluderComponent = NewObject<ULSVisionOccluderComponent>(
				StaticMeshActor,
				ULSVisionOccluderComponent::StaticClass(),
				TEXT("VisionOccluderComponent"),
				RF_Transactional);

			if (OccluderComponent != nullptr)
			{
				StaticMeshActor->AddInstanceComponent(OccluderComponent);
				OccluderComponent->SourceMode = ELSVisionOccluderSourceMode::CollisionGeometry;
				OccluderComponent->SourcePrimitiveComponent = StaticMeshComponent;
				OccluderComponent->bAutoFindOwnerComponents = true;
				OccluderComponent->OnComponentCreated();
				OccluderComponent->RegisterComponent();
				OccluderComponent->Modify();
			}
		}
		else
		{
			OccluderComponent->Modify();
			OccluderComponent->SourceMode = ELSVisionOccluderSourceMode::CollisionGeometry;
			OccluderComponent->SourcePrimitiveComponent = StaticMeshComponent;
		}

		ULSVisionSurfaceComponent* SurfaceComponent = StaticMeshActor->FindComponentByClass<ULSVisionSurfaceComponent>();
		if (SurfaceComponent == nullptr)
		{
			SurfaceComponent = NewObject<ULSVisionSurfaceComponent>(
				StaticMeshActor,
				ULSVisionSurfaceComponent::StaticClass(),
				TEXT("VisionSurfaceComponent"),
				RF_Transactional);

			if (SurfaceComponent != nullptr)
			{
				StaticMeshActor->AddInstanceComponent(SurfaceComponent);
				SurfaceComponent->TargetPrimitives.AddUnique(StaticMeshComponent);
				SurfaceComponent->OnComponentCreated();
				SurfaceComponent->RegisterComponent();
				SurfaceComponent->Modify();
			}
		}
		else
		{
			SurfaceComponent->Modify();
			SurfaceComponent->TargetPrimitives.AddUnique(StaticMeshComponent);
		}

		EnsureStencilMarkerComponent(StaticMeshActor, StaticMeshComponent);

		StaticMeshActor->MarkPackageDirty();
	}
}

void FLostSignalEditorAddComponentToolModule::AddRoofSetupToSelectedActors()
{
	if (GEditor == nullptr)
	{
		return;
	}

	USelection* SelectedActors = GEditor->GetSelectedActors();
	if (SelectedActors == nullptr || SelectedActors->Num() == 0)
	{
		return;
	}

	const FScopedTransaction Transaction(LOCTEXT("AddRoofSetupTransaction", "Add Roof Setup"));

	for (FSelectionIterator SelectionIt(*SelectedActors); SelectionIt; ++SelectionIt)
	{
		AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(*SelectionIt);
		if (StaticMeshActor == nullptr)
		{
			continue;
		}

		UStaticMeshComponent* StaticMeshComponent = StaticMeshActor->GetStaticMeshComponent();
		if (StaticMeshComponent == nullptr)
		{
			continue;
		}

		StaticMeshActor->Modify();
		StaticMeshComponent->Modify();

		ULSRoofFadeComponent* RoofFadeComponent = StaticMeshActor->FindComponentByClass<ULSRoofFadeComponent>();
		if (RoofFadeComponent == nullptr)
		{
			RoofFadeComponent = NewObject<ULSRoofFadeComponent>(
				StaticMeshActor,
				ULSRoofFadeComponent::StaticClass(),
				TEXT("RoofFadeComponent"),
				RF_Transactional);

			if (RoofFadeComponent != nullptr)
			{
				StaticMeshActor->AddInstanceComponent(RoofFadeComponent);
				RoofFadeComponent->TargetPrimitives.AddUnique(StaticMeshComponent);
				RoofFadeComponent->OnComponentCreated();
				RoofFadeComponent->RegisterComponent();
				RoofFadeComponent->Modify();
			}
		}
		else
		{
			RoofFadeComponent->Modify();
			RoofFadeComponent->TargetPrimitives.AddUnique(StaticMeshComponent);
		}

		EnsureStencilMarkerComponent(StaticMeshActor, StaticMeshComponent);

		StaticMeshActor->MarkPackageDirty();
	}
}

IMPLEMENT_MODULE(FLostSignalEditorAddComponentToolModule, LostSignalEditorAddComponentTool)

#undef LOCTEXT_NAMESPACE
