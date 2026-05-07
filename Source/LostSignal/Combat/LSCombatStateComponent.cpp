#include "Combat/LSCombatStateComponent.h"

#include "Engine/World.h"

ULSCombatStateComponent::ULSCombatStateComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool ULSCombatStateComponent::TrySubmitCommand(ELSCombatCommandType CommandType)
{
	if (CanExecuteCommand(CommandType))
	{
		return true;
	}

	if (ShouldBufferCommand(CommandType))
	{
		BufferCommand(CommandType);
	}

	return false;
}

void ULSCombatStateComponent::BeginAction(ELSCombatActionState NewState, ELSCombatActionPhase NewPhase)
{
	CurrentState = NewState;
	CurrentPhase = NewPhase;
}

void ULSCombatStateComponent::SetActionPhase(ELSCombatActionPhase NewPhase)
{
	if (CurrentState == ELSCombatActionState::Idle || CurrentState == ELSCombatActionState::Dead)
	{
		return;
	}

	CurrentPhase = NewPhase;
}

void ULSCombatStateComponent::EndAction()
{
	CurrentState = ELSCombatActionState::Idle;
	CurrentPhase = ELSCombatActionPhase::None;
}

bool ULSCombatStateComponent::EndActionIfCurrent(ELSCombatActionState ExpectedState)
{
	if (CurrentState != ExpectedState)
	{
		return false;
	}

	EndAction();
	return true;
}

bool ULSCombatStateComponent::ConsumeBufferedCommand(ELSCombatCommandType& OutCommandType)
{
	if (!HasBufferedCommand())
	{
		bHasBufferedCommand = false;
		return false;
	}

	OutCommandType = BufferedCommand.CommandType;
	bHasBufferedCommand = false;
	return true;
}

bool ULSCombatStateComponent::PeekBufferedCommand(ELSCombatCommandType& OutCommandType) const
{
	if (!HasBufferedCommand())
	{
		return false;
	}

	OutCommandType = BufferedCommand.CommandType;
	return true;
}

bool ULSCombatStateComponent::CanExecuteCommand(ELSCombatCommandType CommandType) const
{
	if (CurrentState == ELSCombatActionState::Dead || CurrentState == ELSCombatActionState::Stunned)
	{
		return false;
	}

	if (CurrentState == ELSCombatActionState::Idle)
	{
		return true;
	}

	return CanCancelCurrentActionWith(CommandType);
}

bool ULSCombatStateComponent::HasBufferedCommand() const
{
	return bHasBufferedCommand && BufferedCommand.IsValid(GetWorldTime());
}

bool ULSCombatStateComponent::CanCancelCurrentActionWith(ELSCombatCommandType CommandType) const
{
	if (CommandType == ELSCombatCommandType::Dash)
	{
		return CurrentState == ELSCombatActionState::BasicAttack &&
			(CurrentPhase == ELSCombatActionPhase::Startup || CurrentPhase == ELSCombatActionPhase::Recovery);
	}

	const bool bIsSkillCommand =
		CommandType == ELSCombatCommandType::Skill1 ||
		CommandType == ELSCombatCommandType::Skill2 ||
		CommandType == ELSCombatCommandType::Skill3 ||
		CommandType == ELSCombatCommandType::Ultimate;

	if (bIsSkillCommand)
	{
		return CurrentState == ELSCombatActionState::BasicAttack && CurrentPhase == ELSCombatActionPhase::Recovery;
	}

	return false;
}

bool ULSCombatStateComponent::ShouldBufferCommand(ELSCombatCommandType CommandType) const
{
	if (CurrentState == ELSCombatActionState::Dead || CurrentState == ELSCombatActionState::Stunned)
	{
		return false;
	}

	return CurrentPhase == ELSCombatActionPhase::Active || CurrentPhase == ELSCombatActionPhase::Recovery;
}

void ULSCombatStateComponent::BufferCommand(ELSCombatCommandType CommandType)
{
	BufferedCommand.CommandType = CommandType;
	BufferedCommand.ExpireTime = GetWorldTime() + DefaultBufferWindow;
	bHasBufferedCommand = true;
}

float ULSCombatStateComponent::GetWorldTime() const
{
	return GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}
