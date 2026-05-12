#pragma once

#include "Combat/LSCombatTypes.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LSCombatStateComponent.generated.h"

UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSCombatStateComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSCombatStateComponent();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool TrySubmitCommand(ELSCombatCommandType CommandType);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void BeginAction(ELSCombatActionState NewState, ELSCombatActionPhase NewPhase = ELSCombatActionPhase::Startup);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void SetActionPhase(ELSCombatActionPhase NewPhase);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void EndAction();

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool EndActionIfCurrent(ELSCombatActionState ExpectedState);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	bool ConsumeBufferedCommand(ELSCombatCommandType& OutCommandType);

	UFUNCTION(BlueprintCallable, Category="LS/Combat")
	void ClearBufferedCommand();

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool PeekBufferedCommand(ELSCombatCommandType& OutCommandType) const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool CanExecuteCommand(ELSCombatCommandType CommandType) const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	bool HasBufferedCommand() const;

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ELSCombatActionState GetCurrentState() const { return CurrentState; }

	UFUNCTION(BlueprintPure, Category="LS/Combat")
	ELSCombatActionPhase GetCurrentPhase() const { return CurrentPhase; }

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(AllowPrivateAccess="true"))
	ELSCombatActionState CurrentState = ELSCombatActionState::Idle;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(AllowPrivateAccess="true"))
	ELSCombatActionPhase CurrentPhase = ELSCombatActionPhase::None;

	UPROPERTY(EditAnywhere, Category="LS/Combat", meta=(ClampMin="0.0"))
	float DefaultBufferWindow = 0.2f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="LS/Combat", meta=(AllowPrivateAccess="true"))
	FLSBufferedCombatCommand BufferedCommand;

	bool bHasBufferedCommand = false;

	bool CanCancelCurrentActionWith(ELSCombatCommandType CommandType) const;
	bool ShouldBufferCommand(ELSCombatCommandType CommandType) const;
	void BufferCommand(ELSCombatCommandType CommandType);
	float GetWorldTime() const;
};
