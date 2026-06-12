#include "MiniGame/RatSteal/LSRatFarmer.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "LostSignal.h"
#include "Materials/MaterialInterface.h"
#include "MiniGame/RatSteal/LSRatAttackIndicator.h"
#include "MiniGame/RatSteal/LSRatGameMode.h"
#include "MiniGame/RatSteal/LSRatPlayer.h"
#include "MiniGame/RatSteal/LSRatYSortComponent.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"

ALSRatFarmer::ALSRatFarmer()
{
	PrimaryActorTick.bCanEverTick = true;

	// 원작 콜라이더 140x400, offset (4, 0)
	CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
	CollisionBox->SetBoxExtent(FVector(70.f, 10.f, 200.f));
	CollisionBox->SetRelativeLocation(FVector(4.f, 0.f, 0.f));
	CollisionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionBox->SetCollisionObjectType(ECC_Pawn);
	CollisionBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	SetRootComponent(CollisionBox);

	Sprite = CreateDefaultSubobject<UPaperFlipbookComponent>(TEXT("Sprite"));
	Sprite->SetupAttachment(CollisionBox);
	Sprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Sprite->SetRelativeLocation(FVector(0.f, -4.f, 0.f));
	// 원작 스케일 0.35
	Sprite->SetRelativeScale3D(FVector(0.35f, 1.f, 0.35f));
	ApplyRatSpriteMaterial();

	YSort = CreateDefaultSubobject<ULSRatYSortComponent>(TEXT("YSort"));
	YSort->SortOffset = 30;

	IndicatorClass = ALSRatAttackIndicator::StaticClass();
}

void ALSRatFarmer::BeginPlay()
{
	Super::BeginPlay();

	if (YSort)
	{
		YSort->SortOffset = 30;
	}

	if (Sprite)
	{
		Sprite->SetRelativeLocation(FVector(0.f, -4.f, 0.f));
		ApplyRatSpriteMaterial();
	}

	const ALSRatFarmer* ClassDefault = GetClass()->GetDefaultObject<ALSRatFarmer>();
	if (ClassDefault && ClassDefault != this)
	{
		if (!IdleFlipbook) { IdleFlipbook = ClassDefault->IdleFlipbook; }
		if (!AngryIdleFlipbook) { AngryIdleFlipbook = ClassDefault->AngryIdleFlipbook; }
		if (!WalkFlipbook) { WalkFlipbook = ClassDefault->WalkFlipbook; }
		if (!AngryWalkFlipbook) { AngryWalkFlipbook = ClassDefault->AngryWalkFlipbook; }
		if (!AttackFlipbook) { AttackFlipbook = ClassDefault->AttackFlipbook; }
		if (!IndicatorClass) { IndicatorClass = ClassDefault->IndicatorClass; }
	}

	InitialPosition = GetXZ(GetActorLocation());
	SetFlipbookSafe(IdleFlipbook);

	if (!IdleFlipbook || !AngryIdleFlipbook || !WalkFlipbook || !AngryWalkFlipbook || !AttackFlipbook)
	{
		UE_LOG(LogLS, Warning,
			TEXT("[RatSteal] Farmer %s 플립북 미할당: Idle=%s AngryIdle=%s Walk=%s AngryWalk=%s Attack=%s"),
			*GetName(),
			*GetNameSafe(IdleFlipbook.Get()),
			*GetNameSafe(AngryIdleFlipbook.Get()),
			*GetNameSafe(WalkFlipbook.Get()),
			*GetNameSafe(AngryWalkFlipbook.Get()),
			*GetNameSafe(AttackFlipbook.Get()));
	}
}

void ALSRatFarmer::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const ALSRatGameMode* GameMode = GetWorld()->GetAuthGameMode<ALSRatGameMode>();
	if (GameMode && !GameMode->IsPlaying())
	{
		return;
	}

	switch (State)
	{
	case ELSRatFarmerState::Patrol: DoPatrol(DeltaSeconds); break;
	case ELSRatFarmerState::Chase:  DoChase(DeltaSeconds);  break;
	case ELSRatFarmerState::Attack: DoAttack(DeltaSeconds); break;
	}
}

ALSRatPlayer* ALSRatFarmer::GetRatPlayer() const
{
	return Cast<ALSRatPlayer>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void ALSRatFarmer::DoPatrol(float DeltaSeconds)
{
	// 전이: 플레이어가 Chase존(초기 위치 기준 650) 안 + 비-Hide → Chase
	const ALSRatPlayer* Player = GetRatPlayer();
	if (Player && Player->IsAlive() && !Player->IsHidden())
	{
		const float DistFromInitial = FVector2D::Distance(GetXZ(Player->GetActorLocation()), InitialPosition);
		if (DistFromInitial <= ChaseRadius)
		{
			ChangeState(ELSRatFarmerState::Chase);
			return;
		}
	}

	const FVector2D Cur = GetXZ(GetActorLocation());

	// 원작 DoPatrol — 목표 없거나 도달 시 새 목표 (중심 가중 pow(rand, biasExp))
	if (!bHasPatrolTarget || FVector2D::DistSquared(PatrolTarget, Cur) < 4.f)
	{
		bHasPatrolTarget = true;

		const float Rad = FMath::FRandRange(0.f, 2.f * PI);
		const float Biased = FMath::Pow(FMath::FRand(), PatrolBiasExp);
		const float R = PatrolRadius * Biased;

		PatrolTarget = InitialPosition + FVector2D(FMath::Cos(Rad), FMath::Sin(Rad)) * R;
	}

	const FVector2D Dir = PatrolTarget - Cur;
	if (Dir.SizeSquared() > 0.1f)
	{
		MoveTowards(PatrolTarget, DeltaSeconds);
		FaceDirection(Dir.X);
		SetFlipbookSafe(WalkFlipbook);
	}
	else
	{
		SetFlipbookSafe(IdleFlipbook);
	}
}

void ALSRatFarmer::DoChase(float DeltaSeconds)
{
	ALSRatPlayer* Player = GetRatPlayer();
	if (!Player || !Player->IsAlive() || Player->IsHidden())
	{
		ChangeState(ELSRatFarmerState::Patrol);
		return;
	}

	const FVector2D PlayerXZ = GetXZ(Player->GetActorLocation());
	const FVector2D Cur = GetXZ(GetActorLocation());

	// leash: 플레이어가 Chase존(초기 위치 650) 이탈 → 순찰 복귀
	if (FVector2D::Distance(PlayerXZ, InitialPosition) > ChaseRadius)
	{
		ChangeState(ELSRatFarmerState::Patrol);
		return;
	}

	// Attack존(농부 부착 200) 진입 → 공격
	if (FVector2D::Distance(PlayerXZ, Cur) <= AttackRadius)
	{
		ChangeState(ELSRatFarmerState::Attack);
		return;
	}

	SetFlipbookSafe(AngryWalkFlipbook);
	MoveTowards(PlayerXZ, DeltaSeconds);
	FaceDirection((PlayerXZ - Cur).X);
}

void ALSRatFarmer::DoAttack(float DeltaSeconds)
{
	ALSRatPlayer* Player = GetRatPlayer();

	// 플레이어 Hide → 지시자 제거 후 순찰 복귀
	if (!Player || !Player->IsAlive() || Player->IsHidden())
	{
		ClearIndicators();
		ChangeState(ELSRatFarmerState::Patrol);
		return;
	}

	const FVector2D PlayerXZ = GetXZ(Player->GetActorLocation());
	const FVector2D Cur = GetXZ(GetActorLocation());
	const bool bPlayerInAttackZone = FVector2D::Distance(PlayerXZ, Cur) <= AttackRadius;

	// 공격 애니메이션 재생 중이면 대기, 끝나면 존 이탈 여부로 Chase/Attack 결정
	if (AttackAnimRemaining > 0.f)
	{
		AttackAnimRemaining -= DeltaSeconds;
		if (AttackAnimRemaining <= 0.f)
		{
			ChangeState(bPlayerInAttackZone ? ELSRatFarmerState::Attack : ELSRatFarmerState::Chase);
		}
		return;
	}

	SetFlipbookSafe(AngryIdleFlipbook);
	FaceDirection((PlayerXZ - Cur).X);

	if (Indicators.Num() == 0)
	{
		// attackInterval(0.5s)마다 플레이어 위치에 지시자 생성
		AttackIntervalTimer += DeltaSeconds;
		if (AttackIntervalTimer >= AttackInterval)
		{
			AttackIntervalTimer = 0.f;
			AttackTimer = 0.f;
			CreateIndicators();
		}
	}
	else
	{
		// attackDelay(1.5s) 후 타격
		AttackTimer += DeltaSeconds;
		if (AttackTimer >= AttackDelay)
		{
			ExecuteAttack();
			PlayAttackAnimation();
		}
	}
}

void ALSRatFarmer::CreateIndicators()
{
	if (!IndicatorClass)
	{
		return;
	}

	const ALSRatPlayer* Player = GetRatPlayer();
	if (!Player)
	{
		return;
	}

	// 원작 오프셋: {0,0}, {0,+attackRadius}, {0,-attackRadius} (세로 십자형)
	const TArray<FVector2D> Offsets = PickAttackOffsets();
	const FVector PlayerLocation = Player->GetActorLocation();

	for (const FVector2D& Offset : Offsets)
	{
		const FVector SpawnLocation(PlayerLocation.X + Offset.X, PlayerLocation.Y, PlayerLocation.Z + Offset.Y);
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		if (ALSRatAttackIndicator* Indicator = GetWorld()->SpawnActor<ALSRatAttackIndicator>(IndicatorClass, SpawnLocation, FRotator::ZeroRotator, Params))
		{
			Indicators.Add(Indicator);
		}
	}
}

TArray<FVector2D> ALSRatFarmer::PickAttackOffsets() const
{
	const float R = AttackRadius;
	const int32 PatternIndex = FMath::RandRange(0, 4);

	switch (PatternIndex)
	{
	case 0:
		return { FVector2D(0.f, 0.f), FVector2D(0.f, R), FVector2D(0.f, -R) };
	case 1:
		return { FVector2D(0.f, 0.f), FVector2D(R, 0.f), FVector2D(-R, 0.f) };
	case 2:
		return { FVector2D(0.f, 0.f), FVector2D(R, 0.f), FVector2D(-R, 0.f), FVector2D(0.f, R), FVector2D(0.f, -R) };
	case 3:
		return { FVector2D(0.f, 0.f), FVector2D(R, R), FVector2D(-R, R), FVector2D(R, -R), FVector2D(-R, -R) };
	default:
		return { FVector2D(0.f, 0.f), FVector2D(R, R), FVector2D(-R, -R) };
	}
}

void ALSRatFarmer::ExecuteAttack()
{
	ALSRatPlayer* Player = GetRatPlayer();
	if (Player)
	{
		// 지시자 범위(attackRadius) 안이면 HP-1 (피격 1회)
		const FVector2D PlayerXZ = GetXZ(Player->GetActorLocation());
		for (const ALSRatAttackIndicator* Indicator : Indicators)
		{
			if (Indicator && FVector2D::Distance(GetXZ(Indicator->GetActorLocation()), PlayerXZ) <= AttackRadius)
			{
				Player->ApplyHit();
				break;
			}
		}
	}

	ClearIndicators();
}

void ALSRatFarmer::PlayAttackAnimation()
{
	if (!Sprite || !AttackFlipbook)
	{
		AttackAnimRemaining = AttackAnimDuration;
		return;
	}

	Sprite->SetLooping(false);
	Sprite->SetFlipbook(AttackFlipbook);
	Sprite->PlayFromStart();
	PlaySfx(AttackSound);
	AttackAnimRemaining = GetAttackAnimationDuration();
}

float ALSRatFarmer::GetAttackAnimationDuration() const
{
	if (AttackFlipbook)
	{
		return FMath::Max(AttackFlipbook->GetTotalDuration(), AttackAnimDuration);
	}
	return AttackAnimDuration;
}

void ALSRatFarmer::PlaySfx(USoundBase* Sound) const
{
	if (Sound)
	{
		UGameplayStatics::PlaySound2D(this, Sound);
	}
}

void ALSRatFarmer::ApplyRatSpriteMaterial()
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
		UE_LOG(LogLS, Warning, TEXT("[RatSteal] RatSpriteMaterial missing. Farmer sprite may be affected by scene lighting."));
	}
}

void ALSRatFarmer::ClearIndicators()
{
	for (ALSRatAttackIndicator* Indicator : Indicators)
	{
		if (Indicator)
		{
			Indicator->Destroy();
		}
	}
	Indicators.Reset();
}

void ALSRatFarmer::ChangeState(ELSRatFarmerState NewState)
{
	if (State == ELSRatFarmerState::Attack && NewState != ELSRatFarmerState::Attack)
	{
		ClearIndicators();
	}
	if (NewState == ELSRatFarmerState::Attack)
	{
		ClearIndicators();
		AttackTimer = 0.f;
		AttackIntervalTimer = 0.f;
	}
	if (NewState == ELSRatFarmerState::Patrol)
	{
		bHasPatrolTarget = false;
	}

	State = NewState;
}

void ALSRatFarmer::MoveTowards(const FVector2D& TargetXZ, float DeltaSeconds)
{
	const FVector2D Cur = GetXZ(GetActorLocation());
	const FVector2D Dir = (TargetXZ - Cur).GetSafeNormal();
	const FVector2D Step = Dir * MoveSpeed * DeltaSeconds;

	AddActorWorldOffset(FVector(Step.X, 0.f, Step.Y));
}

void ALSRatFarmer::FaceDirection(float DirX)
{
	if (!Sprite || FMath::IsNearlyZero(DirX))
	{
		return;
	}

	FVector NewScale = Sprite->GetRelativeScale3D();
	NewScale.X = FMath::Abs(NewScale.X) * (DirX < 0.f ? -1.f : 1.f);
	Sprite->SetRelativeScale3D(NewScale);
}

void ALSRatFarmer::SetFlipbookSafe(UPaperFlipbook* Flipbook)
{
	if (!Sprite || !Flipbook)
	{
		return;
	}

	const bool bChanged = Sprite->GetFlipbook() != Flipbook;
	Sprite->SetLooping(true);
	if (bChanged)
	{
		Sprite->SetFlipbook(Flipbook);
		Sprite->PlayFromStart();
	}
	else if (!Sprite->IsPlaying())
	{
		Sprite->Play();
	}
}
