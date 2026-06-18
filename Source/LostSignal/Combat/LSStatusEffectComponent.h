#pragma once

#include "AttributeSet.h"
#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "GameplayEffectTypes.h"
#include "LSStatusEffectComponent.generated.h"

class AActor;
class UAbilitySystemComponent;
struct FLSStatusEffectRow;
enum class ELSStatusEffectMathType : uint8;

/** 서버 권위에서 적용 중인 한 상태이상의 런타임 상태. 복제하지 않는다(서버 전용). */
USTRUCT()
struct FLSActiveStatusEffect
{
	GENERATED_BODY()

	int32 StackCount = 0;
	FActiveGameplayEffectHandle EffectHandle;
	FTimerHandle ExpiryTimerHandle;
};

/**
 * 상태이상 적용 컴포넌트.
 * Status_ID로 DT_StatusEffect row를 조회해 stat modifier 기반 버프/디버프를 동적 GameplayEffect로 대상 ASC에 적용한다.
 * 모든 ALSCharacterBase가 보유하며(플레이어/몬스터 공용), 적용은 서버 권위에서만 한다.
 * 수치는 GameplayEffect를 거치고(직접 Attribute 수정 금지), 어트리뷰트 복제로 클라이언트 UI에 반영된다.
 *
 * 범위: Stat_Modifiers를 가진 Buff/Debuff 그룹만 처리한다.
 * CC/Tag 그룹(기절·무적 등)은 태그 부여 정책/데이터가 정해지면 확장한다.
 */
UCLASS(ClassGroup=(LS), meta=(BlueprintSpawnableComponent))
class LOSTSIGNAL_API ULSStatusEffectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	ULSStatusEffectComponent();

	/**
	 * Status_ID 상태이상을 소유 액터에 적용한다(서버 전용).
	 * @param StatusID         DT_StatusEffect row 키
	 * @param Instigator       유발 액터(EffectContext SourceObject)
	 * @param DurationOverride  >0이면 지속시간으로 사용, <=0이면 무한(또는 호출자 기본 정책)
	 * @return 실제로 적용/갱신되면 true
	 */
	UFUNCTION(BlueprintCallable, Category="LS/StatusEffect")
	bool ApplyStatusEffectByID(int32 StatusID, AActor* Instigator, float DurationOverride = -1.0f);

	UFUNCTION(BlueprintCallable, Category="LS/StatusEffect")
	bool RemoveStatusEffectByID(int32 StatusID);

	UFUNCTION(BlueprintPure, Category="LS/StatusEffect")
	bool HasStatusEffect(int32 StatusID) const;

private:
	struct FResolvedModifier
	{
		FGameplayAttribute Attribute;
		ELSStatusEffectMathType MathType;
		float Value = 0.0f;
	};

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	// CSV의 Target_Stat 문자열(Char_*/Mon_*)을 어트리뷰트로 변환하는 단일 출처.
	static FGameplayAttribute ResolveAttribute(const FName& TargetStat);

	// Stat_Modifiers 배열 우선, 비어 있으면 평면 Target_Stat/_2 fallback (중복 집계 방지).
	void GatherModifiers(const FLSStatusEffectRow& Row, TArray<FResolvedModifier>& OutModifiers) const;

	// StackCount 기준으로 동적 GE를 만들어 적용하고 핸들을 돌려준다.
	FActiveGameplayEffectHandle BuildAndApplyEffect(
		UAbilitySystemComponent& ASC,
		const TArray<FResolvedModifier>& Modifiers,
		int32 StackCount,
		float Duration,
		AActor* Instigator,
		int32 StatusID) const;

	void HandleStatusEffectExpired(int32 StatusID);

	// StatusID -> 활성 상태(서버 런타임 전용, 복제 안 함).
	TMap<int32, FLSActiveStatusEffect> ActiveStatusEffects;
};
