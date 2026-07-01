#include "Animation/LSANS_BlockInput.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/LSGameplayTags.h"
#include "LostSignal.h"

namespace
{
	UAbilitySystemComponent* ResolveBlockInputAbilitySystem(const USkeletalMeshComponent* MeshComp)
	{
		AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
		return OwnerActor ? UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor) : nullptr;
	}
}

void ULSANS_BlockInput::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	if (UAbilitySystemComponent* ASC = ResolveBlockInputAbilitySystem(MeshComp))
	{
		// loose 태그는 카운트 방식이라 겹쳐도 안전(Begin=+1 / End=-1).
		ASC->AddLooseGameplayTag(LSGameplayTags::State_InputBlocked);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[LSANS_BlockInput] 소유 Actor ASC를 찾지 못해 입력 차단을 적용하지 못함."));
	}
}

void ULSANS_BlockInput::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (UAbilitySystemComponent* ASC = ResolveBlockInputAbilitySystem(MeshComp))
	{
		ASC->RemoveLooseGameplayTag(LSGameplayTags::State_InputBlocked);
	}
}

FString ULSANS_BlockInput::GetNotifyName_Implementation() const
{
	return TEXT("LS Block Input");
}
