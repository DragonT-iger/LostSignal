#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MiniGame/RatSteal/LSRatTypes.h"
#include "LSRatPlayer.generated.h"

class UBoxComponent;
class UCameraComponent;
class UMaterialInterface;
class UPaperFlipbook;
class UPaperFlipbookComponent;
class UPaperSprite;
class USoundBase;
class USpringArmComponent;
class ULSRatInventoryComponent;
class ULSRatYSortComponent;
class ALSRatThrownCrop;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FLSRatOnHpChanged, int32, NewHp);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLSRatOnFullnessChanged, float, Fullness, float, MaxFullness);

/**
 * 쥐 플레이어 (10_Entity_Player).
 * X-Z 평면 이동(Y=깊이), 직교 추종 카메라 부착.
 * HP3 / 포만 1000(2초당 -20) / 이동 500~50(인벤토리 감속식) — 모든 수치 원작 그대로.
 */
UCLASS()
class LOSTSIGNAL_API ALSRatPlayer : public APawn
{
	GENERATED_BODY()

public:
	ALSRatPlayer();

	virtual void Tick(float DeltaSeconds) override;

	/** 컨트롤러가 매 프레임 전달하는 이동 입력 (정규화 전) */
	void SetMoveInput(const FVector2D& Input) { MoveInput = Input; }

	/** Z키: 겹친 작물 중 가장 가까운 것 훔치기 (03_Controls) */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void TrySteal();

	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void TryThrowItem();

	/** 제출존 진입 시: 인벤토리 정산 → 점수 → FeedBaby(점수) */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void SubmitAndFeed();

	/** 농부 공격 적중. 무적 중이면 무시, HP-1 후 무적 (원작 ivc_T=15프레임 기준) */
	UFUNCTION(BlueprintCallable, Category = "LS/RatSteal")
	void ApplyHit();

	/** 부쉬 Overlap에서 호출 (15_Mechanic_Stealth) */
	void EnterBush();
	void ExitBush();

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	bool IsHidden() const { return BushOverlapCount > 0; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	bool IsAlive() const { return Hp > 0; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	int32 GetHp() const { return Hp; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	float GetFullness() const { return Fullness; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	float GetMaxFullness() const { return MaxFullness; }

	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	ULSRatInventoryComponent* GetInventory() const { return Inventory; }

	/** 현재 이동속도 = max(기본속도 / SpeedMultiplier, 최소속도) */
	UFUNCTION(BlueprintPure, Category = "LS/RatSteal")
	float GetCurrentMoveSpeed() const;

	UPROPERTY(BlueprintAssignable, Category = "LS/RatSteal")
	FLSRatOnHpChanged OnHpChanged;

	UPROPERTY(BlueprintAssignable, Category = "LS/RatSteal")
	FLSRatOnFullnessChanged OnFullnessChanged;

protected:
	virtual void BeginPlay() override;

	void TickFullness(float DeltaSeconds);
	void TickMovement(float DeltaSeconds);
	void TickThrowDash(float DeltaSeconds);
	void FaceHorizontalInput(float InputX);
	void PlayStealAnimation();
	void PlayHitAnimation();
	void SpawnThrownCropVisual(ELSRatCropType Type);
	void PlaySfx(USoundBase* Sound) const;
	void FeedBaby(float Amount);
	void Die();
	void ApplyRatSpriteMaterial();

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UBoxComponent> CollisionBox;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UPaperFlipbookComponent> Sprite;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<USpringArmComponent> CameraArm;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<ULSRatInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, Category = "LS/RatSteal")
	TObjectPtr<ULSRatYSortComponent> YSort;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Visual")
	TObjectPtr<UMaterialInterface> RatSpriteMaterial;

	// ---- 밸런스 (50_Content_Balance, 원작 그대로) ----

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	int32 MaxHp = 3;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float BaseMoveSpeed = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float MinMoveSpeed = 50.f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float MaxFullness = 1000.f;

	/** 포만 감소 주기/량: 2초당 20 고정 (가속 없음, 14_Mechanic_Hunger) */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float FullnessDecayInterval = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float FullnessDecayAmount = 20.f;

	/** 피격 무적. 원작 ivc_T=15프레임 → 60fps 기준 0.25s, 플레이로 미세조정 */
	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float InvincibleDuration = 0.25f;

	/** 이동 가능 맵 반경 (30_Level_Layout — 픽셀→cm 환산은 플레이로 조정) */
	UPROPERTY(EditAnywhere, Category = "LS/RatSteal|Balance")
	FVector2D MapHalfExtent = FVector2D(3840.f, 2160.f);

	// ---- 애니메이션 (Idle/Walk/Steal/Hit — WBP/에셋 임포트 후 할당) ----

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	TObjectPtr<UPaperFlipbook> IdleFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	TObjectPtr<UPaperFlipbook> WalkFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	TObjectPtr<UPaperFlipbook> StealFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	float StealAnimDuration = 0.35f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	TObjectPtr<UPaperFlipbook> HitFlipbook;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Anim")
	float HitAnimDuration = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Visual")
	TMap<ELSRatCropType, TObjectPtr<UPaperSprite>> ThrowSprites;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Visual")
	TSubclassOf<ALSRatThrownCrop> ThrownCropClass;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float ThrowDashDuration = 0.12f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Balance")
	float ThrowDashSpeed = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Visual", meta = (ClampMin = 0.0, ClampMax = 1.0))
	float HiddenOpacity = 0.55f;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Audio")
	TObjectPtr<USoundBase> StealSound;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Audio")
	TObjectPtr<USoundBase> ThrowSound;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Audio")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(EditDefaultsOnly, Category = "LS/RatSteal|Audio")
	TObjectPtr<USoundBase> SubmitSound;

private:
	void ApplyRatCameraPostProcess();
	void ApplyCameraBounds();
	void ResolveDefaultAssets();

	FVector2D MoveInput = FVector2D::ZeroVector;
	FVector LastMoveDirection = FVector(1.f, 0.f, 0.f);
	FVector ThrowDashDirection = FVector::ZeroVector;

	int32 Hp = 3;
	float Fullness = 1000.f;
	float FullnessElapsed = 0.f;
	float InvincibleRemaining = 0.f;
	float StealAnimRemaining = 0.f;
	float HitAnimRemaining = 0.f;
	float ThrowDashRemaining = 0.f;
	int32 BushOverlapCount = 0;
};
