#include "UI/Combat/LSEnemyHealthBarComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSMonsterPresentationSettings.h"
#include "Data/LSProtocolUnlockRow.h"
#include "Data/LSProtocolTypes.h"
#include "Engine/GameInstance.h"
#include "GAS/LSCombatAttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "UI/Combat/LSEnemyHealthBarWidget.h"

ULSEnemyHealthBarComponent::ULSEnemyHealthBarComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(false);
}

void ULSEnemyHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeForEnemy();
	CreateWidgetComponent();
	RefreshHealthBar();
	SetHealthBarVisible(false);
}

void ULSEnemyHealthBarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromEnemyASC();

	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->DestroyComponent();
		HealthBarWidgetComponent = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void ULSEnemyHealthBarComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	APlayerController* LocalPlayerController = FindLocalPlayerController();
	const bool bProtocolVisible = LocalPlayerController && IsEnemyHealthBarProtocolVisible(LocalPlayerController);
	SetHealthBarVisible(bProtocolVisible);
	if (!bProtocolVisible)
	{
		return;
	}

	RefreshHealthBar();
	UpdateCameraFacing(LocalPlayerController);
}

void ULSEnemyHealthBarComponent::InitializeForEnemy()
{
	ObservedEnemy = Cast<ALSEnemyCharacter>(GetOwner());
	BindToEnemyASC(ObservedEnemy.IsValid() ? ObservedEnemy->GetAbilitySystemComponent() : nullptr);
}

void ULSEnemyHealthBarComponent::CreateWidgetComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	HealthBarWidgetComponent = NewObject<UWidgetComponent>(Owner, TEXT("EnemyHealthBarWidgetComponent"));
	if (!HealthBarWidgetComponent)
	{
		return;
	}

	HealthBarWidgetComponent->SetupAttachment(this);
	HealthBarWidgetComponent->RegisterComponent();
	ConfigureWidgetComponent();
}

void ULSEnemyHealthBarComponent::ConfigureWidgetComponent()
{
	if (!HealthBarWidgetComponent)
	{
		return;
	}

	HealthBarWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HealthBarWidgetComponent->SetGenerateOverlapEvents(false);
	HealthBarWidgetComponent->SetWidgetSpace(WidgetSpace);
	HealthBarWidgetComponent->SetDrawSize(DrawSize);
	HealthBarWidgetComponent->SetPivot(Pivot);
	HealthBarWidgetComponent->SetTwoSided(bTwoSided);
	HealthBarWidgetComponent->SetRelativeLocation(WidgetOffset);
	HealthBarWidgetComponent->SetHiddenInGame(true);

	if (const ULSMonsterPresentationSettings* Settings = GetDefault<ULSMonsterPresentationSettings>())
	{
		ResolvedHealthBarWidgetClass = Settings->EnemyHealthBarWidgetClass.LoadSynchronous();
	}

	if (ResolvedHealthBarWidgetClass)
	{
		HealthBarWidgetComponent->SetWidgetClass(ResolvedHealthBarWidgetClass);
		HealthBarWidgetComponent->InitWidget();
	}
	else if (!bWarnedMissingWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot show enemy health bar because EnemyHealthBarWidgetClass is not set in LS Monster Presentation Settings."), *GetNameSafe(this));
		bWarnedMissingWidgetClass = true;
	}
}

void ULSEnemyHealthBarComponent::BindToEnemyASC(UAbilitySystemComponent* NewASC)
{
	if (ObservedASC.Get() == NewASC)
	{
		return;
	}

	UnbindFromEnemyASC();
	ObservedASC = NewASC;
	if (!NewASC)
	{
		return;
	}

	CurrentHealthChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetCurrentHealthAttribute())
		.AddUObject(this, &ULSEnemyHealthBarComponent::HandleCurrentHealthChanged);
	MaxHealthChangedHandle = NewASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &ULSEnemyHealthBarComponent::HandleMaxHealthChanged);
}

void ULSEnemyHealthBarComponent::UnbindFromEnemyASC()
{
	if (UAbilitySystemComponent* ASC = ObservedASC.Get())
	{
		if (CurrentHealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetCurrentHealthAttribute()).Remove(CurrentHealthChangedHandle);
		}
		if (MaxHealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedHandle);
		}
	}

	CurrentHealthChangedHandle.Reset();
	MaxHealthChangedHandle.Reset();
	ObservedASC.Reset();
}

void ULSEnemyHealthBarComponent::RefreshHealthBar()
{
	float CurrentHealth = 0.0f;
	float MaxHealth = 0.0f;
	const float Percent = ResolveHealth(CurrentHealth, MaxHealth) && MaxHealth > 0.0f
		? CurrentHealth / MaxHealth
		: 0.0f;

	if (ULSEnemyHealthBarWidget* HealthBarWidget = GetHealthBarWidget())
	{
		HealthBarWidget->SetHealthPercent(Percent);
	}
}

void ULSEnemyHealthBarComponent::RefreshVisibility()
{
	APlayerController* LocalPlayerController = FindLocalPlayerController();
	SetHealthBarVisible(LocalPlayerController && IsEnemyHealthBarProtocolVisible(LocalPlayerController));
}

void ULSEnemyHealthBarComponent::UpdateCameraFacing(APlayerController* LocalPlayerController)
{
	if (!bFaceCamera || WidgetSpace != EWidgetSpace::World || !HealthBarWidgetComponent)
	{
		return;
	}

	APlayerCameraManager* CameraManager = LocalPlayerController ? LocalPlayerController->PlayerCameraManager : nullptr;
	if (!CameraManager)
	{
		return;
	}

	const FVector WidgetLocation = HealthBarWidgetComponent->GetComponentLocation();
	const FVector ToCamera = (CameraManager->GetCameraLocation() - WidgetLocation).GetSafeNormal();
	if (ToCamera.IsNearlyZero())
	{
		return;
	}

	HealthBarWidgetComponent->SetWorldRotation(ToCamera.Rotation());
}

void ULSEnemyHealthBarComponent::SetHealthBarVisible(const bool bShouldBeVisible)
{
	float CurrentHealth = 0.0f;
	float MaxHealth = 0.0f;
	const bool bShouldShow = bShouldBeVisible &&
		HealthBarWidgetComponent &&
		ResolvedHealthBarWidgetClass &&
		ResolveHealth(CurrentHealth, MaxHealth) &&
		CurrentHealth > 0.0f;
	if (bHealthBarVisible == bShouldShow)
	{
		return;
	}

	bHealthBarVisible = bShouldShow;
	if (HealthBarWidgetComponent)
	{
		HealthBarWidgetComponent->SetHiddenInGame(!bShouldShow);
		HealthBarWidgetComponent->SetVisibility(bShouldShow);
	}
}

void ULSEnemyHealthBarComponent::HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshHealthBar();
	RefreshVisibility();
}

void ULSEnemyHealthBarComponent::HandleMaxHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	RefreshHealthBar();
	RefreshVisibility();
}

bool ULSEnemyHealthBarComponent::ResolveHealth(float& OutCurrentHealth, float& OutMaxHealth) const
{
	OutCurrentHealth = 0.0f;
	OutMaxHealth = 0.0f;

	const ALSEnemyCharacter* Enemy = ObservedEnemy.Get();
	const ULSCombatAttributeSet* CombatAttributeSet = Enemy ? Enemy->GetCombatAttributeSet() : nullptr;
	if (!CombatAttributeSet)
	{
		return false;
	}

	OutCurrentHealth = CombatAttributeSet->GetCurrentHealth();
	OutMaxHealth = CombatAttributeSet->GetMaxHealth();
	return OutMaxHealth > 0.0f;
}

bool ULSEnemyHealthBarComponent::IsEnemyHealthBarProtocolVisible(APlayerController* LocalPlayerController) const
{
	int32 CurrentLevel = 0;
	int32 PreviousLevel = 0;
	if (const ALSPlayerControllerBase* LSPlayerController = Cast<ALSPlayerControllerBase>(LocalPlayerController))
	{
		if (LSPlayerController->HasProtocolTestLevel(ELSProtocolType::Battle))
		{
			CurrentLevel = LSPlayerController->GetProtocolTestLevel(ELSProtocolType::Battle);
			PreviousLevel = CurrentLevel;
		}
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (CurrentLevel <= 0 && PreviousLevel <= 0)
	{
		const ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
		if (SaveSubsystem)
		{
			const int32 InactiveSlotCount = LSChipStats::ResolveInactiveSignalSlotCount(SaveSubsystem->GetChipSignalGaugePercent());
			const TArray<FLSSessionItem> ActiveItems = LSChipStats::BuildSignalActiveEquipmentItems(SaveSubsystem->GetChipEquipmentSlots(), InactiveSlotCount);
			CurrentLevel = LSChipStats::AggregateChipProtocolTotals(ActiveItems, this).Battle;
			PreviousLevel = LSChipStats::AggregateChipProtocolTotals(SaveSubsystem->GetChipEquipmentSlots(), this).Battle;
		}
	}

	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return CurrentLevel >= 5;
	}

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(
		ELSProtocolType::Battle,
		TEXT("Enemy_Health_Bar"),
		TEXT("EnemyHealthBar"));
	if (!Row)
	{
		Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(
			ELSProtocolType::Battle,
			TEXT("Enemy_HP"),
			TEXT("EnemyHealthBar"));
	}

	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : CurrentLevel >= 5;
}

APlayerController* ULSEnemyHealthBarComponent::FindLocalPlayerController() const
{
	const UWorld* World = GetWorld();
	if (!World || World->GetNetMode() == NM_DedicatedServer)
	{
		return nullptr;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (PlayerController && PlayerController->IsLocalPlayerController())
		{
			return PlayerController;
		}
	}
	return nullptr;
}

ULSEnemyHealthBarWidget* ULSEnemyHealthBarComponent::GetHealthBarWidget() const
{
	return HealthBarWidgetComponent ? Cast<ULSEnemyHealthBarWidget>(HealthBarWidgetComponent->GetWidget()) : nullptr;
}
