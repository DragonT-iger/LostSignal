#pragma once

#include "CoreMinimal.h"
#include "LSSkillAreaTypes.generated.h"

class UMaterialInterface;

UENUM(BlueprintType)
enum class ELSSkillAreaShape : uint8
{
	Circle,
	Box
};

UENUM(BlueprintType)
enum class ELSSkillPreviewLocationMode : uint8
{
	MouseWorld,
	CasterOrigin
};

USTRUCT(BlueprintType)
struct FLSSkillAreaPreviewSpec
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	ELSSkillAreaShape Shape = ELSSkillAreaShape::Circle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	ELSSkillPreviewLocationMode LocationMode = ELSSkillPreviewLocationMode::MouseWorld;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	FVector2D LocationOffset = FVector2D::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview", meta=(ClampMin="0.0"))
	float Radius = 300.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview", meta=(ClampMin="0.0"))
	float BoxLength = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview", meta=(ClampMin="0.0"))
	float BoxWidth = 200.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview", meta=(ClampMin="0.0", ClampMax="360.0"))
	float Degrees = 360.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview", meta=(ClampMin="0.0"))
	float FillAmount = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview", meta=(ClampMin="0.0"))
	float FadeIntensity = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview", meta=(ClampMin="0.0"))
	float OutlineThickness = 0.02f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview", meta=(ClampMin="0.0"))
	float InnerRadius = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	float RotationOffsetDegrees = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	float WorldZOffset = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/Skill|Preview")
	TObjectPtr<UMaterialInterface> Material = nullptr;
};
