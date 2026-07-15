// ULSPlayerCombatComponent의 디버그 기본 공격 범위 표시(LS.Debug.BasicAttackRange) 구현 분리 파일.
#include "Combat/LSPlayerCombatComponent.h"

#include "Characters/LSCharacterBase.h"
#include "Data/LSComboAttackRow.h"
#include "GAS/Abilities/Character1/LSGA_PlayerBasicAttack.h"
#include "HAL/IConsoleManager.h"
#include "Skills/LSPlayerSkillComponent.h"
#include "Skills/LSSkillAreaTypes.h"
#include "Skills/Preview/LSSkillPreviewComponent.h"

static TAutoConsoleVariable<int32> CVarLSDebugBasicAttackRange(
	TEXT("LS.Debug.BasicAttackRange"),
	0,
	TEXT("1이면 기본 공격 판정 범위를 스킬 프리뷰 컴포넌트로 표시한다(로컬 플레이어 전용, 0=끔)."),
	ECVF_Cheat);

void ULSPlayerCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateDebugBasicAttackRangePreview();
}

void ULSPlayerCombatComponent::UpdateDebugBasicAttackRangePreview()
{
	const bool bEnabled = CVarLSDebugBasicAttackRange.GetValueOnGameThread() != 0;
	ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	if (!bEnabled || !OwnerCharacter || !OwnerCharacter->IsLocallyControlled())
	{
		EndDebugBasicAttackRangePreview();
		return;
	}

	ULSSkillPreviewComponent* PreviewComponent = OwnerCharacter->FindComponentByClass<ULSSkillPreviewComponent>();
	if (!PreviewComponent)
	{
		return;
	}

	// 실제 스킬 프리뷰가 컴포넌트를 쓰는 동안은 양보한다(스킬 프리뷰 종료 후 다음 틱에 자동 복귀).
	const ULSPlayerSkillComponent* SkillComponent = OwnerCharacter->FindComponentByClass<ULSPlayerSkillComponent>();
	if (SkillComponent && SkillComponent->IsPreviewingSkill())
	{
		bDebugRangePreviewActive = false;
		DebugRangePreviewKey = INDEX_NONE;
		return;
	}

	// 공격 중이면 재생 중인 콤보 섹션, 아니면 1타 기준. (Ability가 ServerOnly라 원격 클라이언트에선 항상 1타 표시)
	const ULSGA_PlayerBasicAttack* ActiveAbility = FindActiveBasicAttackAbility();
	const int32 ComboSectionIndex = ActiveAbility ? ActiveAbility->GetCurrentComboIndex() : 0;
	const int32 ComboTag = ActiveAbility ? ActiveAbility->GetCurrentComboTag() : 0;
	const FLSComboAttackRow* ComboRow = ResolveComboAttackRow(ComboSectionIndex, ComboTag);

	// 재질은 프리뷰 컴포넌트 소유(Circle/BoxMaterial) — Spec.Material은 채우지 않는다. 미할당 경고도 컴포넌트가 남긴다.
	FLSSkillAreaPreviewSpec Spec;
	float ForwardOffset = 0.0f;
	BuildDebugBasicAttackRangeSpec(ComboRow, Spec, ForwardOffset);

	// 표시 중인 row가 바뀌었거나 프리뷰가 꺼져 있으면 재시작(같은 row면 위치/방향만 갱신).
	const int32 PreviewKey = ComboRow ? ComboRow->Combo_ID : INDEX_NONE;
	if (!bDebugRangePreviewActive || !PreviewComponent->IsAreaPreviewActive() || PreviewKey != DebugRangePreviewKey)
	{
		if (!PreviewComponent->BeginAreaPreview(Spec))
		{
			bDebugRangePreviewActive = false;
			return;
		}

		bDebugRangePreviewActive = true;
		DebugRangePreviewKey = PreviewKey;
	}

	const FVector AttackDirection = ResolveBasicAttackDirection();
	const FVector PreviewOrigin = OwnerCharacter->GetActorLocation() + (AttackDirection * ForwardOffset);
	PreviewComponent->UpdateAreaPreview(PreviewOrigin, AttackDirection.Rotation());
}

void ULSPlayerCombatComponent::BuildDebugBasicAttackRangeSpec(const FLSComboAttackRow* ComboRow, FLSSkillAreaPreviewSpec& OutSpec, float& OutForwardOffset) const
{
	// GatherBasicAttackTargets와 동일한 유효성 기준 — 표시가 실제 판정과 어긋나지 않게 한다.
	const bool bUseRowRange = ComboRow
		&& ComboRow->Range_Shape != ELSCharacterSkillRangeShape::None
		&& ComboRow->Range_X > 0.0f;

	OutSpec.LocationMode = ELSSkillPreviewLocationMode::CasterOrigin;
	OutSpec.FillAmount = 1.0f;
	OutForwardOffset = 0.0f;

	if (!bUseRowRange)
	{
		// 판정 폴백(전방 오프셋 고정 구체)을 그대로 표시.
		OutSpec.Shape = ELSSkillAreaShape::Circle;
		OutSpec.Radius = BasicAttackRadius;
		OutSpec.Degrees = 360.0f;
		OutForwardOffset = BasicAttackForwardOffset;
		return;
	}

	if (ComboRow->Range_Shape == ELSCharacterSkillRangeShape::Box)
	{
		OutSpec.Shape = ELSSkillAreaShape::Box;
		OutSpec.BoxLength = ComboRow->Range_X;
		OutSpec.BoxWidth = ComboRow->Range_Y;
		OutSpec.OutlineThickness = 0.2f;
		// 박스 판정은 원점에서 전방으로 뻗지만 프리뷰 메시는 중심 정렬 — 전방 절반만큼 밀어 맞춘다(몬스터 텔레그래프와 동일).
		OutForwardOffset = ComboRow->Range_X * 0.5f;
		return;
	}

	OutSpec.Shape = ELSSkillAreaShape::Circle;
	OutSpec.Radius = ComboRow->Range_X;
	OutSpec.Degrees = ComboRow->Range_Shape == ELSCharacterSkillRangeShape::Cone ? ComboRow->Range_Y : 360.0f;
}

void ULSPlayerCombatComponent::EndDebugBasicAttackRangePreview()
{
	if (!bDebugRangePreviewActive)
	{
		return;
	}

	bDebugRangePreviewActive = false;
	DebugRangePreviewKey = INDEX_NONE;

	const ALSCharacterBase* OwnerCharacter = ResolveOwnerCharacter();
	if (ULSSkillPreviewComponent* PreviewComponent = OwnerCharacter ? OwnerCharacter->FindComponentByClass<ULSSkillPreviewComponent>() : nullptr)
	{
		PreviewComponent->EndAreaPreview();
	}
}
