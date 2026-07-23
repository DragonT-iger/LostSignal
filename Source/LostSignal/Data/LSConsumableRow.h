#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "LSConsumableRow.generated.h"

// 소모품 사용 방식.
UENUM(BlueprintType)
enum class ELSConsumableUseType : uint8
{
	Direct,      // 직접 사용(자신/즉발 대상)
	Throwable,   // 던지기(투척 거리·범위 설정)
	Contextual   // 조건부 사용(열쇠 등 특정 장소·조건)
};

// 소모품 입력 방식.
UENUM(BlueprintType)
enum class ELSConsumableInputType : uint8
{
	Down,   // 누르는 순간 입력 완료
	Hold,   // 누르고 있는 동안 조준/진행
	Toggle  // 입력마다 활성/비활성 전환
};

// 소모품 적용 범위 형태. 스킬의 ELSCharacterSkillRangeShape와 의미는 같으나,
// 기획 테이블 표기(Sphere)를 따라 소모품 도메인 전용으로 둔다.
UENUM(BlueprintType)
enum class ELSConsumableRangeShape : uint8
{
	None,    // 범위 없음(자신/단일 대상)
	Sphere,  // 구/원형: X=반경, Z=판정 높이
	Cone,    // 부채꼴: X=길이, Y=각도, Z=판정 높이
	Box      // 사각형: X=길이, Y=너비, Z=높이
};

// 효과 종류. Attribute=스탯 수치 변경, Status=상태 효과 부여·제거.
UENUM(BlueprintType)
enum class ELSConsumableEffectType : uint8
{
	Attribute,
	Status
};

// 처리 방식. Add/Subtract는 Attribute, Apply/Remove는 Status에 쓴다.
UENUM(BlueprintType)
enum class ELSConsumableEffectOperation : uint8
{
	Add,
	Subtract,
	Apply,
	Remove
};

// 효과 적용 대상.
UENUM(BlueprintType)
enum class ELSConsumableEffectTarget : uint8
{
	Self,     // 사용자 자신
	Enemy,    // 적(범위/조준 대상)
	Friendly, // 자신 포함 아군(미지원 — 정책 정해지면 확장)
	All       // 진영 구분 없이(미지원 — 정책 정해지면 확장)
};

// 적용 스탯. Health는 ULSCombatAttributeSet, 그 외는 ULSCharacterAttributeSet 소속.
// 현재 즉발 GE 경로는 Health/Stamina만 지원(Attack/Defense/MoveSpeed는 지속 버프=Status 권장).
UENUM(BlueprintType)
enum class ELSConsumableAttribute : uint8
{
	None,
	Health,
	Stamina,
	Attack,
	Defense,
	MoveSpeed
};

// 수치 계산 방식. Flat=고정값, Percent=비율. Status 효과는 None.
UENUM(BlueprintType)
enum class ELSConsumableValueType : uint8
{
	None,
	Flat,
	Percent
};

// 반복 적용 설정. Once=1회, Periodic=주기 반복.
UENUM(BlueprintType)
enum class ELSConsumableApplyType : uint8
{
	Once,
	Periodic
};

/**
 * 적용효과 사전 DataTable Row(DT_ConsumableEffect) — "어떻게 작동하는가".
 * RowName은 의미 있는 효과 ID(예: "Heal", "Damage_AOE", "RemoveBleeding")이며, 소모품이 이 이름으로 참조한다.
 * "얼마나"(수치)는 여기 두지 않고, 참조하는 소모품의 FLSConsumableEffectValue.Effect_Value가 전달한다.
 */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSConsumableEffectRow : public FTableRowBase
{
	GENERATED_BODY()

	// Attribute=스탯 수치 변경, Status=상태 효과 부여·제거.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	ELSConsumableEffectType Consumable_Effect_Type = ELSConsumableEffectType::Attribute;

	// Add/Subtract=Attribute, Apply/Remove=Status.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	ELSConsumableEffectOperation Consumable_Effect_Operation = ELSConsumableEffectOperation::Add;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	ELSConsumableEffectTarget Consumable_Effect_Target = ELSConsumableEffectTarget::Self;

	// Attribute 효과의 대상 스탯. Status 효과는 None.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	ELSConsumableAttribute Consumable_Effect_Attribute = ELSConsumableAttribute::None;

	// Flat=고정값, Percent=비율. Status 효과는 None.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	ELSConsumableValueType Consumable_Effect_Value_Type = ELSConsumableValueType::None;

	// Once=1회, Periodic=주기 반복.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	ELSConsumableApplyType Consumable_Effect_Apply_Type = ELSConsumableApplyType::Once;

	// Periodic일 때 적용 주기(초). Once면 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	float Consumable_Effect_Interval = 0.0f;

	// 반복/상태 효과의 지속시간(초). Status 적용 시 >0이면 지속시간 override, 0이면 status row 기본 정책.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	float Consumable_Effect_Duration = 0.0f;

	// Status 효과가 참조하는 DT_StatusEffect의 RowName(정수). 엑셀의 Status_Effect_Name을 int32로 둔다. Attribute 효과는 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	int32 Consumable_Status_Effect_Name = 0;

	// 에디터 가독용 설명(선택, 엑셀 '설명' 대응).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	FText Consumable_Effect_Desc;
};

/**
 * 소모품 Row의 효과 배열 원소 — "얼마나".
 * 적용효과 사전(DT_ConsumableEffect)의 Row를 이름으로 참조하고, 아이템별 실제 수치만 전달한다.
 */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSConsumableEffectValue
{
	GENERATED_BODY()

	// DT_ConsumableEffect의 RowName 참조(예: "Heal").
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	FName Effect_ID;

	// SetByCaller magnitude(InstantAttribute). 상태 부여형은 미사용(0).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable/Effect")
	float Effect_Value = 0.0f;
};

/**
 * 소모품 DataTable Row(DT_Consumable) — 표시 정보 + 거동을 함께 소유한다.
 * RowName은 `Consumable_` 접두사(Weapon/Armor/Chip Row와 동일한 타입별 분리 컨벤션).
 * 표시 컬럼(Item_Text/Type/Max/Description/Cost)은 FLSItemRow와 같은 구조로 미러해,
 * 아이템슬롯·툴팁이 접두사 분기로 이 테이블에서 직접 조회한다(더는 DT_Item에 의존하지 않음).
 * 수치는 이 테이블이 단일 출처이며, 효과 규칙은 DT_StatusEffect가 소유한다.
 */
USTRUCT(BlueprintType)
struct LOSTSIGNAL_API FLSConsumableRow : public FTableRowBase
{
	GENERATED_BODY()

	// --- 아이템 공통 표시 정보 (FLSItemRow와 동일 구조; 무기/방어구/칩 Row와 같은 컨벤션) ---

	// 아이템 이름 출력용 텍스트
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	FText Item_Text;

	// 아이템 타입(소모품 분류값). 기획 데이터가 지정한다.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	int32 Item_Type = 3;

	// 최대 스택 수량
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	int32 Item_Max = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	FText Item_Description;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	int32 Item_Cost = 0;

	// --- 소모품 거동 ---

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	ELSConsumableUseType Item_Use_Type = ELSConsumableUseType::Direct;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	ELSConsumableInputType Item_Input_Type = ELSConsumableInputType::Down;

	// 사용 완료까지 걸리는 시간(초). 즉시 완료는 0.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	float Item_Cast_Time = 0.0f;

	// 사용 중 캐릭터 이동 가능 여부.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	bool Item_Can_Move = false;

	// 사용 완료 후 실제 효과가 적용되기까지의 지연(초).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	float Item_Trigger_Delay = 0.0f;

	// 사용/투척 가능 거리(Unreal Unit, cm).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	float Item_Cast_Range = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	ELSConsumableRangeShape Item_Range_Shape = ELSConsumableRangeShape::None;

	// Range_Shape별 크기값. 예: Sphere 반경(X)/높이(Z), Cone 길이(X)/각도(Y)/높이(Z).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	float Item_Range_X = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	float Item_Range_Y = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	float Item_Range_Z = 0.0f;

	// 적용 효과 목록. 각 원소는 DT_ConsumableEffect 참조 + 수치. 복합 효과(응급키트=회복+출혈제거)를 위해 배열.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="LS/Consumable")
	TArray<FLSConsumableEffectValue> Item_Effects;
};
