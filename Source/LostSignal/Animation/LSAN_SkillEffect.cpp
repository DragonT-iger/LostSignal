#include "Animation/LSAN_SkillEffect.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/LSGameplayTags.h"

void ULSAN_SkillEffect::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	// 노티파이는 서버/클라 모두에서 발동하지만, 스킬 Ability는 ServerOnly라 서버 인스턴스만 이벤트를 수신한다.
	const FGameplayTag EventTag = SkillEffectEventTag.IsValid() ? SkillEffectEventTag : LSGameplayTags::Event_Skill_Hit;

	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = OwnerActor;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, EventTag, Payload);
}

FString ULSAN_SkillEffect::GetNotifyName_Implementation() const
{
	return TEXT("LS Skill Effect");
}
