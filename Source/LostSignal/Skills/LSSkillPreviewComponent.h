#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Skills/LSSkillAreaTypes.h"
#include "LSSkillPreviewComponent.generated.h"

class ALSSkillAreaPreviewActor;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSSkillPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSSkillPreviewComponent();

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	bool BeginAreaPreview(const FLSSkillAreaPreviewSpec& PreviewSpec);

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	void UpdateAreaPreview(const FVector& WorldLocation, const FRotator& WorldRotation);

	UFUNCTION(BlueprintCallable, Category="LS/Skill|Preview")
	void EndAreaPreview();

	UFUNCTION(BlueprintPure, Category="LS/Skill|Preview")
	bool IsAreaPreviewActive() const { return ActivePreviewActor != nullptr; }

	UFUNCTION(BlueprintPure, Category="LS/Skill|Preview")
	ALSSkillAreaPreviewActor* GetActivePreviewActor() const { return ActivePreviewActor; }

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(EditDefaultsOnly, Category="LS/Skill|Preview")
	TSubclassOf<ALSSkillAreaPreviewActor> PreviewActorClass;

	UPROPERTY(Transient, VisibleInstanceOnly, Category="LS/Skill|Preview")
	TObjectPtr<ALSSkillAreaPreviewActor> ActivePreviewActor;
};
