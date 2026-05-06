#include "Session/LSSessionSubsystem.h"
#include "Session/LSSessionSettings.h"
#include "Session/LSSaveSubsystem.h"
#include "LostSignal.h"
#include "Kismet/GameplayStatics.h"

void ULSSessionSubsystem::StartRaid(const TArray<FLSSessionItem>& Loadout)
{
	LoadoutSnapshot.Items = Loadout;
	SessionInventory.Empty();
	ConsumedItems.Empty();
	ResolvedItems.Empty();

	UE_LOG(LogLS, Log, TEXT("[Session] 레이드 시작 - 출발 장비 %d종"), Loadout.Num());
}

void ULSSessionSubsystem::EndRaid(ELSRaidResult Result)
{
	LastRaidResult = Result;
	ResolvedItems.Empty();

	switch (Result)
	{
	case ELSRaidResult::Extracted:
		ResolvedItems = SessionInventory;
		UE_LOG(LogLS, Log, TEXT("[Session] 탈출 성공 - 획득 아이템 %d종 보관"), ResolvedItems.Num());
		break;

	case ELSRaidResult::Quit:
		if (bAllowQuitRecovery)
		{
			ResolvedItems = BuildQuitRecovery();
			UE_LOG(LogLS, Log, TEXT("[Session] 탈주 - 장비 복구 %d종"), ResolvedItems.Num());
		}
		else
		{
			UE_LOG(LogLS, Log, TEXT("[Session] 탈주 - 장비 복구 비활성화, 전부 소실"));
		}
		break;

	case ELSRaidResult::Dead:
		UE_LOG(LogLS, Log, TEXT("[Session] 사망 - 전부 소실"));
		break;
	}

	// 스태시에 저장 (레벨 전환 전에 처리)
	if (!ResolvedItems.IsEmpty())
	{
		if (ULSSaveSubsystem* SaveSub = GetGameInstance()->GetSubsystem<ULSSaveSubsystem>())
		{
			SaveSub->AddToStash(ResolvedItems);
		}
	}

	// 결과 레벨로 전환
	const ULSSessionSettings* Settings = GetDefault<ULSSessionSettings>();
	if (!Settings->ResultLevel.IsNull())
	{
		UGameplayStatics::OpenLevelBySoftObjectPtr(this, Settings->ResultLevel);
	}
	else
	{
		UE_LOG(LogLS, Warning, TEXT("[Session] ResultLevel 미설정 - 프로젝트 설정 > LS Session Settings 확인"));
	}
}

void ULSSessionSubsystem::AddSessionItem(FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0) return;

	for (FLSSessionItem& Item : SessionInventory)
	{
		if (Item.ItemRowName == ItemRowName)
		{
			Item.Amount += Amount;
			return;
		}
	}

	SessionInventory.Add({ ItemRowName, Amount });
}

void ULSSessionSubsystem::ConsumeItem(FName ItemRowName, int32 Amount)
{
	if (ItemRowName.IsNone() || Amount <= 0) return;

	for (FLSSessionItem& Item : ConsumedItems)
	{
		if (Item.ItemRowName == ItemRowName)
		{
			Item.Amount += Amount;
			return;
		}
	}

	ConsumedItems.Add({ ItemRowName, Amount });
}

TArray<FLSSessionItem> ULSSessionSubsystem::BuildQuitRecovery() const
{
	// 출발 장비에서 소모된 수량을 차감해 반환
	TArray<FLSSessionItem> Recovery = LoadoutSnapshot.Items;

	for (const FLSSessionItem& Consumed : ConsumedItems)
	{
		for (FLSSessionItem& Item : Recovery)
		{
			if (Item.ItemRowName != Consumed.ItemRowName) continue;

			Item.Amount = FMath::Max(0, Item.Amount - Consumed.Amount);
			break;
		}
	}

	// 수량이 0이 된 항목 제거
	Recovery.RemoveAll([](const FLSSessionItem& Item) { return Item.Amount <= 0; });

	return Recovery;
}
