#include "UI/Protocol/LSProtocolTooltipWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "LostSignal.h"
#include "UI/Protocol/LSProtocolTooltipTextWidget.h"

#define LOCTEXT_NAMESPACE "LSProtocolTooltipWidget"

namespace
{
struct FLSProtocolTooltipData
{
	FText ProtocolName;
	FText Description;
	TArray<FText> SynergyTexts;
};

FLSProtocolTooltipData MakeSurvivalTooltipData()
{
	return {
		LOCTEXT("SurvivalProtocolName", "생존 프로토콜"),
		LOCTEXT("SurvivalProtocolDescription", "생존 및 회복 정보 신호를 담당한다."),
		{
			LOCTEXT("SurvivalSynergy1", "(1)\n- 최대 체력 UI 표시"),
			LOCTEXT("SurvivalSynergy2", "(2)\n- 회복량 UI 표시"),
			LOCTEXT("SurvivalSynergy3", "(3)\n- 상태이상 지속 시간 UI 표시"),
			LOCTEXT("SurvivalSynergy4", "(4)\n- 위험 상태 경고 UI 표시"),
			LOCTEXT("SurvivalSynergy5", "(5)\n- 보호막 UI 표시"),
			LOCTEXT("SurvivalSynergy6", "(6)\n- Lv.3까지 정보가 사라지지 않는다."),
			LOCTEXT("SurvivalSynergy7", "(7)\n- Lv.5까지 정보가 사라지지 않는다.")
		}
	};
}

FLSProtocolTooltipData MakeCarryingTooltipData()
{
	return {
		LOCTEXT("CarryingProtocolName", "적재 프로토콜"),
		LOCTEXT("CarryingProtocolDescription", "아이템 운반 및 적재 관련 신호를 담당한다."),
		{
			LOCTEXT("CarryingSynergy1", "(1)\n- 인벤토리 수치 표시"),
			LOCTEXT("CarryingSynergy2", "(2)\n- 적재 한계 UI 표시"),
			LOCTEXT("CarryingSynergy3", "(3)\n- 아이템 가치 정보 UI 표시"),
			LOCTEXT("CarryingSynergy4", "(4)\n- 보관 우선순위 UI 표시"),
			LOCTEXT("CarryingSynergy5", "(5)\n- 장비 무게 UI 표시"),
			LOCTEXT("CarryingSynergy6", "(6)\n- Lv.3까지 정보가 사라지지 않는다."),
			LOCTEXT("CarryingSynergy7", "(7)\n- Lv.5까지 정보가 사라지지 않는다.")
		}
	};
}

FLSProtocolTooltipData MakeBattleTooltipData()
{
	return {
		LOCTEXT("BattleProtocolName", "전투 프로토콜"),
		LOCTEXT("BattleProtocolDescription", "인벤토리 및 아이템 정보 신호를 담당한다."),
		{
			LOCTEXT("BattleSynergy1", "(1)\n- 데미지 수치 표시, 스킬 슬롯 UI 표시"),
			LOCTEXT("BattleSynergy2", "(2)\n- 스킬 범위 표시, 스킬 쿨타임 숫자 UI 표시"),
			LOCTEXT("BattleSynergy3", "(3)\n- 캐스팅, 버프 지속 시간,\n스킬 쿨타임 게이지바 UI 표시"),
			LOCTEXT("BattleSynergy4", "(4)\n- 적 공격 타이밍 및 공격 범위 UI 표시"),
			LOCTEXT("BattleSynergy5", "(5)\n- 적 체력바 UI 표시"),
			LOCTEXT("BattleSynergy6", "(6)\n- Lv.3까지 정보가 사라지지 않는다."),
			LOCTEXT("BattleSynergy7", "(7)\n- Lv.5까지 정보가 사라지지 않는다.")
		}
	};
}

FLSProtocolTooltipData MakeNavigationTooltipData()
{
	return {
		LOCTEXT("NavigationProtocolName", "탐색 프로토콜"),
		LOCTEXT("NavigationProtocolDescription", "맵 탐색 및 목표 추적 신호를 담당한다."),
		{
			LOCTEXT("NavigationSynergy1", "(1)\n- 미니맵 기본 UI 표시"),
			LOCTEXT("NavigationSynergy2", "(2)\n- 목표 방향 UI 표시"),
			LOCTEXT("NavigationSynergy3", "(3)\n- 상호작용 가능 오브젝트 UI 표시"),
			LOCTEXT("NavigationSynergy4", "(4)\n- 탈출 지점 UI 표시"),
			LOCTEXT("NavigationSynergy5", "(5)\n- 위험 구역 UI 표시"),
			LOCTEXT("NavigationSynergy6", "(6)\n- Lv.3까지 정보가 사라지지 않는다."),
			LOCTEXT("NavigationSynergy7", "(7)\n- Lv.5까지 정보가 사라지지 않는다.")
		}
	};
}

FLSProtocolTooltipData GetTemporaryProtocolTooltipData(const ELSProtocolType ProtocolType)
{
	switch (ProtocolType)
	{
	case ELSProtocolType::Survival:
		return MakeSurvivalTooltipData();
	case ELSProtocolType::Carrying:
		return MakeCarryingTooltipData();
	case ELSProtocolType::Battle:
		return MakeBattleTooltipData();
	case ELSProtocolType::Navigation:
		return MakeNavigationTooltipData();
	default:
		return MakeSurvivalTooltipData();
	}
}
}

void ULSProtocolTooltipWidget::SetProtocolTooltip(const ELSProtocolType ProtocolType, UTexture2D* IconTexture)
{
	const FLSProtocolTooltipData TooltipData = GetTemporaryProtocolTooltipData(ProtocolType);

	if (!ProtocolImage)
	{
		UE_LOG(LogLS, Warning, TEXT("ProtocolImage is not bound on %s."), *GetNameSafe(this));
	}
	else if (IconTexture)
	{
		ProtocolImage->SetBrushFromTexture(IconTexture);
	}

	if (!ProtocolTypeText)
	{
		UE_LOG(LogLS, Warning, TEXT("ProtocolTypeText is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		ProtocolTypeText->SetText(TooltipData.ProtocolName);
	}

	if (!DescriptionText)
	{
		UE_LOG(LogLS, Warning, TEXT("DescriptionText is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		DescriptionText->SetText(TooltipData.Description);
	}

	if (!SynergyBox)
	{
		UE_LOG(LogLS, Warning, TEXT("SynergyBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	SynergyBox->ClearChildren();
	for (const FText& SynergyText : TooltipData.SynergyTexts)
	{
		AddSynergyText(SynergyText);
	}
}

void ULSProtocolTooltipWidget::AddSynergyText(const FText& SynergyText)
{
	if (!SynergyBox)
	{
		UE_LOG(LogLS, Warning, TEXT("SynergyBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	if (!TooltipTextWidgetClass)
	{
		UE_LOG(LogLS, Warning, TEXT("TooltipTextWidgetClass is not set on %s."), *GetNameSafe(this));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot create protocol tooltip text because world is missing on %s."), *GetNameSafe(this));
		return;
	}

	ULSProtocolTooltipTextWidget* TooltipTextWidget = CreateWidget<ULSProtocolTooltipTextWidget>(World, TooltipTextWidgetClass);
	if (!TooltipTextWidget)
	{
		UE_LOG(LogLS, Warning, TEXT("Failed to create protocol tooltip text on %s."), *GetNameSafe(this));
		return;
	}

	TooltipTextWidget->SetProtocolTooltipText(SynergyText);
	SynergyBox->AddChild(TooltipTextWidget);
}

#undef LOCTEXT_NAMESPACE
