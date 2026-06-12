#include "MiniGame/RatSteal/LSRatPlayer.h"

#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "LostSignal.h"
#include "MiniGame/RatSteal/LSRatCrop.h"
#include "MiniGame/RatSteal/LSRatGameMode.h"
#include "MiniGame/RatSteal/LSRatInventoryComponent.h"
#include "MiniGame/RatSteal/LSRatYSortComponent.h"
#include "PaperFlipbookComponent.h"

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
	// 원작 플레이어 스케일 0.35 (512px 원본 → 약 180px)
	Sprite->SetRelativeScale3D(FVector(0.35f, 1.f, 0.35f));

	// 직교 추종 카메라 (23_System_Camera). 스프링암 랙으로 부드러운 추종
	CameraArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraArm"));
	CameraArm->SetupAttachment(CollisionBox);
	CameraArm->SetUsingAbsoluteRotation(true);
	CameraArm->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	CameraArm->TargetArmLength = 2000.f;
	CameraArm->bDoCollisionTest = false;
	CameraArm->bEnableCameraLag = true;
	CameraArm->CameraLagSpeed = 5.f;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(CameraArm);
	Camera->SetProjectionMode(ECameraProjectionMode::Orthographic);
	// 원작 D2D 화면 1920px = 월드 1920uu (1px=1uu)
	Camera->SetOrthoWidth(1920.f);

	Inventory = CreateDefaultSubobject<ULSRatInventoryComponent>(TEXT("Inventory"));
	YSort = CreateDefaultSubobject<ULSRatYSortComponent>(TEXT("YSort"));

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

	if (Sprite && IdleFlipbook)
	{
		Sprite->SetFlipbook(IdleFlipbook);
	}
}

void ALSRatPlayer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (InvincibleRemaining > 0.f)
	{
		InvincibleRemaining -= DeltaSeconds;
	}

	const ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	const bool bPlaying = GameMode && GameMode->IsPlaying();

	if (bPlaying && IsAlive())
	{
		TickMovement(DeltaSeconds);

		if (GameMode->IsFullnessDecayEnabled())
		{
			TickFullness(DeltaSeconds);
		}
	}

	MoveInput = FVector2D::ZeroVector;
}

void ALSRatPlayer::TickMovement(float DeltaSeconds)
{
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

	FVector NewLocation = GetActorLocation() + FVector(Dir.X, 0.f, Dir.Y) * Speed * DeltaSeconds;
	NewLocation.X = FMath::Clamp(NewLocation.X, -MapHalfExtent.X, MapHalfExtent.X);
	NewLocation.Z = FMath::Clamp(NewLocation.Z, -MapHalfExtent.Y, MapHalfExtent.Y);
	SetActorLocation(NewLocation);

	if (Sprite)
	{
		if (WalkFlipbook && Sprite->GetFlipbook() != WalkFlipbook)
		{
			Sprite->SetFlipbook(WalkFlipbook);
		}

		// 좌우 플립 (스프라이트 X스케일 부호)
		if (!FMath::IsNearlyZero(Dir.X))
		{
			FVector NewScale = Sprite->GetRelativeScale3D();
			NewScale.X = FMath::Abs(NewScale.X) * (Dir.X < 0.f ? -1.f : 1.f);
			Sprite->SetRelativeScale3D(NewScale);
		}
	}
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
		Nearest->NotifyStolen();
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
}

void ALSRatPlayer::ExitBush()
{
	BushOverlapCount = FMath::Max(0, BushOverlapCount - 1);
}
