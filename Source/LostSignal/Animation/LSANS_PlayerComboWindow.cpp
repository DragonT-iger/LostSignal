#include "Animation/LSANS_PlayerComboWindow.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/Abilities/LSGA_PlayerBasicAttack.h"

void ULSANS_PlayerComboWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	UAbilitySystemComponent* ASC = MeshComp ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()) : nullptr;
	if (ULSGA_PlayerBasicAttack* BasicAttackAbility = ULSGA_PlayerBasicAttack::FindActiveBasicAttackAbility(ASC))
	{
		BasicAttackAbility->OpenComboWindow();
	}
}

void ULSANS_PlayerComboWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	UAbilitySystemComponent* ASC = MeshComp ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MeshComp->GetOwner()) : nullptr;
	if (ULSGA_PlayerBasicAttack* BasicAttackAbility = ULSGA_PlayerBasicAttack::FindActiveBasicAttackAbility(ASC))
	{
		BasicAttackAbility->CloseComboWindow();
	}
}

FString ULSANS_PlayerComboWindow::GetNotifyName_Implementation() const
{
	return TEXT("LS Player Combo Window");
}
