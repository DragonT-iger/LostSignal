#include "UI/CharacterNode/LSSkillNodeLayout.h"

#include "UI/CharacterNode/LSSkillNodeGraphTypes.h"

namespace
{
// 파일 로컬 헬퍼에는 SkillNodeLayout 접두사를 붙인다(유니티 빌드 중복 정의 방지).

struct FSkillNodeLayoutEntry
{
	// 안쪽 링에 있는 선행. 이 노드가 어느 부모 밑에 매달릴지 결정한다.
	TArray<FName> InnerPrerequisites;

	// 같은 링에 있는 선행. 링 안에서의 깊이와 각도(원형 평균)를 결정한다.
	TArray<FName> SameRingPrerequisites;

	int32 DepthInRing = 0;
};

// 각도들의 원형 평균(도). 1차 링의 랩어라운드를 올바르게 처리하기 위한 것이다.
float SkillNodeLayoutCircularMeanDegrees(const TArray<float>& AnglesDegrees)
{
	FVector2D Sum = FVector2D::ZeroVector;
	for (const float Angle : AnglesDegrees)
	{
		const float Radians = FMath::DegreesToRadians(Angle);
		Sum += FVector2D(FMath::Cos(Radians), FMath::Sin(Radians));
	}

	if (Sum.IsNearlyZero())
	{
		return AnglesDegrees.Num() > 0 ? AnglesDegrees[0] : 0.0f;
	}

	return FMath::RadiansToDegrees(FMath::Atan2(Sum.Y, Sum.X));
}

// 12시 방향을 0도로 두고 시계 방향으로 돈다. 화면 Y 가 아래로 증가하므로 Y 를 뒤집는다.
FVector2D SkillNodeLayoutPolarToNormalized(const float AngleDegrees, const float Radius)
{
	const float Radians = FMath::DegreesToRadians(AngleDegrees);
	return FVector2D(FMath::Sin(Radians) * Radius, -FMath::Cos(Radians) * Radius);
}

void SkillNodeLayoutResolveDepths(TMap<FName, FSkillNodeLayoutEntry>& Entries)
{
	// 같은 링 선행이 없으면 깊이 1, 있으면 1 + 선행 깊이의 최대값.
	// 링 안의 노드 수가 작아서 반복 패스로 충분하다. 순환이 있어도 패스 수로 끊긴다.
	const int32 MaxPasses = Entries.Num() + 1;
	for (int32 Pass = 0; Pass < MaxPasses; ++Pass)
	{
		bool bChanged = false;
		for (TPair<FName, FSkillNodeLayoutEntry>& Pair : Entries)
		{
			FSkillNodeLayoutEntry& Entry = Pair.Value;
			if (Entry.DepthInRing > 0)
			{
				continue;
			}

			if (Entry.SameRingPrerequisites.IsEmpty())
			{
				Entry.DepthInRing = 1;
				bChanged = true;
				continue;
			}

			int32 MaxPrerequisiteDepth = 0;
			for (const FName Prerequisite : Entry.SameRingPrerequisites)
			{
				const FSkillNodeLayoutEntry* PrerequisiteEntry = Entries.Find(Prerequisite);
				if (!PrerequisiteEntry || PrerequisiteEntry->DepthInRing == 0)
				{
					MaxPrerequisiteDepth = 0;
					break;
				}
				MaxPrerequisiteDepth = FMath::Max(MaxPrerequisiteDepth, PrerequisiteEntry->DepthInRing);
			}

			if (MaxPrerequisiteDepth > 0)
			{
				Entry.DepthInRing = MaxPrerequisiteDepth + 1;
				bChanged = true;
			}
		}

		if (!bChanged)
		{
			break;
		}
	}

	// 순환 등으로 확정되지 못한 노드는 깊이 1로 둔다. 그래프 검증이 별도로 경고를 남긴다.
	for (TPair<FName, FSkillNodeLayoutEntry>& Pair : Entries)
	{
		if (Pair.Value.DepthInRing == 0)
		{
			Pair.Value.DepthInRing = 1;
		}
	}
}

// 링 안의 깊이 1 노드들에 각도를 준다. 부모(안쪽 링 선행)를 중심으로 섹터 폭 안에 균등 분할한다.
void SkillNodeLayoutAssignFirstDepthAngles(
	const TArray<FName>& NodeKeys,
	const TMap<FName, FSkillNodeLayoutEntry>& Entries,
	TMap<FName, float>& InOutAngles)
{
	TMap<FName, TArray<FName>> ChildrenByParent;
	for (const FName NodeKey : NodeKeys)
	{
		const FSkillNodeLayoutEntry& Entry = Entries[NodeKey];
		const FName ParentKey = Entry.InnerPrerequisites.IsEmpty() ? NAME_None : Entry.InnerPrerequisites[0];
		ChildrenByParent.FindOrAdd(ParentKey).Add(NodeKey);
	}

	// 부모 수가 섹터 수다. 부모들이 균등하게 놓여 있다는 전제이며, 부모 자신도 같은 규칙으로 놓였다.
	const float SectorWidth = 360.0f / static_cast<float>(FMath::Max(1, ChildrenByParent.Num()));

	for (TPair<FName, TArray<FName>>& Pair : ChildrenByParent)
	{
		TArray<FName>& Children = Pair.Value;
		Children.Sort([](const FName Left, const FName Right) { return Left.LexicalLess(Right); });

		const int32 ChildCount = Children.Num();
		const float* ParentAngle = InOutAngles.Find(Pair.Key);
		for (int32 Index = 0; Index < ChildCount; ++Index)
		{
			const float Fraction = (static_cast<float>(Index) + 0.5f) / static_cast<float>(ChildCount);
			// 부모에 각도가 없으면(코어이거나 선행이 없으면) 360도 전체에 균등 분할한다.
			const float Angle = ParentAngle
				? *ParentAngle + SectorWidth * (Fraction - 0.5f)
				: 360.0f * static_cast<float>(Index) / static_cast<float>(ChildCount);
			InOutAngles.Add(Children[Index], Angle);
		}
	}
}
} // namespace

namespace LSSkillNodeLayout
{
void ComputeAutoLayout(
	const TArray<FLSSkillNodeView>& Views,
	const FLSSkillNodeLayoutParams& Params,
	TMap<FName, FVector2D>& OutNormalizedPositions)
{
	OutNormalizedPositions.Reset();
	if (Views.IsEmpty())
	{
		return;
	}

	TMap<FName, int32> RingByKey;
	RingByKey.Reserve(Views.Num());
	for (const FLSSkillNodeView& View : Views)
	{
		RingByKey.Add(View.NodeKey, View.Ring);
	}

	// 링별로 엔트리를 나눈다. 깊이와 각도 계산이 링 단위로 돌아간다.
	TMap<int32, TMap<FName, FSkillNodeLayoutEntry>> EntriesByRing;
	for (const FLSSkillNodeView& View : Views)
	{
		if (View.IsCore())
		{
			OutNormalizedPositions.Add(View.NodeKey, FVector2D::ZeroVector);
			continue;
		}

		FSkillNodeLayoutEntry Entry;
		for (const FName Prerequisite : { View.Prerequisite_1, View.Prerequisite_2 })
		{
			const int32* PrerequisiteRing = Prerequisite.IsNone() ? nullptr : RingByKey.Find(Prerequisite);
			if (!PrerequisiteRing)
			{
				continue;
			}

			if (*PrerequisiteRing < View.Ring)
			{
				Entry.InnerPrerequisites.Add(Prerequisite);
			}
			else if (*PrerequisiteRing == View.Ring)
			{
				Entry.SameRingPrerequisites.Add(Prerequisite);
			}
		}

		EntriesByRing.FindOrAdd(View.Ring).Add(View.NodeKey, MoveTemp(Entry));
	}

	TArray<int32> Rings;
	EntriesByRing.GetKeys(Rings);
	Rings.Sort();

	TMap<FName, float> AnglesByKey;
	for (const int32 Ring : Rings)
	{
		TMap<FName, FSkillNodeLayoutEntry>& Entries = EntriesByRing[Ring];
		SkillNodeLayoutResolveDepths(Entries);

		int32 MaxDepth = 1;
		for (const TPair<FName, FSkillNodeLayoutEntry>& Pair : Entries)
		{
			MaxDepth = FMath::Max(MaxDepth, Pair.Value.DepthInRing);
		}

		for (int32 Depth = 1; Depth <= MaxDepth; ++Depth)
		{
			TArray<FName> NodeKeysAtDepth;
			for (const TPair<FName, FSkillNodeLayoutEntry>& Pair : Entries)
			{
				if (Pair.Value.DepthInRing == Depth)
				{
					NodeKeysAtDepth.Add(Pair.Key);
				}
			}

			if (NodeKeysAtDepth.IsEmpty())
			{
				continue;
			}

			if (Depth == 1)
			{
				SkillNodeLayoutAssignFirstDepthAngles(NodeKeysAtDepth, Entries, AnglesByKey);
				continue;
			}

			// 머지 노드는 선행들 각도의 원형 평균에 놓는다. 그러면 연결선이 교차하지 않는다.
			for (const FName NodeKey : NodeKeysAtDepth)
			{
				TArray<float> PrerequisiteAngles;
				for (const FName Prerequisite : Entries[NodeKey].SameRingPrerequisites)
				{
					if (const float* Angle = AnglesByKey.Find(Prerequisite))
					{
						PrerequisiteAngles.Add(*Angle);
					}
				}
				AnglesByKey.Add(NodeKey, SkillNodeLayoutCircularMeanDegrees(PrerequisiteAngles));
			}
		}

		for (const TPair<FName, FSkillNodeLayoutEntry>& Pair : Entries)
		{
			const float Radius = GetRingNormalizedRadius(Ring, Params)
				+ static_cast<float>(Pair.Value.DepthInRing - 1) * Params.InRingDepthStep;

			// 회전은 여기서 한 번만 더한다. 각도 계산(섹터 분할·원형 평균)은 회전을 모르므로
			// 노드 사이의 상대 위치가 틀어지지 않는다.
			OutNormalizedPositions.Add(
				Pair.Key,
				SkillNodeLayoutPolarToNormalized(AnglesByKey.FindRef(Pair.Key) + Params.RotationDegrees, Radius));
		}
	}
}

FVector2D ToLocalOffset(const FVector2D& Normalized, const FVector2D& LocalSize, const float FillRatio)
{
	const float Scale = FMath::Min(LocalSize.X, LocalSize.Y) * 0.5f * FillRatio;
	return Normalized * Scale;
}

float ToLocalRadius(const float NormalizedRadius, const FVector2D& LocalSize, const float FillRatio)
{
	return NormalizedRadius * FMath::Min(LocalSize.X, LocalSize.Y) * 0.5f * FillRatio;
}

float GetRingNormalizedRadius(const int32 Ring, const FLSSkillNodeLayoutParams& Params)
{
	if (Ring <= 0)
	{
		return 0.0f;
	}

	return Params.FirstRingRadius + static_cast<float>(Ring - 1) * Params.RingRadiusStep;
}
} // namespace LSSkillNodeLayout
