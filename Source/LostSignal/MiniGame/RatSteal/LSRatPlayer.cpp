#include "MiniGame/RatSteal/LSRatPlayer.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Materials/MaterialInterface.h"
#include "MiniGame/RatSteal/LSRatCrop.h"
#include "MiniGame/RatSteal/LSRatGameMode.h"
#include "MiniGame/RatSteal/LSRatInventoryComponent.h"
#include "MiniGame/RatSteal/LSRatThrownCrop.h"
#include "MiniGame/RatSteal/LSRatYSortComponent.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"

ALSRatPlayer::ALSRatPlayer()
{
	PrimaryActorTick.bCanEverTick = true;

	// 원작 콜라이더 150x150 (픽셀). X-Z 평면, Y는 깊이라 얇게
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(FVector(75.f, 10.f, 75.f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_Pawn);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	CollisionBox->SetGenerateOverlapEvents(true);
	SetRootComponent(CollisionBox);

	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(CollisionBox);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetRelativeLocation(FVector(0.f, -4.f, 0.f));
	// 원작 플레이어 스케일 0.35 (512px 원본 → 약 180px)
	Sprite->SetRelativeScale3D(FVector(0.35f, 1.f, 0.35f));
	ApplyRatSpriteMaterial();

	// 직교 추종 카메라 (23_System_Camera). 스프링암 랙으로 부드러운 추종
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(CollisionBox);
	CameraArm->SetUsingAbsoluteRotation(true);
	CameraArm->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	CameraArm->TargetArmLength = 2000.f;
	CameraArm->bDoCollisionTest = false;
	CameraArm->bEnableCameraLag = false;
	CameraArm->CameraLagSpeed = 5.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	// 플레이 기준 줌인 값. 원작 1920uu에서 시작했으나 현재 스프라이트 스케일에 맞춰 조정.
	Camera->SetOrthoWidth(950.f);
	ApplyRatCameraPostProcess();
	ApplyCameraBounds();

	Inventory = CreateDefaultSubobject<ULSRatInventoryComponent>(TEXT("Inventory"));
	YSort = CreateDefaultSubobject<ULSRatYSortComponent>(TEXT("YSort"));
	YSort->SortOffset = 30;
	ThrownCropClass = ALSRatThrownCrop::StaticClass();

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void ALSRatPlayer::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogLS, Log, TEXT("[RatSteal] Player BeginPlay: %s Location=%s Controller=%s"),
		*GetName(),
		*GetActorLocation().ToCompactString(),
		*GetNameSafe(GetController()));

	Hp = MaxHp;
	Fullness = MaxFullness;
	OnHpChanged.Broadcast(Hp);
	OnFullnessChanged.Broadcast(Fullness, MaxFullness);
	ResolveDefaultAssets();

	if (YSort)
	{
		YSort->SortOffset = 30;
	}

	if (Camera)
	{
		Camera->SetOrthoWidth(950.f);
		ApplyRatCameraPostProcess();
		ApplyCameraBounds();
	}

	if (Sprite)
	{
		Sprite->SetRelativeLocation(FVector(0.f, -4.f, 0.f));
		ApplyRatSpriteMaterial();
	}

	if (Sprite && IdleFlipbook)
	{
		Sprite->SetFlipbook(IdleFlipbook);
	}
}

void ALSRatPlayer::ApplyRatCameraPostProcess()
{
	if (!Camera)
	{
		return;
	}

	Camera->PostProcessBlendWeight = 1.f;

	FPostProcessSettings& Settings = Camera->PostProcessSettings;
	Settings.bOverride_LocalExposureHighlightContrastScale = true;
	Settings.LocalExposureHighlightContrastScale = 0.f;
	Settings.bOverride_LocalExposureShadowContrastScale = true;
	Settings.LocalExposureShadowContrastScale = 0.f;
	Settings.bOverride_BloomIntensity = true;
	Settings.BloomIntensity = 0.f;
	Settings.bOverride_VignetteIntensity = true;
	Settings.VignetteIntensity = 0.f;
}

void ALSRatPlayer::ApplyRatSpriteMaterial()
{
	if (!Sprite)
	{
		return;
	}

	if (!RatSpriteMaterial)
	{
		RatSpriteMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			TEXT("/Paper2D/TranslucentUnlitSpriteMaterial.TranslucentUnlitSpriteMaterial"));
	}

	if (RatSpriteMaterial)
	{
		Sprite->SetMaterial(0, RatSpriteMaterial);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] RatSpriteMaterial missing. Player sprite may be affected by scene lighting."));
	}
}

void ALSRatPlayer::ApplyCameraBounds()
{
	if (!CameraArm || !Camera)
	{
		return;
	}

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		PlayerController->GetViewportSize(ViewportX, ViewportY);
	}

	const float AspectRatio = ViewportY > 0
		? static_cast<float>(ViewportX) / static_cast<float>(ViewportY)
		: Camera->AspectRatio;
	const float SafeAspectRatio = AspectRatio > SMALL_NUMBER ? AspectRatio : (16.f / 9.f);
	const float HalfViewX = Camera->OrthoWidth * 0.5f;
	const float HalfViewZ = HalfViewX / SafeAspectRatio;
	const float MaxCenterX = FMath::Max(0.f, MapHalfExtent.X - HalfViewX);
	const float MaxCenterZ = FMath::Max(0.f, MapHalfExtent.Y - HalfViewZ);

	const FVector PlayerLocation = GetActorLocation();
	const float CameraCenterX = FMath::Clamp(PlayerLocation.X, -MaxCenterX, MaxCenterX);
	const float CameraCenterZ = FMath::Clamp(PlayerLocation.Z, -MaxCenterZ, MaxCenterZ);
	CameraArm->SetRelativeLocation(FVector(CameraCenterX - PlayerLocation.X, 0.f, CameraCenterZ - PlayerLocation.Z));
}

void ALSRatPlayer::ResolveDefaultAssets()
{
	if (!HitFlipbook)
	{
		HitFlipbook = LoadObject<UPaperFlipbook>(
			nullptr,
			TEXT("/Game/LostSignal/MiniGame/RatSteal/Flipbooks/FB_Mole_hit.FB_Mole_hit"));

		if (!HitFlipbook)
		{
			UE_LOG(LogLS, Warning, TEXT("[RatSteal] HitFlipbook missing. Assign FB_Mole_hit on BP_RatPlayer."));
		}
	}

	if (!HitSound)
	{
		HitSound = LoadObject<USoundBase>(
			nullptr,
			TEXT("/Game/LostSignal/MiniGame/RatSteal/Imported/Audio/Sounds/SFX/18.18"));

		if (!HitSound)
		{
			UE_LOG(LogLS, Warning, TEXT("[RatSteal] HitSound missing. Assign a hit SFX on BP_RatPlayer."));
		}
	}
}

void ALSRatPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (InvincibleRemaining > 0.f)
	{
		InvincibleRemaining -= DeltaSeconds;
	}
	if (StealAnimRemaining > 0.f)
	{
		StealAnimRemaining -= DeltaSeconds;
		if (StealAnimRemaining <= 0.f && Sprite && IdleFlipbook)
		{
			Sprite->SetFlipbook(IdleFlipbook);
		}
	}
	if (HitAnimRemaining > 0.f)
	{
		HitAnimRemaining -= DeltaSeconds;
		if (HitAnimRemaining <= 0.f && Sprite && IdleFlipbook)
		{
			Sprite->SetFlipbook(IdleFlipbook);
		}
	}

	const ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	const bool bPlaying = GameMode && GameMode->IsPlaying();

	if (bPlaying && IsAlive())
	{
		TickMovement(DeltaSeconds);
		TickThrowDash(DeltaSeconds);
		ApplyCameraBounds();

		if (GameMode->IsFullnessDecayEnabled())
		{
			TickFullness(DeltaSeconds);
		}
	}

	MoveInput = FVector2D::ZeroVector;
}

void ALSRatPlayer::TickMovement(float DeltaSeconds)
{
	if (!MoveInput.IsNearlyZero())
	{
		FaceHorizontalInput(MoveInput.X);
	}

	if (StealAnimRemaining > 0.f || HitAnimRemaining > 0.f)
	{
		return;
	}

	if (MoveInput.IsNearlyZero())
	{
		if (Sprite && IdleFlipbook && Sprite->GetFlipbook() != IdleFlipbook)
		{
			Sprite->SetFlipbook(IdleFlipbook);
		}
		return;
	}

	const FVector2D Dir = MoveInput.GetSafeNormal();
	const float Speed = GetCurrentMoveSpeed();
	LastMoveDirection = FVector(Dir.X, 0.f, Dir.Y).GetSafeNormal();

	FVector NewLocation = GetActorLocation() + LastMoveDirection * Speed * DeltaSeconds;
	NewLocation.X = FMath::Clamp(NewLocation.X, -MapHalfExtent.X, MapHalfExtent.X);
	NewLocation.Z = FMath::Clamp(NewLocation.Z, -MapHalfExtent.Y, MapHalfExtent.Y);
	SetActorLocation(NewLocation);

	if (Sprite)
	{
		if (WalkFlipbook && Sprite->GetFlipbook() != WalkFlipbook)
		{
			Sprite->SetFlipbook(WalkFlipbook);
		}
	}
}

void ALSRatPlayer::FaceHorizontalInput(float InputX)
{
	if (!Sprite || FMath::IsNearlyZero(InputX))
	{
		return;
	}

	FVector NewScale = Sprite->GetRelativeScale3D();
	NewScale.X = FMath::Abs(NewScale.X) * (InputX < 0.f ? -1.f : 1.f);
	Sprite->SetRelativeScale3D(NewScale);
}

void ALSRatPlayer::TickThrowDash(float DeltaSeconds)
{
	if (ThrowDashRemaining <= 0.f || ThrowDashDirection.IsNearlyZero())
	{
		return;
	}

	ThrowDashRemaining -= DeltaSeconds;
	FVector NewLocation = GetActorLocation() + ThrowDashDirection * ThrowDashSpeed * DeltaSeconds;
	NewLocation.X = FMath::Clamp(NewLocation.X, -MapHalfExtent.X, MapHalfExtent.X);
	NewLocation.Z = FMath::Clamp(NewLocation.Z, -MapHalfExtent.Y, MapHalfExtent.Y);
	SetActorLocation(NewLocation);
}

void ALSRatPlayer::TickFullness(float DeltaSeconds)
{
	// 원작 Player::Update — 2초마다 fullness -= 20 (고정값, 가속 없음)
	FullnessElapsed += DeltaSeconds;
	if (FullnessElapsed < FullnessDecayInterval)
	{
		return;
	}

	FullnessElapsed = 0.f;
	Fullness = FMath::Max(0.f, Fullness - FullnessDecayAmount);
	OnFullnessChanged.Broadcast(Fullness, MaxFullness);

	if (Fullness <= 0.f)
	{
		if (ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>())
		{
			GameMode->EndGame(ELSRatEndReason::BabyStarved);
		}
	}
}

float ALSRatPlayer::GetCurrentMoveSpeed() const
{
	const float Multiplier = Inventory ? Inventory->GetSpeedMultiplier() : 1.f;
	return FMath::Max(BaseMoveSpeed / Multiplier, MinMoveSpeed);
}

void ALSRatPlayer::TrySteal()
{
	const ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	if (!IsAlive() || (GameMode && !GameMode->IsPlaying()))
	{
		return;
	}

	TArray<AActor*> Overlapped;
	GetOverlappingActors(Overlapped, ALSRatCrop::StaticClass());

	// 여러 작물과 겹치면 가장 가까운 1개 (03_Controls)
	ALSRatCrop* Nearest = nullptr;
	float NearestDistSq = TNumericLimits<float>::Max();
	for (AActor* Actor : Overlapped)
	{
		ALSRatCrop* Crop = Cast<ALSRatCrop>(Actor);
		if (!Crop || !Crop->IsStealable())
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(Crop->GetActorLocation(), GetActorLocation());
		if (DistSq < NearestDistSq)
		{
			NearestDistSq = DistSq;
			Nearest = Crop;
		}
	}

	if (Nearest && Inventory)
	{
		Inventory->AddCrop(Nearest->GetCropType(), Nearest->GetCropSize());
		PlaySfx(StealSound);
		PlayStealAnimation();
		Nearest->NotifyStolen();
	}
}

void ALSRatPlayer::TryThrowItem()
{
	const ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	if (!IsAlive() || (GameMode && !GameMode->IsPlaying()) || !Inventory)
	{
		return;
	}

	const ELSRatCropType Type = Inventory->GetCurrentSlotCropType();
	if (!Inventory->ThrowItem())
	{
		return;
	}

	ThrowDashDirection = LastMoveDirection.IsNearlyZero() ? FVector(1.f, 0.f, 0.f) : LastMoveDirection;
	ThrowDashRemaining = ThrowDashDuration;
	PlaySfx(ThrowSound);
	SpawnThrownCropVisual(Type);
}

void ALSRatPlayer::PlayStealAnimation()
{
	if (!Sprite || !StealFlipbook)
	{
		return;
	}

	StealAnimRemaining = StealAnimDuration;
	Sprite->SetFlipbook(StealFlipbook);
	Sprite->PlayFromStart();
}

void ALSRatPlayer::PlayHitAnimation()
{
	if (!Sprite || !HitFlipbook)
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] Hit animation skipped. Sprite=%s HitFlipbook=%s"),
			*GetNameSafe(Sprite.Get()), *GetNameSafe(HitFlipbook.Get()));
		return;
	}

	StealAnimRemaining = 0.f;
	HitAnimRemaining = HitAnimDuration;
	Sprite->SetFlipbook(HitFlipbook);
	Sprite->PlayFromStart();
}

void ALSRatPlayer::SpawnThrownCropVisual(ELSRatCropType Type)
{
	if (!ThrownCropClass || Type == ELSRatCropType::None)
	{
		return;
	}

	const TObjectPtr<UPaperSprite>* SpriteAsset = ThrowSprites.Find(Type);
	if (!SpriteAsset || !SpriteAsset->Get())
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] Throw visual sprite missing Type=%s"),
			*UEnum::GetValueAsString(Type));
		return;
	}

	const FVector ThrowDirection = ThrowDashDirection.IsNearlyZero() ? FVector(-1.f, 0.f, 0.f) : -ThrowDashDirection;
	const FVector SpawnLocation = GetActorLocation() + ThrowDirection * 90.f + FVector(0.f, -25.f, 95.f);
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ALSRatThrownCrop* ThrownCrop = GetWorld()->SpawnActor<ALSRatThrownCrop>(
		ThrownCropClass, SpawnLocation, FRotator::ZeroRotator, Params);
	if (ThrownCrop)
	{
		ThrownCrop->InitThrownCrop(SpriteAsset->Get(), SpawnLocation, ThrowDirection);
		UE_LOG(LogLS, Log, TEXT("[RatSteal] Throw visual spawned Type=%s Sprite=%s"),
			*UEnum::GetValueAsString(Type), *GetNameSafe(SpriteAsset->Get()));
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] Throw visual spawn failed Type=%s"),
			*UEnum::GetValueAsString(Type));
	}
}

void ALSRatPlayer::PlaySfx(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void ALSRatPlayer::SubmitAndFeed()
{
	if (!Inventory)
	{
		return;
	}

	ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	if (!GameMode)
	{
		return;
	}

	// 원작 Player::OnTriggerEnter(SubMissionArea) — 점수 = 베이비 회복량
	const int32 Score = GameMode->SubmitInventory(Inventory->SubmitAll());
	if (Score > 0)
	{
		PlaySfx(SubmitSound);
		FeedBaby(static_cast<float>(Score));
	}
}

void ALSRatPlayer::FeedBaby(float Amount)
{
	Fullness = FMath::Min(Fullness + Amount, MaxFullness);
	OnFullnessChanged.Broadcast(Fullness, MaxFullness);
}

void ALSRatPlayer::ApplyHit()
{
	if (!IsAlive() || InvincibleRemaining > 0.f)
	{
		return;
	}

	Hp--;
	InvincibleRemaining = InvincibleDuration;
	OnHpChanged.Broadcast(Hp);
	PlaySfx(HitSound);
	PlayHitAnimation();
	UE_LOG(LogLS, Log, TEXT("[RatSteal] 플레이어 피격. HP %d"), Hp);

	if (Hp <= 0)
	{
		Die();
	}
}

void ALSRatPlayer::Die()
{
	if (ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>())
	{
		GameMode->EndGame(ELSRatEndReason::PlayerDead);
	}
}

void ALSRatPlayer::EnterBush()
{
	BushOverlapCount++;
	if (BushOverlapCount == 1 && Sprite)
	{
		Sprite->SetSpriteColor(FLinearColor(1.f, 1.f, 1.f, HiddenOpacity));
	}
}

void ALSRatPlayer::ExitBush()
{
	BushOverlapCount = FMath::Max(0, BushOverlapCount - 1);
	if (BushOverlapCount == 0 && Sprite)
	{
		Sprite->SetSpriteColor(FLinearColor::White);
	}
}
