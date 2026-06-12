#include "UI/Combat/LSCombatBuffListWidget.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/PanelWidget.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolTypes.h"
#include "Data/LSProtocolUnlockRow.h"
#include "GAS/LSGameplayTags.h"
#include "GameplayEffect.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Combat/LSCombatBuffIconWidget.h"

#define LOCTEXT_NAMESPACE "LSCombatBuffListWidget"

void ULSCombatBuffListWidget::InitializeBuffListForPawn(APawn* InPawn)
{
	ObservedPawn = InPawn;
	RefreshBuffList();
}

void ULSCombatBuffListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (!BuffIconPanel)
	{
		UE_LOG(LogLS, Warning, TEXT("%s is missing required combat buff list binding: BuffIconPanel."), *GetNameSafe(this));
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void ULSCombatBuffListWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	RefreshBuffList();
}

void ULSCombatBuffListWidget::RefreshBuffList()
{
	if (!BuffIconPanel)
	{
		return;
	}

	if (!ObservedPawn.IsValid() || !IsBuffDurationProtocolVisible())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	TArray<FLSCombatBuffDisplayData> Displays;
	BuildBuffDisplays(Displays);
	if (Displays.IsEmpty())
	{
		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	for (int32 Index = 0; Index < Displays.Num(); ++Index)
	{
		ULSCombatBuffIconWidget* IconWidget = GetOrCreateBuffIcon(Index);
		if (!IconWidget)
		{
			continue;
		}

		IconWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		IconWidget->SetBuffDisplay(Displays[Index]);
	}

	for (int32 Index = Displays.Num(); Index < BuffIconPool.Num(); ++Index)
	{
		if (BuffIconPool[Index])
		{
			BuffIconPool[Index]->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void ULSCombatBuffListWidget::BuildBuffDisplays(TArray<FLSCombatBuffDisplayData>& OutDisplays) const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ObservedPawn.Get());
	if (!ASC)
	{
		return;
	}

	const FGameplayTag SupportedTags[] = {
		LSGameplayTags::Buff_CombatAcceleration,
		LSGameplayTags::Buff_AttackSpeed
	};

	for (const FGameplayTag BuffTag : SupportedTags)
	{
		FGameplayTagContainer QueryTags;
		QueryTags.AddTag(BuffTag);

		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAllOwningTags(QueryTags);
		for (const FActiveGameplayEffectHandle& Handle : ASC->GetActiveEffects(Query))
		{
			const float TotalDuration = ASC->GetGameplayEffectDuration(Handle);
			const float RemainingTime = UAbilitySystemBlueprintLibrary::GetActiveGameplayEffectRemainingDuration(ObservedPawn.Get(), Handle);
			if (TotalDuration <= 0.0f || RemainingTime <= 0.0f)
			{
				continue;
			}

			FLSCombatBuffDisplayData DisplayData;
			DisplayData.BuffTag = BuffTag;
			DisplayData.IconTexture = BuffIconTextures.FindRef(BuffTag);
			DisplayData.RemainingTime = RemainingTime;
			DisplayData.TotalDuration = TotalDuration;
			DisplayData.StackCount = FMath::Max(ASC->GetCurrentStackCount(Handle), 1);
			OutDisplays.Add(DisplayData);
		}
	}

	OutDisplays.Sort([](const FLSCombatBuffDisplayData& Left, const FLSCombatBuffDisplayData& Right)
	{
		return Left.RemainingTime < Right.RemainingTime;
	});
}

ULSCombatBuffIconWidget* ULSCombatBuffListWidget::GetOrCreateBuffIcon(const int32 Index)
{
	if (BuffIconPool.IsValidIndex(Index))
	{
		return BuffIconPool[Index];
	}

	if (!BuffIconPanel)
	{
		return nullptr;
	}

	if (!BuffIconWidgetClass)
	{
		if (!bLoggedMissingIconClass)
		{
			UE_LOG(LogLS, Warning, TEXT("%s cannot create combat buff icon because BuffIconWidgetClass is missing."), *GetNameSafe(this));
			bLoggedMissingIconClass = true;
		}
		return nullptr;
	}

	UWorld* World = GetWorld();
	ULSCombatBuffIconWidget* IconWidget = GetOwningPlayer()
		? CreateWidget<ULSCombatBuffIconWidget>(GetOwningPlayer(), BuffIconWidgetClass)
		: CreateWidget<ULSCombatBuffIconWidget>(World, BuffIconWidgetClass);
	if (!IconWidget)
	{
		return nullptr;
	}

	BuffIconPanel->AddChild(IconWidget);
	BuffIconPool.Add(IconWidget);
	return IconWidget;
}

bool ULSCombatBuffListWidget::IsBuffDurationProtocolVisible() const
{
	int32 CurrentLevel = 0;
	int32 PreviousLevel = 0;
	ResolveBattleProtocolLevels(CurrentLevel, PreviousLevel);

	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return CurrentLevel >= 3;
	}

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(
		ELSProtocolType::Battle,
		TEXT("Buff_Duration"),
		TEXT("CombatBuffDurationProtocol"));
	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : CurrentLevel >= 3;
}

void ULSCombatBuffListWidget::ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const
{
	OutCurrentLevel = 0;
	OutPreviousLevel = 0;

	if (const ALSPlayerControllerBase* PlayerController = GetOwningPlayer<ALSPlayerControllerBase>())
	{
		if (PlayerController->HasProtocolTestLevel(ELSProtocolType::Battle))
		{
			OutCurrentLevel = PlayerController->GetProtocolTestLevel(ELSProtocolType::Battle);
			OutPreviousLevel = OutCurrentLevel;
			return;
		}
	}

	UGameInstance* GameInstance = GetGameInstance();
	const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		return;
	}

	const int32 InactiveSlotCount = LSChipStats::ResolveInactiveSignalSlotCount(SaveSubsystem->GetChipSignalGaugePercent());
	const TArray<FLSSessionItem> ActiveEquipmentItems = LSChipStats::BuildSignalActiveEquipmentItems(SaveSubsystem->GetChipEquipmentSlots(), InactiveSlotCount);
	OutCurrentLevel = LSChipStats::AggregateChipProtocolTotals(ActiveEquipmentItems, this).Battle;
	OutPreviousLevel = LSChipStats::AggregateChipProtocolTotals(SaveSubsystem->GetChipEquipmentSlots(), this).Battle;
}

#undef LOCTEXT_NAMESPACE
