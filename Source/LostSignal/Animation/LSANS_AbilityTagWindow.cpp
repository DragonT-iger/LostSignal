#include "Animation/LSANS_AbilityTagWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"

void ULSANS_AbilityTagWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	UAbilitySystemComponent* ASC = MeshComp ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()) : nullptr;
	if (!ASC || !GrantedTag.IsValid())
	{
		return;
	}

	ASC->AddLooseGameplayTag(GrantedTag);
}

void ULSANS_AbilityTagWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	UAbilitySystemComponent* ASC = MeshComp ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()) : nullptr;
	if (!ASC || !GrantedTag.IsValid())
	{
		return;
	}

	ASC->RemoveLooseGameplayTag(GrantedTag);
}

FString ULSANS_AbilityTagWindow::GetNotifyName_Implementation() const
{
	return GrantedTag.IsValid() ? FString::Printf(TEXT("LS Tag Window: %s"), *GrantedTag.ToString()) : TEXT("LS Tag Window");
}
