#pragma once

#include "Components/SceneComponent.h"
#include "Components/WidgetComponent.h"
#include "CoreMinimal.h"
#include "LSEnemyHealthBarComponent.generated.h"

class ALSEnemyCharacter;
class UAbilitySystemComponent;
class ULSEnemyHealthBarWidget;
struct FOnAttributeChangeData;

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSEnemyHealthBarComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	ULSEnemyHealthBarComponent();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	FVector WidgetOffset = FVector(0.0f, 0.0f, 160.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	FVector2D DrawSize = FVector2D(100.0f, 10.0f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	FVector2D Pivot = FVector2D(0.5f, 0.5f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	EWidgetSpace WidgetSpace = EWidgetSpace::World;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	bool bFaceCamera = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="LS/UI|Combat")
	bool bTwoSided = true;

private:
	void InitializeForEnemy();
	void CreateWidgetComponent();
	void ConfigureWidgetComponent();
	void BindToEnemyASC(UAbilitySystemComponent* NewASC);
	void UnbindFromEnemyASC();
	void RefreshHealthBar();
	void RefreshVisibility();
	void UpdateCameraFacing(APlayerController* LocalPlayerController);
	void SetHealthBarVisible(bool bShouldBeVisible);
	void HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData);
	bool ResolveHealth(float& OutCurrentHealth, float& OutMaxHealth) const;
	bool IsEnemyHealthBarProtocolVisible(APlayerController* LocalPlayerController) const;
	APlayerController* FindLocalPlayerController() const;
	ULSEnemyHealthBarWidget* GetHealthBarWidget() const;

	UPROPERTY(Transient)
	TObjectPtr<UWidgetComponent> HealthBarWidgetComponent;

	UPROPERTY(Transient)
	TSubclassOf<ULSEnemyHealthBarWidget> ResolvedHealthBarWidgetClass;

	TWeakObjectPtr<ALSEnemyCharacter> ObservedEnemy;
	TWeakObjectPtr<UAbilitySystemComponent> ObservedASC;
	FDelegateHandle CurrentHealthChangedHandle;
	FDelegateHandle MaxHealthChangedHandle;
	bool bHealthBarVisible = false;
	bool bWarnedMissingWidgetClass = false;
};
