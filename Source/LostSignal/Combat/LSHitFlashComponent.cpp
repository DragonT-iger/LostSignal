#include "Combat/LSHitFlashComponent.h"

#include "AbilitySystemComponent.h"
#include "Characters/LSCharacterBase.h"
#include "Combat/LSCharacterCombatComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/LSCombatAttributeSet.h"
#include "LostSignal.h"
#include "Materials/MaterialInstanceDynamic.h"

ULSHitFlashComponent::ULSHitFlashComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(false);
}

void ULSHitFlashComponent::BeginPlay()
{
	Super::BeginPlay();

	CreateOverlayMaterialInstance();
	BindToOwnerASC();
}

void ULSHitFlashComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindFromOwnerASC();

	if (TargetMeshComponent && OverlayMaterialInstance)
	{
		TargetMeshComponent->SetOverlayMaterial(OriginalOverlayMaterial);
	}

	Super::EndPlay(EndPlayReason);
}

void ULSHitFlashComponent::TickComponent(
	const float DeltaTime,
	const ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	FlashTimeRemaining = FMath::Max(FlashTimeRemaining - DeltaTime, 0.0f);
	SetFlashIntensity(FlashTimeRemaining / FlashDuration);

	if (FlashTimeRemaining <= 0.0f)
	{
		SetComponentTickEnabled(false);
	}
}

void ULSHitFlashComponent::CreateOverlayMaterialInstance()
{
	const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	TargetMeshComponent = OwnerCharacter ? OwnerCharacter->GetMesh() : nullptr;
	if (!TargetMeshComponent)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot play hit flash because owner %s has no skeletal mesh."),
			*GetNameSafe(this), *GetNameSafe(GetOwner()));
		return;
	}

	// 컴포넌트 오버라이드가 없으면 메시 애셋의 오버레이로 폴백해 해석된다.
	OriginalOverlayMaterial = TargetMeshComponent->GetOverlayMaterial();
	if (!OriginalOverlayMaterial)
	{
		UE_LOG(LogLS, Warning, TEXT("%s cannot play hit flash because %s has no overlay material (outline material expected)."),
			*GetNameSafe(this), *GetNameSafe(TargetMeshComponent));
		return;
	}

	OverlayMaterialInstance = UMaterialInstanceDynamic::Create(OriginalOverlayMaterial, this);
	if (!OverlayMaterialInstance)
	{
		return;
	}

	OverlayMaterialInstance->SetVectorParameterValue(HitFlashColorParamName, FlashColor);
	OverlayMaterialInstance->SetScalarParameterValue(HitFlashIntensityParamName, 0.0f);
	TargetMeshComponent->SetOverlayMaterial(OverlayMaterialInstance);
}

void ULSHitFlashComponent::BindToOwnerASC()
{
	const ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	UAbilitySystemComponent* ASC = OwnerCharacter ? OwnerCharacter->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	ObservedASC = ASC;
	CurrentHealthChangedHandle = ASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetCurrentHealthAttribute())
		.AddUObject(this, &ULSHitFlashComponent::HandleCurrentHealthChanged);
}

void ULSHitFlashComponent::UnbindFromOwnerASC()
{
	if (UAbilitySystemComponent* ASC = ObservedASC.Get())
	{
		if (CurrentHealthChangedHandle.IsValid())
		{
			ASC->GetGameplayAttributeValueChangeDelegate(ULSCombatAttributeSet::GetCurrentHealthAttribute()).Remove(CurrentHealthChangedHandle);
		}
	}

	CurrentHealthChangedHandle.Reset();
	ObservedASC.Reset();
}

void ULSHitFlashComponent::HandleCurrentHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (ChangeData.NewValue >= ChangeData.OldValue)
	{
		return;
	}

	const ALSCharacterBase* OwnerCharacter = Cast<ALSCharacterBase>(GetOwner());
	const ULSCharacterCombatComponent* CombatComponent = OwnerCharacter ? OwnerCharacter->GetCharacterCombatComponent() : nullptr;
	if (CombatComponent && CombatComponent->IsDead())
	{
		return;
	}

	StartFlash();
}

void ULSHitFlashComponent::StartFlash()
{
	if (!OverlayMaterialInstance)
	{
		return;
	}

	FlashTimeRemaining = FlashDuration;
	SetFlashIntensity(1.0f);
	SetComponentTickEnabled(true);
}

void ULSHitFlashComponent::SetFlashIntensity(const float Intensity) const
{
	if (OverlayMaterialInstance)
	{
		OverlayMaterialInstance->SetScalarParameterValue(HitFlashIntensityParamName, Intensity);
	}
}
