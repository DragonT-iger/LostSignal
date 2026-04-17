#include "LostSignalEditorAddComponentToolModule.h"

#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMeshActor.h"
#include "ScopedTransaction.h"
#include "ToolMenus.h"
#include "Vision/LSVisionOccluderComponent.h"
#include "Vision/LSVisionSurfaceComponent.h"

#define LOCTEXT_NAMESPACE "LostSignalEditorAddComponentTool"

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
		LOCTEXT("AddVisionSetupTooltip", "Add VisionOccluder and VisionSurface components to selected static mesh actors."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateRaw(this, &FLostSignalEditorAddComponentToolModule::AddVisionSetupToSelectedActors))
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

		StaticMeshActor->MarkPackageDirty();
	}
}

IMPLEMENT_MODULE(FLostSignalEditorAddComponentToolModule, LostSignalEditorAddComponentTool)

#undef LOCTEXT_NAMESPACE
