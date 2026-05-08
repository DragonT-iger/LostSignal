#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Skills/LSSkillAreaTypes.h"
#include "LSSkillDataAsset.generated.h"

UCLASS(BlueprintType)
class LOSTSIGNAL_API ULSSkillDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category="LS/Skill")
	const FLSSkillAreaPreviewSpec& GetPreviewSpec() const { return PreviewSpec; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	FName SkillId;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	FLSSkillAreaPreviewSpec PreviewSpec;
};
