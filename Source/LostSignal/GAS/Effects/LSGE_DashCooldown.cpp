#include "GAS/Effects/LSGE_DashCooldown.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "GameplayEffectAttributeCaptureDefinition.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"

ULSGE_DashCooldown::ULSGE_DashCooldown(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DurationPolicy = EGameplayEffectDurationType::HasDuration;

	// 쿨타임 활성 중 LS.Cooldown.Dash 태그를 ASC에 부여 → LSGA_Dash::GetCooldownTags()가 이 태그로 재발동 차단
	UTargetTagsGameplayEffectComponent* TargetTagsComp =
		ObjectInitializer.CreateDefaultSubobject<UTargetTagsGameplayEffectComponent>(this, TEXT("DashCooldownTargetTags"));
	FInheritedTagContainer TagContainer;
	TagContainer.Added.AddTag(LSGameplayTags::Cooldown_Dash);
	TargetTagsComp->SetAndApplyTargetTagChanges(TagContainer);
	GEComponents.Add(TargetTagsComp);

	// DashCooldown 어트리뷰트 값을 GE 지속시간으로 사용
	FAttributeBasedFloat AttributeBased;
	AttributeBased.BackingAttribute = FGameplayEffectAttributeCaptureDefinition(
		ULSCharacterAttributeSet::GetDashCooldownAttribute(),
		EGameplayEffectAttributeCaptureSource::Source,
		true
	);
	AttributeBased.AttributeCalculationType = EAttributeBasedFloatCalculationType::AttributeMagnitude;
	AttributeBased.Coefficient = FScalableFloat(1.0f);

	DurationMagnitude = FGameplayEffectModifierMagnitude(AttributeBased);
}
