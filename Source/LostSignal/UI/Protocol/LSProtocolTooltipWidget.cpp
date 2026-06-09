#include "UI/Protocol/LSProtocolTooltipWidget.h"

#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Data/LSGameDataSubsystem.h"
#include "Data/LSProtocolUnlockRow.h"
#include "Engine/GameInstance.h"
#include "Engine/Texture2D.h"
#include "LostSignal.h"
#include "UI/Protocol/LSProtocolTooltipTextWidget.h"

#define LOCTEXT_NAMESPACE "LSProtocolTooltipWidget"

namespace
{
FText GetProtocolName(const ELSProtocolType ProtocolType)
{
	switch (ProtocolType)
	{
	case ELSProtocolType::Survival:
		return LOCTEXT("SurvivalProtocolName", "생존 프로토콜");
	case ELSProtocolType::Carrying:
		return LOCTEXT("CarryingProtocolName", "적재 프로토콜");
	case ELSProtocolType::Battle:
		return LOCTEXT("BattleProtocolName", "전투 프로토콜");
	case ELSProtocolType::Navigation:
		return LOCTEXT("NavigationProtocolName", "탐색 프로토콜");
	default:
		return LOCTEXT("UnknownProtocolName", "프로토콜");
	}
}

FText GetProtocolDescription(const ELSProtocolType ProtocolType)
{
	switch (ProtocolType)
	{
	case ELSProtocolType::Survival:
		return LOCTEXT("SurvivalProtocolDescription", "체력, 스태미나, 회복 정보를 해금합니다.");
	case ELSProtocolType::Carrying:
		return LOCTEXT("CarryingProtocolDescription", "인벤토리, 적재, 보호 슬롯 정보를 해금합니다.");
	case ELSProtocolType::Battle:
		return LOCTEXT("BattleProtocolDescription", "전투 판정과 전투 보조 UI 정보를 해금합니다.");
	case ELSProtocolType::Navigation:
		return LOCTEXT("NavigationProtocolDescription", "미니맵과 목표 추적 정보를 해금합니다.");
	default:
		return FText::GetEmpty();
	}
}

FText GetEnableDisplayText(const FName EnableName)
{
	static const TMap<FName, FText> Labels = {
		{ TEXT("Health_Bar"), LOCTEXT("HealthBar", "최대 체력 UI 표시") },
		{ TEXT("Stamina_Bar"), LOCTEXT("StaminaBar", "스태미나 UI 표시") },
		{ TEXT("Recovery_Bar"), LOCTEXT("RecoveryBar", "회복 UI 표시") },
		{ TEXT("Status_Duration"), LOCTEXT("StatusDuration", "상태이상 지속 시간 UI 표시") },
		{ TEXT("Danger_Warning"), LOCTEXT("DangerWarning", "위험 상태 경고 UI 표시") },
		{ TEXT("Shield_Bar"), LOCTEXT("ShieldBar", "보호막 UI 표시") },
		{ TEXT("Carry_Weight"), LOCTEXT("CarryWeight", "인벤토리 수치 표시") },
		{ TEXT("Carry_Limit"), LOCTEXT("CarryLimit", "적재 한계 UI 표시") },
		{ TEXT("Item_Value"), LOCTEXT("ItemValue", "아이템 가치 정보 UI 표시") },
		{ TEXT("Protection_Priority"), LOCTEXT("ProtectionPriority", "보관 우선순위 UI 표시") },
		{ TEXT("Equipment_Weight"), LOCTEXT("EquipmentWeight", "장비 무게 UI 표시") },
		{ TEXT("Damage_Number"), LOCTEXT("DamageNumber", "데미지 수치 UI 표시") },
		{ TEXT("Projectile_Trajectory"), LOCTEXT("ProjectileTrajectory", "투사체 궤적 UI 표시") },
		{ TEXT("Skill_Range"), LOCTEXT("SkillRange", "스킬 범위 UI 표시") },
		{ TEXT("Skill_Cooldown"), LOCTEXT("SkillCooldown", "스킬 쿨타임 UI 표시") },
		{ TEXT("Buff_Duration"), LOCTEXT("BuffDuration", "버프 지속 시간 UI 표시") },
		{ TEXT("Enemy_Attack_Range"), LOCTEXT("EnemyAttackRange", "적 공격 범위 UI 표시") },
		{ TEXT("Enemy_Health_Bar"), LOCTEXT("EnemyHealthBar", "적 체력바 UI 표시") },
		{ TEXT("Minimap"), LOCTEXT("Minimap", "미니맵 기본 UI 표시") },
		{ TEXT("Player_Point"), LOCTEXT("PlayerPoint", "플레이어 위치 UI 표시") },
		{ TEXT("Minimap_Looting_Object"), LOCTEXT("MinimapLootingObject", "루팅 오브젝트 UI 표시") },
		{ TEXT("Exit_Point"), LOCTEXT("ExitPoint", "탈출 지점 UI 표시") },
		{ TEXT("Minimap_View_Angle"), LOCTEXT("MinimapViewAngle", "시야각 UI 표시") },
		{ TEXT("Minimap_Enemy"), LOCTEXT("MinimapEnemy", "적 위치 UI 표시") },
		{ TEXT("Protected_Level"), LOCTEXT("ProtectedLevel", "해금 정보 보호") },
	};

	const FName NormalizedEnableName = LSProtocol::NormalizeProtocolEnableName(EnableName);
	if (const FText* Label = Labels.Find(NormalizedEnableName))
	{
		return *Label;
	}

	return FText::FromName(NormalizedEnableName);
}

FText BuildSynergyText(const FLSProtocolUnlockRow& Row, const bool bProtected)
{
	FText DisplayText = GetEnableDisplayText(Row.Protocol_Enable_Name);
	if (Row.Protocol_Enable_Type == TEXT("Protection") && Row.Protocol_Protected_Level > 0)
	{
		DisplayText = FText::Format(
			LOCTEXT("ProtectedLevelFormat", "Lv.{0}까지 해금 정보 보호"),
			FText::AsNumber(Row.Protocol_Protected_Level));
	}
	else if (Row.Protocol_Enable_Value != 0)
	{
		DisplayText = FText::Format(
			LOCTEXT("EnableValueFormat", "{0} ({1})"),
			DisplayText,
			FText::AsNumber(Row.Protocol_Enable_Value));
	}

	if (bProtected)
	{
		DisplayText = FText::Format(LOCTEXT("ProtectedFormat", "{0} - 보호 유지"), DisplayText);
	}

	return FText::Format(
		LOCTEXT("SynergyTextFormat", "({0})\n- {1}"),
		FText::AsNumber(Row.Protocol_Required_Level),
		DisplayText);
}
}

void ULSProtocolTooltipWidget::SetProtocolTooltip(const ELSProtocolType ProtocolType, UTexture2D* IconTexture)
{
	SetProtocolTooltipLevels(ProtocolType, IconTexture, 0, 0);
}

void ULSProtocolTooltipWidget::SetProtocolTooltipLevels(const ELSProtocolType ProtocolType, UTexture2D* IconTexture, const int32 CurrentLevel, const int32 PreviousLevel)
{
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
		ProtocolTypeText->SetText(GetProtocolName(ProtocolType));
	}

	if (!DescriptionText)
	{
		UE_LOG(LogLS, Warning, TEXT("DescriptionText is not bound on %s."), *GetNameSafe(this));
	}
	else
	{
		DescriptionText->SetText(GetProtocolDescription(ProtocolType));
	}

	if (!SynergyBox)
	{
		UE_LOG(LogLS, Warning, TEXT("SynergyBox is not bound on %s."), *GetNameSafe(this));
		return;
	}

	SynergyBox->ClearChildren();

	UGameInstance* GameInstance = GetGameInstance();
	const ULSGameDataSubsystem* GameDataSubsystem = GameInstance ? GameInstance->GetSubsystem<ULSGameDataSubsystem>() : nullptr;
	if (!GameDataSubsystem)
	{
		UE_LOG(LogLS, Warning, TEXT("Cannot build protocol tooltip because GameDataSubsystem is missing on %s."), *GetNameSafe(this));
		return;
	}

	TArray<const FLSProtocolUnlockRow*> Rows;
	GameDataSubsystem->GetProtocolUnlockRows(ProtocolType, Rows, TEXT("ProtocolTooltip"));
	for (const FLSProtocolUnlockRow* Row : Rows)
	{
		if (!Row)
		{
			continue;
		}

		bool bProtected = false;
		const bool bUnlocked = GameDataSubsystem->IsProtocolUnlockVisible(*Row, CurrentLevel, PreviousLevel, &bProtected);
		AddSynergyText(BuildSynergyText(*Row, bProtected), bUnlocked, bProtected);
	}
}

void ULSProtocolTooltipWidget::AddSynergyText(const FText& SynergyText, const bool bUnlocked, const bool bProtected)
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

	TooltipTextWidget->SetProtocolTooltipStateText(SynergyText, bUnlocked, bProtected);
	SynergyBox->AddChild(TooltipTextWidget);
}

#undef LOCTEXT_NAMESPACE
