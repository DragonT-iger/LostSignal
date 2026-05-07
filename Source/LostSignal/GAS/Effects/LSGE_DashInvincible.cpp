#include "GAS/Effects/LSGE_DashInvincible.h"

#include "GAS/LSGameplayTags.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

ULSGE_DashInvincible::ULSGE_DashInvincible(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::Infinite;

	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("DashInvincibleTargetTags"));
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(LSGameplayTags::State_Invincible);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
	GEComponents.Add(TargetTagsComp);
}
