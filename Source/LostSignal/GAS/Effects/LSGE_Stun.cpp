#include "GAS/Effects/LSGE_Stun.h"

#include "GAS/LSGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

ULSGE_Stun::ULSGE_Stun(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;
	DurationMagnitude = FScalableFloat(1.0f);

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("StunTargetTags"));
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(LSGameplayTags::State_Stunned);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
	GEComponents.Add(TargetTagsComp);
}
