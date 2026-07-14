#include "Skills/LSPlayerSkillComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Characters/Enemys/LSEnemyCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Core/LSPlayerControllerBase.h"
#include "Data/LSCharacterSkillRow.h"
#include "Data/LSChipStats.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolTypes.h"
#include "Data/LSProtocolUnlockRow.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/RootMotionSource.h"
#include "GAS/Abilities/Character1/LSGA_Bypass.h"
#include "GAS/Abilities/Character1/LSGA_Execution.h"
#include "GAS/LSCharacterAttributeSet.h"
#include "GAS/LSGameplayTags.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LostSignal.h"
#include "Session/LSSaveSubsystem.h"
#include "Session/LSSkillCastSettingsSubsystem.h"
#include "Skills/LSDashSkillDataAsset.h"
#include "Skills/LSPassiveSkillDataAsset.h"
#include "Skills/LSSkillDataAsset.h"
#include "Skills/LSSkillPoolDataAsset.h"
#include "Skills/Preview/LSSkillPreviewComponent.h"
#include "TimerManager.h"

namespace
{
	void ApplySkillRowRangeToPreviewSpec(const FLSCharacterSkillRow& Row, FLSSkillAreaPreviewSpec& InOutSpec)
	{
		switch (Row.Range_Shape)
		{
		case ELSCharacterSkillRangeShape::Cone:
			InOutSpec.Shape = ELSSkillAreaShape::Circle;
			InOutSpec.LocationMode = ELSSkillPreviewLocationMode::CasterOrigin;
			InOutSpec.Radius = Row.Range_X;
			InOutSpec.Degrees = Row.Range_Y;
			break;

		case ELSCharacterSkillRangeShape::Circle:
			InOutSpec.Shape = ELSSkillAreaShape::Circle;
			InOutSpec.Radius = Row.Range_X;
			InOutSpec.Degrees = 360.0f;
			break;

		case ELSCharacterSkillRangeShape::Box:
			InOutSpec.Shape = ELSSkillAreaShape::Box;
			InOutSpec.LocationMode = ELSSkillPreviewLocationMode::CasterOrigin;
			InOutSpec.BoxLength = Row.Range_X;
			InOutSpec.BoxWidth = Row.Range_Y;
			if (InOutSpec.LocationOffset.IsNearlyZero() && Row.Range_X > 0.0f)
			{
				InOutSpec.LocationOffset.X = Row.Range_X * 0.5f;
			}
			break;

		case ELSCharacterSkillRangeShape::None:
		default:
			break;
		}

		InOutSpec.WorldZOffset = 0.0f;
	}
}

ULSPlayerSkillComponent::ULSPlayerSkillComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

bool ULSPlayerSkillComponent::BeginSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!CanUseLocalPreview())
	{
		return false;
	}

	ULSSkillDataAsset* SkillData = GetSkillData(Slot);
	ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent();
	if (!SkillData)
	{
		return false;
	}

	if (IsSkillCooldownActive(SkillData))
	{
		LogSkillCooldownBlocked(SkillData, TEXT("Preview"));
		return false;
	}

	if (ActiveSkillData)
	{
		if (PreviewComponent)
		{
			PreviewComponent->EndAreaPreview();
		}
	}

	ActiveSkillData = nullptr;
	ActiveSlot = Slot;

	if (IsSkillRangeProtocolVisible())
	{
		if (!PreviewComponent || !PreviewComponent->BeginAreaPreview(BuildPreviewSpecForSkill(SkillData)))
		{
			return false;
		}
	}

	ActiveSkillData = SkillData;
	return true;
}

void ULSPlayerSkillComponent::UpdateActiveSkillPreview(const FVector& WorldLocation, const FRotator& WorldRotation)
{
	if (!ActiveSkillData)
	{
		return;
	}

	if (ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent())
	{
		PreviewComponent->UpdateAreaPreview(ClampTargetLocationToCastRange(ActiveSkillData, WorldLocation), WorldRotation);
	}
}

bool ULSPlayerSkillComponent::ConfirmActiveSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!ActiveSkillData || ActiveSlot != Slot)
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to confirm skill preview. ActiveSkill=%s ActiveSlot=%d RequestedSlot=%d"),
			*GetNameSafe(GetOwner()),
			*GetNameSafe(ActiveSkillData),
			static_cast<int32>(ActiveSlot),
			static_cast<int32>(Slot));
		return false;
	}

	if (ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent())
	{
		PreviewComponent->EndAreaPreview();
	}

	ActiveSkillData = nullptr;
	return true;
}

bool ULSPlayerSkillComponent::ConfirmAnyActiveSkillPreview(const FVector& TargetLocation, const FRotator& AimRotation)
{
	if (!ActiveSkillData)
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to confirm any skill preview because ActiveSkillData is missing."),
			*GetNameSafe(GetOwner()));
		return false;
	}

	const ELSPlayerSkillSlot SlotToActivate = ActiveSlot;
	ULSSkillDataAsset* SkillData = ActiveSkillData;

	const bool bConfirmed = ConfirmActiveSkillPreview(SlotToActivate);
	if (!bConfirmed)
	{
		return false;
	}

	return CommitSkillActivation(SlotToActivate, SkillData, TargetLocation, AimRotation);
}

bool ULSPlayerSkillComponent::ActivateSkillInstant(ELSPlayerSkillSlot Slot, const FVector& TargetLocation, const FRotator& AimRotation)
{
	if (!CanUseLocalPreview())
	{
		return false;
	}

	ULSSkillDataAsset* SkillData = GetSkillData(Slot);
	if (!SkillData)
	{
		return false;
	}

	if (IsSkillCooldownActive(SkillData))
	{
		LogSkillCooldownBlocked(SkillData, TEXT("Instant"));
		return false;
	}

	// 다른 슬롯 프리뷰가 진행 중이면 정리하고 즉시 발동한다.
	if (ActiveSkillData)
	{
		CancelAnyActiveSkillPreview();
	}

	return CommitSkillActivation(Slot, SkillData, TargetLocation, AimRotation);
}

bool ULSPlayerSkillComponent::CommitSkillActivation(ELSPlayerSkillSlot Slot, ULSSkillDataAsset* SkillData, const FVector& TargetLocation, const FRotator& AimRotation)
{
	if (!SkillData)
	{
		return false;
	}

	const FVector ClampedTargetLocation = ClampTargetLocationToCastRange(SkillData, TargetLocation);

	if (const AActor* OwnerActor = GetOwner())
	{
		if (OwnerActor->HasAuthority())
		{
			return ActivateSkillOnServer(Slot, ClampedTargetLocation, AimRotation.Yaw);
		}

		TryPredictFastMovementSkill(SkillData, ClampedTargetLocation, AimRotation.Yaw);
		ServerRequestActivateSkill(Slot, ClampedTargetLocation, AimRotation.Yaw);
	}

	return true;
}

void ULSPlayerSkillComponent::CancelActiveSkillPreview(ELSPlayerSkillSlot Slot)
{
	if (!ActiveSkillData || ActiveSlot != Slot)
	{
		return;
	}

	CancelAnyActiveSkillPreview();
}

void ULSPlayerSkillComponent::CancelAnyActiveSkillPreview()
{
	if (ULSSkillPreviewComponent* PreviewComponent = ResolvePreviewComponent())
	{
		PreviewComponent->EndAreaPreview();
	}

	ActiveSkillData = nullptr;
}

ELSSkillCastMode ULSPlayerSkillComponent::GetEffectiveCastMode(ELSPlayerSkillSlot Slot) const
{
	if (bOverrideCastModeForDebug)
	{
		if (const ELSSkillCastMode* Override = DebugCastModeOverrides.Find(Slot))
		{
			return *Override;
		}
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const ULSSkillCastSettingsSubsystem* CastSettings = GameInstance ? GameInstance->GetSubsystem<ULSSkillCastSettingsSubsystem>() : nullptr;
	return CastSettings ? CastSettings->GetSlotCastMode(Slot) : ELSSkillCastMode::QuickCastWithIndicator;
}

ULSSkillDataAsset* ULSPlayerSkillComponent::GetSkillData(ELSPlayerSkillSlot Slot) const
{
	const FLSPlayerSkillSlotSpec* SlotSpec = SkillSlots.Find(Slot);
	return SlotSpec ? SlotSpec->SkillData.Get() : nullptr;
}

bool ULSPlayerSkillComponent::SetSkillData(ELSPlayerSkillSlot Slot, ULSSkillDataAsset* NewSkillData)
{
	if (!NewSkillData)
	{
		return false;
	}

	if (ActiveSkillData && ActiveSlot == Slot)
	{
		CancelAnyActiveSkillPreview();
	}

	FLSPlayerSkillSlotSpec& SlotSpec = SkillSlots.FindOrAdd(Slot);
	SlotSpec.SkillData = NewSkillData;

	UE_LOG(LogLS, Log, TEXT("%s changed skill slot data. Slot=%d Skill=%s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(Slot),
		*GetNameSafe(NewSkillData));

	return true;
}

bool ULSPlayerSkillComponent::ApplySkillEnhancementByIndex(ELSPlayerSkillSlot Slot, int32 EnhancementIndex)
{
	ULSSkillDataAsset* CurrentSkillData = GetSkillData(Slot);
	ULSSkillDataAsset* EnhancedSkillData = CurrentSkillData ? CurrentSkillData->GetEnhancementVariant(EnhancementIndex) : nullptr;
	if (!EnhancedSkillData)
	{
		UE_LOG(LogLS, Warning, TEXT("%s failed to apply skill enhancement. Slot=%d Index=%d CurrentSkill=%s"),
			*GetNameSafe(GetOwner()),
			static_cast<int32>(Slot),
			EnhancementIndex,
			*GetNameSafe(CurrentSkillData));
		return false;
	}

	return SetSkillData(Slot, EnhancedSkillData);
}

bool ULSPlayerSkillComponent::GetActivePreviewSpec(FLSSkillAreaPreviewSpec& OutPreviewSpec) const
{
	if (!ActiveSkillData)
	{
		return false;
	}

	OutPreviewSpec = BuildPreviewSpecForSkill(ActiveSkillData);
	return true;
}

void ULSPlayerSkillComponent::HandleBasicAttackHit(int32 ComboIndex, int32 ComboAttackID, int32 ValidHitCount)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority() || ValidHitCount <= 0)
	{
		return;
	}

	for (ULSPassiveSkillDataAsset* PassiveSkillData : PassiveSkills)
	{
		if (!PassiveSkillData)
		{
			continue;
		}

		TrySendPassiveGameplayEvent(PassiveSkillData, ComboIndex, ComboAttackID);
	}
}

bool ULSPlayerSkillComponent::ConsumePendingAbilityContext(TSubclassOf<UGameplayAbility> AbilityClass, FLSSkillActivationContext& OutContext)
{
	if (!AbilityClass || PendingAbilityClass != AbilityClass || !PendingAbilityContext.SkillData)
	{
		return false;
	}

	OutContext = PendingAbilityContext;
	PendingAbilityContext = FLSSkillActivationContext();
	PendingAbilityClass = nullptr;
	return true;
}

bool ULSPlayerSkillComponent::ApplySkillCooldown(const ULSSkillDataAsset* SkillData) const
{
	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!OwnerActor || !OwnerActor->HasAuthority() || !ASC || !SkillData)
	{
		return false;
	}

	const FGameplayTag CooldownTag = SkillData->GetCooldownTag();
	const float BaseDuration = ResolveSkillCooldownDuration(SkillData);
	if (!CooldownTag.IsValid() || BaseDuration <= 0.0f || !SkillData->CooldownEffectClass)
	{
		return false;
	}

	const float FinalDuration = ResolveReducedSkillCooldownDuration(BaseDuration);
	if (FinalDuration <= 0.0f)
	{
		return false;
	}

	FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
	EffectContext.AddSourceObject(SkillData);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(SkillData->CooldownEffectClass, 1.0f, EffectContext);
	if (!SpecHandle.IsValid())
	{
		return false;
	}

	SpecHandle.Data->SetDuration(FinalDuration, true);
	SpecHandle.Data->DynamicGrantedTags.AddTag(CooldownTag);
	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	UE_LOG(LogLS, Log, TEXT("%s applied skill cooldown. Skill=%s Tag=%s Duration=%.2f Base=%.2f Reduction=%.2f"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(SkillData),
		*CooldownTag.ToString(),
		FinalDuration,
		BaseDuration,
		ASC->GetNumericAttribute(ULSCharacterAttributeSet::GetCooldownReductionAttribute()));

	return true;
}

bool ULSPlayerSkillComponent::IsSkillCooldownActive(const ULSSkillDataAsset* SkillData) const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	const FGameplayTag CooldownTag = SkillData ? SkillData->GetCooldownTag() : FGameplayTag();
	return ASC && CooldownTag.IsValid() && ASC->HasMatchingGameplayTag(CooldownTag);
}

float ULSPlayerSkillComponent::GetSkillCooldownRemaining(const ULSSkillDataAsset* SkillData) const
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	const FGameplayTag CooldownTag = SkillData ? SkillData->GetCooldownTag() : FGameplayTag();
	if (!ASC || !CooldownTag.IsValid())
	{
		return 0.0f;
	}

	FGameplayTagContainer QueryTags;
	QueryTags.AddTag(CooldownTag);

	float RemainingTime = 0.0f;
	for (const TPair<float, float>& TimePair : ASC->GetActiveEffectsTimeRemainingAndDuration(FGameplayEffectQuery::MakeQuery_MatchAllOwningTags(QueryTags)))
	{
		RemainingTime = FMath::Max(RemainingTime, TimePair.Key);
	}

	return RemainingTime;
}

float ULSPlayerSkillComponent::GetSkillCooldownTotalDuration(const ULSSkillDataAsset* SkillData) const
{
	const float ConfiguredDuration = ResolveReducedSkillCooldownDuration(ResolveSkillCooldownDuration(SkillData));
	if (ConfiguredDuration > 0.0f)
	{
		return ConfiguredDuration;
	}

	// 대쉬처럼 설정 총시간(스킬 행/FallbackCooldown)이 없으면, 실제 활성 쿨타임 GE의 지속시간을 사용해
	// 진행바 분모가 항상 실제 쿨타임과 일치하도록 한다. 기존 스킬(설정 총시간 > 0)은 이 경로를 타지 않는다.
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	const FGameplayTag CooldownTag = SkillData ? SkillData->GetCooldownTag() : FGameplayTag();
	if (!ASC || !CooldownTag.IsValid())
	{
		return 0.0f;
	}

	FGameplayTagContainer QueryTags;
	QueryTags.AddTag(CooldownTag);

	float TotalDuration = 0.0f;
	for (const TPair<float, float>& TimePair : ASC->GetActiveEffectsTimeRemainingAndDuration(FGameplayEffectQuery::MakeQuery_MatchAllOwningTags(QueryTags)))
	{
		TotalDuration = FMath::Max(TotalDuration, TimePair.Value);
	}

	return TotalDuration;
}

void ULSPlayerSkillComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyEquippedSkillLoadout();
}

void ULSPlayerSkillComponent::ApplyEquippedSkillLoadout()
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	// 서버 권한(발동 판정) 또는 로컬 조종 클라(프리뷰/UI)에서 적용한다. 데디 서버의 비소유 캐릭터는 건너뛴다.
	const bool bShouldApply = OwnerPawn && (OwnerPawn->HasAuthority() || OwnerPawn->IsLocallyControlled());
	if (!bShouldApply)
	{
		return;
	}

	const UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	ULSSaveSubsystem* SaveSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSSaveSubsystem>() : nullptr;
	if (!SaveSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot apply equipped skill loadout because SaveSubsystem is missing."), *GetNameSafe(GetOwner()));
		return;
	}

	if (!SkillPool)
	{
		// 풀이 없으면 캐릭터별 로드아웃을 조회할 수 없다 → BP 기본 SkillSlots를 폴백 기본 로드아웃으로 유지한다.
		return;
	}

	const TArray<int32>& EquippedSkillIDs = SaveSubsystem->GetEquippedSkillIDs(SkillPool->CharacterID);
	const bool bHasAnyEquipped = EquippedSkillIDs.ContainsByPredicate([](int32 SkillID) { return SkillID != 0; });
	if (!bHasAnyEquipped)
	{
		// 저장된 선택이 하나도 없으면(신규/미선택) BP 기본 SkillSlots를 폴백 기본 로드아웃으로 유지한다.
		return;
	}

	static const ELSPlayerSkillSlot SlotOrder[] = { ELSPlayerSkillSlot::Skill1, ELSPlayerSkillSlot::Skill2, ELSPlayerSkillSlot::Skill3, ELSPlayerSkillSlot::Skill4 };
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(SlotOrder); ++Index)
	{
		const ELSPlayerSkillSlot Slot = SlotOrder[Index];
		const int32 SkillID = EquippedSkillIDs.IsValidIndex(Index) ? EquippedSkillIDs[Index] : 0;
		ULSSkillDataAsset* SkillData = (SkillID != 0) ? SkillPool->FindSkillByID(SkillID) : nullptr;

		if (SkillID != 0 && !SkillData)
		{
			UE_LOG(LogLS, Warning, TEXT("%s could not resolve equipped Skill_ID %d from SkillPool for slot %d."),
				*GetNameSafe(GetOwner()), SkillID, static_cast<int32>(Slot));
		}

		if (SkillData)
		{
			SetSkillData(Slot, SkillData);
		}
		else
		{
			ClearSkillSlot(Slot);
		}
	}
}

void ULSPlayerSkillComponent::ClearSkillSlot(ELSPlayerSkillSlot Slot)
{
	if (ActiveSkillData && ActiveSlot == Slot)
	{
		CancelAnyActiveSkillPreview();
	}

	if (FLSPlayerSkillSlotSpec* SlotSpec = SkillSlots.Find(Slot))
	{
		SlotSpec->SkillData = nullptr;
	}
}

void ULSPlayerSkillComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FinishPredictedFastMovementSkill();
	Super::EndPlay(EndPlayReason);
}

bool ULSPlayerSkillComponent::CanUseLocalPreview() const
{
	if (GetNetMode() == NM_DedicatedServer)
	{
		return false;
	}

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	return OwnerPawn && OwnerPawn->IsLocallyControlled();
}

bool ULSPlayerSkillComponent::IsSkillRangeProtocolVisible() const
{
	if (bAlwaysShowSkillPreviewDebug)
	{
		return true;
	}

	int32 CurrentLevel = 0;
	int32 PreviousLevel = 0;
	ResolveBattleProtocolLevels(CurrentLevel, PreviousLevel);

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		return CurrentLevel >= 2;
	}

	const FLSProtocolUnlockRow* Row = GameDataSubsystem->FindProtocolUnlockRowByEnableName(
		ELSProtocolType::Battle,
		TEXT("Skill_Range"),
		TEXT("PlayerSkillRangeProtocol"));
	return Row ? GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel) : CurrentLevel >= 2;
}

void ULSPlayerSkillComponent::ResolveBattleProtocolLevels(int32& OutCurrentLevel, int32& OutPreviousLevel) const
{
	OutCurrentLevel = 0;
	OutPreviousLevel = 0;

	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	const ALSPlayerControllerBase* PlayerController = OwnerPawn ? Cast<ALSPlayerControllerBase>(OwnerPawn->GetController()) : nullptr;
	if (PlayerController && PlayerController->HasProtocolTestLevel(ELSProtocolType::Battle))
	{
		OutCurrentLevel = PlayerController->GetProtocolTestLevel(ELSProtocolType::Battle);
		OutPreviousLevel = OutCurrentLevel;
		return;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
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

ULSSkillPreviewComponent* ULSPlayerSkillComponent::ResolvePreviewComponent() const
{
	return GetOwner() ? GetOwner()->FindComponentByClass<ULSSkillPreviewComponent>() : nullptr;
}

void ULSPlayerSkillComponent::ServerRequestActivateSkill_Implementation(ELSPlayerSkillSlot Slot, FVector_NetQuantize TargetLocation, float AimYaw)
{
	ActivateSkillOnServer(Slot, FVector(TargetLocation), AimYaw);
}

bool ULSPlayerSkillComponent::ActivateSkillOnServer(ELSPlayerSkillSlot Slot, const FVector& TargetLocation, float AimYaw)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	ULSSkillDataAsset* SkillData = GetSkillData(Slot);
	if (!SkillData)
	{
		return false;
	}

	const FLSCharacterSkillRow* SkillRow = ResolveActiveSkillRow(SkillData, TEXT("ActivateSkillOnServer"));
	if (!SkillRow)
	{
		UE_LOG(LogLS, Warning, TEXT("%s skill activation rejected because %s has no active skill row. RowName=%s"),
			*GetNameSafe(OwnerActor),
			*GetNameSafe(SkillData),
			*SkillData->GetSkillRowName().ToString());
		return false;
	}

	if (IsSkillCooldownActive(SkillData))
	{
		LogSkillCooldownBlocked(SkillData, TEXT("ServerActivate"));
		return false;
	}

	const FVector ClampedTargetLocation = ClampTargetLocationToCastRange(SkillData, TargetLocation);

	FLSSkillActivationContext Context;
	Context.SourceActor = OwnerActor;
	Context.SkillData = SkillData;
	Context.TargetLocation = ClampedTargetLocation;
	Context.AimYaw = AimYaw;
	Context.SkillRow = *SkillRow;
	Context.bHasSkillRow = true;

	if (SkillData->GetAbilityClass())
	{
		return TryActivateGameplayAbility(SkillData, Context);
	}

	UE_LOG(LogLS, Warning, TEXT("%s skill activation rejected because %s has no GAS AbilityClass."),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(SkillData));
	return false;
}

const FLSCharacterSkillRow* ULSPlayerSkillComponent::ResolveActiveSkillRow(const ULSSkillDataAsset* SkillData, const TCHAR* Context) const
{
	if (!SkillData || SkillData->GetSkillID() <= 0)
	{
		return nullptr;
	}

	const UWorld* World = GetWorld();
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	return GameDataSubsystem ? GameDataSubsystem->FindActiveSkillRowByID(SkillData->GetSkillID(), Context) : nullptr;
}

FLSSkillAreaPreviewSpec ULSPlayerSkillComponent::BuildPreviewSpecForSkill(const ULSSkillDataAsset* SkillData) const
{
	FLSSkillAreaPreviewSpec ResolvedSpec = SkillData ? SkillData->BuildPreviewSpec() : FLSSkillAreaPreviewSpec();
	if (const FLSCharacterSkillRow* Row = ResolveActiveSkillRow(SkillData, TEXT("BuildPreviewSpecForSkill")))
	{
		ApplySkillRowRangeToPreviewSpec(*Row, ResolvedSpec);
	}

	return ResolvedSpec;
}

float ULSPlayerSkillComponent::ResolveSkillCooldownDuration(const ULSSkillDataAsset* SkillData) const
{
	// 대쉬처럼 쿨타임 소스를 캐릭터 어트리뷰트로 지정한 스킬은 스킬 테이블 대신 그 어트리뷰트에서 읽는다.
	if (const ULSDashSkillDataAsset* DashData = Cast<ULSDashSkillDataAsset>(SkillData); DashData && DashData->CooldownAttribute.IsValid())
	{
		const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
		return ASC ? ASC->GetNumericAttribute(DashData->CooldownAttribute) : 0.0f;
	}

	if (const FLSCharacterSkillRow* Row = ResolveActiveSkillRow(SkillData, TEXT("ResolveSkillCooldownDuration")); Row && Row->Skill_Cooldown > 0.0f)
	{
		return Row->Skill_Cooldown;
	}

	return SkillData ? SkillData->GetCooldownDuration() : 0.0f;
}

float ULSPlayerSkillComponent::ResolveReducedSkillCooldownDuration(const float BaseDuration) const
{
	if (BaseDuration <= 0.0f)
	{
		return 0.0f;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
	const float CooldownReduction = ASC ? ASC->GetNumericAttribute(ULSCharacterAttributeSet::GetCooldownReductionAttribute()) : 0.0f;
	const float ReductionRatio = CooldownReduction > 1.0f ? CooldownReduction * 0.01f : CooldownReduction;
	return BaseDuration * FMath::Clamp(1.0f - ReductionRatio, 0.0f, 1.0f);
}

FVector ULSPlayerSkillComponent::ClampTargetLocationToCastRange(const ULSSkillDataAsset* SkillData, const FVector& TargetLocation) const
{
	const FLSCharacterSkillRow* Row = ResolveActiveSkillRow(SkillData, TEXT("ClampTargetLocationToCastRange"));
	if (!Row || Row->Cast_Range <= 0.0f)
	{
		return TargetLocation;
	}

	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return TargetLocation;
	}

	const FVector OwnerLocation = OwnerActor->GetActorLocation();
	FVector ToTarget = TargetLocation - OwnerLocation;
	ToTarget.Z = 0.0f;
	const float Distance = ToTarget.Size2D();
	if (Distance <= Row->Cast_Range || Distance <= KINDA_SMALL_NUMBER)
	{
		return TargetLocation;
	}

	FVector ClampedLocation = OwnerLocation + (ToTarget / Distance * Row->Cast_Range);
	ClampedLocation.Z = TargetLocation.Z;
	return ClampedLocation;
}

void ULSPlayerSkillComponent::LogSkillCooldownBlocked(const ULSSkillDataAsset* SkillData, const TCHAR* Phase) const
{
	const AActor* OwnerActor = GetOwner();
	const FGameplayTag CooldownTag = SkillData ? SkillData->GetCooldownTag() : FGameplayTag();
	UE_LOG(LogLS, Log, TEXT("%s skill blocked by cooldown. Phase=%s Skill=%s Tag=%s Remaining=%.2f"),
		*GetNameSafe(OwnerActor),
		Phase ? Phase : TEXT("Unknown"),
		*GetNameSafe(SkillData),
		CooldownTag.IsValid() ? *CooldownTag.ToString() : TEXT("None"),
		GetSkillCooldownRemaining(SkillData));
}

bool ULSPlayerSkillComponent::TryActivateGameplayAbility(ULSSkillDataAsset* SkillData, const FLSSkillActivationContext& Context)
{
	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	const TSubclassOf<UGameplayAbility> AbilityClass = SkillData ? SkillData->GetAbilityClass() : nullptr;
	if (!OwnerActor || !OwnerActor->HasAuthority() || !ASC || !AbilityClass)
	{
		return false;
	}

	bool bHasAbility = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
		{
			bHasAbility = true;
			break;
		}
	}

	if (!bHasAbility)
	{
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}

	PendingAbilityContext = Context;
	PendingAbilityClass = AbilityClass;

	const bool bActivated = ASC->TryActivateAbilityByClass(AbilityClass);
	if (!bActivated)
	{
		PendingAbilityContext = FLSSkillActivationContext();
		PendingAbilityClass = nullptr;
	}
	else
	{
		// 발동 성공 시 시전음 Cue 발동. 서버 권한 경로라 멀티캐스트로 전 클라가 재생한다.
		PlaySkillCastCue(SkillData);
	}

	return bActivated;
}

void ULSPlayerSkillComponent::PlaySkillCastCue(const ULSSkillDataAsset* SkillData) const
{
	if (!SkillData || !SkillData->CastSound)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	if (!OwnerActor || !ASC)
	{
		return;
	}

	FGameplayCueParameters CueParams;
	CueParams.SourceObject = SkillData->CastSound.Get();
	CueParams.Location = OwnerActor->GetActorLocation();
	CueParams.Instigator = OwnerActor;

	ASC->ExecuteGameplayCue(LSGameplayTags::GameplayCue_Skill_Cast, CueParams);
}

bool ULSPlayerSkillComponent::TrySendPassiveGameplayEvent(ULSPassiveSkillDataAsset* SkillData, int32 ComboIndex, int32 ComboAttackID) const
{
	AActor* OwnerActor = GetOwner();
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);
	const TSubclassOf<UGameplayAbility> AbilityClass = SkillData ? SkillData->GetAbilityClass() : nullptr;
	if (!OwnerActor || !OwnerActor->HasAuthority() || !ASC || !AbilityClass)
	{
		return false;
	}

	bool bHasAbility = false;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->GetClass() == AbilityClass)
		{
			bHasAbility = true;
			break;
		}
	}

	if (!bHasAbility)
	{
		ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1));
	}

	FGameplayEventData EventData;
	EventData.EventTag = LSGameplayTags::Event_Combat_BasicAttackHit;
	EventData.Instigator = OwnerActor;
	EventData.Target = OwnerActor;
	EventData.OptionalObject = SkillData;
	EventData.EventMagnitude = static_cast<float>(ComboAttackID > 0 ? ComboAttackID : ComboIndex);
	ASC->HandleGameplayEvent(LSGameplayTags::Event_Combat_BasicAttackHit, &EventData);

	return true;
}

bool ULSPlayerSkillComponent::TryPredictFastMovementSkill(ULSSkillDataAsset* SkillData, const FVector& TargetLocation, float AimYaw)
{
	const TSubclassOf<UGameplayAbility> AbilityClass = SkillData ? SkillData->GetAbilityClass() : nullptr;
	if (!AbilityClass || bPredictedFastMovementInProgress || IsSkillCooldownActive(SkillData))
	{
		return false;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || OwnerCharacter->HasAuthority() || !OwnerCharacter->IsLocallyControlled())
	{
		return false;
	}

	float Distance = 0.0f;
	float Duration = 0.0f;
	if (!ResolvePredictedFastMovementParams(SkillData, Distance, Duration))
	{
		return false;
	}

	FVector AimDirection = TargetLocation - OwnerCharacter->GetActorLocation();
	AimDirection.Z = 0.0f;
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = FRotator(0.0f, AimYaw, 0.0f).Vector();
	}

	AimDirection = AimDirection.GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = OwnerCharacter->GetActorForwardVector().GetSafeNormal2D();
	}

	UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement();
	if (AimDirection.IsNearlyZero() || !MovementComponent)
	{
		return false;
	}

	IgnoreEnemiesForPredictedFastMovement(OwnerCharacter, OwnerCharacter->GetActorLocation(), AimDirection, Distance);

	TSharedPtr<FRootMotionSource_ConstantForce> RootMotion = MakeShared<FRootMotionSource_ConstantForce>();
	RootMotion->InstanceName = FName("PredictedFastMovementSkill");
	RootMotion->AccumulateMode = ERootMotionAccumulateMode::Override;
	RootMotion->Priority = 5;
	RootMotion->Force = AimDirection * (Distance / Duration);
	RootMotion->Duration = Duration;
	RootMotion->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::SetVelocity;
	RootMotion->FinishVelocityParams.SetVelocity = FVector::ZeroVector;

	PredictedFastMovementRootMotionSourceID = MovementComponent->ApplyRootMotionSource(RootMotion);
	bPredictedFastMovementInProgress = true;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PredictedFastMovementTimerHandle);
		World->GetTimerManager().SetTimer(PredictedFastMovementTimerHandle, this, &ULSPlayerSkillComponent::FinishPredictedFastMovementSkill, Duration, false);
	}

	return true;
}

bool ULSPlayerSkillComponent::ResolvePredictedFastMovementParams(ULSSkillDataAsset* SkillData, float& OutDistance, float& OutDuration) const
{
	const TSubclassOf<UGameplayAbility> AbilityClass = SkillData ? SkillData->GetAbilityClass() : nullptr;
	if (!AbilityClass)
	{
		return false;
	}

	if (AbilityClass->IsChildOf(ULSGA_Bypass::StaticClass()))
	{
		const ULSGA_Bypass* BypassCDO = AbilityClass->GetDefaultObject<ULSGA_Bypass>();
		return BypassCDO && BypassCDO->ResolveMovementParams(SkillData, ResolveActiveSkillRow(SkillData, TEXT("ResolvePredictedFastMovementParams")), OutDistance, OutDuration);
	}

	if (AbilityClass->IsChildOf(ULSGA_Execution::StaticClass()))
	{
		const ULSGA_Execution* ExecutionCDO = AbilityClass->GetDefaultObject<ULSGA_Execution>();
		return ExecutionCDO && ExecutionCDO->ResolveMovementParams(SkillData, ResolveActiveSkillRow(SkillData, TEXT("ResolvePredictedFastMovementParams")), OutDistance, OutDuration);
	}

	return false;
}

void ULSPlayerSkillComponent::IgnoreEnemiesForPredictedFastMovement(ACharacter* OwnerCharacter, const FVector& StartLocation, const FVector& Direction, float Distance)
{
	if (!OwnerCharacter || OwnerCharacter->HasAuthority() || Distance <= 0.0f || Direction.IsNearlyZero())
	{
		return;
	}

	UCapsuleComponent* OwnerCapsule = OwnerCharacter->GetCapsuleComponent();
	if (!OwnerCapsule)
	{
		return;
	}

	TArray<AActor*> OverlappedActors;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	const float PathHalfLength = Distance * 0.5f;
	const float PathHalfWidth = OwnerCapsule->GetScaledCapsuleRadius() * 2.0f;
	const FVector AreaCenter = StartLocation + (Direction * PathHalfLength);
	const float QueryRadius = FMath::Sqrt(FMath::Square(PathHalfLength) + FMath::Square(PathHalfWidth));

	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		AreaCenter,
		QueryRadius,
		ObjectTypes,
		ALSEnemyCharacter::StaticClass(),
		ActorsToIgnore,
		OverlappedActors);

	const FVector RightDirection = FVector::CrossProduct(FVector::UpVector, Direction).GetSafeNormal2D();
	for (AActor* EnemyActor : OverlappedActors)
	{
		if (!EnemyActor)
		{
			continue;
		}

		const FVector ToEnemy = EnemyActor->GetActorLocation() - StartLocation;
		const float ForwardDistance = FVector::DotProduct(ToEnemy, Direction);
		const float LateralDistance = FMath::Abs(FVector::DotProduct(ToEnemy, RightDirection));
		if (ForwardDistance < 0.0f || ForwardDistance > Distance || LateralDistance > PathHalfWidth)
		{
			continue;
		}

		OwnerCapsule->IgnoreActorWhenMoving(EnemyActor, true);
		OwnerCharacter->MoveIgnoreActorAdd(EnemyActor);

		if (ACharacter* EnemyCharacter = Cast<ACharacter>(EnemyActor))
		{
			if (UCapsuleComponent* EnemyCapsule = EnemyCharacter->GetCapsuleComponent())
			{
				EnemyCapsule->IgnoreActorWhenMoving(OwnerCharacter, true);
			}
			EnemyCharacter->MoveIgnoreActorAdd(OwnerCharacter);
		}

		PredictedFastMovementIgnoredEnemies.AddUnique(EnemyActor);
	}
}

void ULSPlayerSkillComponent::ClearIgnoredEnemiesForPredictedFastMovement(ACharacter* OwnerCharacter)
{
	if (!OwnerCharacter)
	{
		PredictedFastMovementIgnoredEnemies.Reset();
		return;
	}

	if (UCapsuleComponent* OwnerCapsule = OwnerCharacter->GetCapsuleComponent())
	{
		for (const TWeakObjectPtr<AActor>& IgnoredActor : PredictedFastMovementIgnoredEnemies)
		{
			if (AActor* EnemyActor = IgnoredActor.Get())
			{
				OwnerCapsule->IgnoreActorWhenMoving(EnemyActor, false);
			}
		}
	}

	for (const TWeakObjectPtr<AActor>& IgnoredActor : PredictedFastMovementIgnoredEnemies)
	{
		AActor* EnemyActor = IgnoredActor.Get();
		if (!EnemyActor)
		{
			continue;
		}

		OwnerCharacter->MoveIgnoreActorRemove(EnemyActor);
		if (ACharacter* EnemyCharacter = Cast<ACharacter>(EnemyActor))
		{
			if (UCapsuleComponent* EnemyCapsule = EnemyCharacter->GetCapsuleComponent())
			{
				EnemyCapsule->IgnoreActorWhenMoving(OwnerCharacter, false);
			}
			EnemyCharacter->MoveIgnoreActorRemove(OwnerCharacter);
		}
	}

	PredictedFastMovementIgnoredEnemies.Reset();
}

void ULSPlayerSkillComponent::FinishPredictedFastMovementSkill()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(PredictedFastMovementTimerHandle);
	}

	if (bPredictedFastMovementInProgress)
	{
		if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
		{
			ClearIgnoredEnemiesForPredictedFastMovement(OwnerCharacter);

			if (UCharacterMovementComponent* MovementComponent = OwnerCharacter->GetCharacterMovement())
			{
				MovementComponent->RemoveRootMotionSourceByID(PredictedFastMovementRootMotionSourceID);
			}
		}
	}

	PredictedFastMovementRootMotionSourceID = 0;
	PredictedFastMovementIgnoredEnemies.Reset();
	bPredictedFastMovementInProgress = false;
}
