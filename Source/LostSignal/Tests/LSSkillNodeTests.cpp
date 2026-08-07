#include "Data/LSSkillNodeIndex.h"
#include "UI/CharacterNode/LSSkillNodeGraphTypes.h"
#include "UI/CharacterNode/LSSkillNodeLayout.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

namespace
{
constexpr int32 SkillNodeTestCharacterID = 101;

void AddSkillNodeTestEntry(
	FLSSkillNodeIndex& Index,
	const FName NodeKey,
	const ELSSkillNodeKind Kind,
	const int32 Ring,
	const FName Prerequisite1 = NAME_None,
	const FName Prerequisite2 = NAME_None)
{
	FLSSkillNodeRef Ref;
	Ref.NodeKey = NodeKey;
	Ref.Kind = Kind;
	Ref.CharacterID = SkillNodeTestCharacterID;
	Ref.Ring = Ring;
	Ref.Prerequisite_1 = Prerequisite1;
	Ref.Prerequisite_2 = Prerequisite2;
	Index.Nodes.Add(NodeKey, Ref);
	Index.NodeKeysByCharacter.FindOrAdd(SkillNodeTestCharacterID).Add(NodeKey);
}

TArray<const FLSSkillNodeRef*> CollectSkillNodeTestRefs(const FLSSkillNodeIndex& Index)
{
	TArray<const FLSSkillNodeRef*> Refs;
	const TArray<FName>* NodeKeys = Index.NodeKeysByCharacter.Find(SkillNodeTestCharacterID);
	if (!NodeKeys)
	{
		return Refs;
	}

	for (const FName NodeKey : *NodeKeys)
	{
		Refs.Add(Index.Find(NodeKey));
	}
	return Refs;
}

/**
 * 1차 링 8각 고리를 만든다 — 실측 데이터의 가장 까다로운 모양이다.
 * 메인 4개가 코어에 붙고, 서브 4개가 인접한 두 메인을 선행으로 갖는다.
 * 마지막 서브(S04)는 M04 와 M01 을 선행으로 갖는 랩어라운드다.
 */
FLSSkillNodeIndex MakeSkillNodeTestRing1Index()
{
	FLSSkillNodeIndex Index;
	AddSkillNodeTestEntry(Index, TEXT("CORE"), ELSSkillNodeKind::Core, 0);
	AddSkillNodeTestEntry(Index, TEXT("M01"), ELSSkillNodeKind::MainStat, 1, TEXT("CORE"));
	AddSkillNodeTestEntry(Index, TEXT("M02"), ELSSkillNodeKind::MainStat, 1, TEXT("CORE"));
	AddSkillNodeTestEntry(Index, TEXT("M03"), ELSSkillNodeKind::MainStat, 1, TEXT("CORE"));
	AddSkillNodeTestEntry(Index, TEXT("M04"), ELSSkillNodeKind::MainStat, 1, TEXT("CORE"));
	AddSkillNodeTestEntry(Index, TEXT("S01"), ELSSkillNodeKind::SubStat, 1, TEXT("M01"), TEXT("M02"));
	AddSkillNodeTestEntry(Index, TEXT("S02"), ELSSkillNodeKind::SubStat, 1, TEXT("M02"), TEXT("M03"));
	AddSkillNodeTestEntry(Index, TEXT("S03"), ELSSkillNodeKind::SubStat, 1, TEXT("M03"), TEXT("M04"));
	AddSkillNodeTestEntry(Index, TEXT("S04"), ELSSkillNodeKind::SubStat, 1, TEXT("M04"), TEXT("M01"));
	return Index;
}

const FLSSkillNodeView* FindSkillNodeTestView(const TArray<FLSSkillNodeView>& Views, const FName NodeKey)
{
	return Views.FindByPredicate([NodeKey](const FLSSkillNodeView& View) { return View.NodeKey == NodeKey; });
}
} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSSkillNodePrerequisiteAnyTest,
	"LostSignal.SkillNode.PrerequisiteAny",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSSkillNodePrerequisiteAnyTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FLSSkillNodeIndex Index = MakeSkillNodeTestRing1Index();
	const TArray<const FLSSkillNodeRef*> Refs = CollectSkillNodeTestRefs(Index);

	// 코어만 활성인 초기 상태.
	TSet<FName> Activated;
	LSSkillNodeViews::CollectAutoActivatedNodeKeys(Refs, Activated);
	TestEqual(TEXT("코어만 자동 활성이다"), Activated.Num(), 1);

	TArray<FLSSkillNodeView> Views;
	LSSkillNodeViews::BuildViews(Refs, Activated, Views);
	TestEqual(TEXT("뷰가 노드 수만큼 나온다"), Views.Num(), 9);

	const FLSSkillNodeView* Core = FindSkillNodeTestView(Views, TEXT("CORE"));
	const FLSSkillNodeView* Main = FindSkillNodeTestView(Views, TEXT("M01"));
	const FLSSkillNodeView* Sub = FindSkillNodeTestView(Views, TEXT("S01"));
	if (!Core || !Main || !Sub)
	{
		AddError(TEXT("테스트 노드를 찾을 수 없다"));
		return false;
	}

	TestEqual(TEXT("코어는 활성"), static_cast<uint8>(Core->State), static_cast<uint8>(ELSSkillNodeState::Activated));
	TestEqual(TEXT("코어 자식은 해금 가능"), static_cast<uint8>(Main->State), static_cast<uint8>(ELSSkillNodeState::Available));
	TestEqual(TEXT("선행이 둘 다 비활성인 노드는 잠김"), static_cast<uint8>(Sub->State), static_cast<uint8>(ELSSkillNodeState::Locked));

	// 선행 하나만 활성으로 만든다. ALL 이면 여전히 잠김이어야 하고, ANY 면 열려야 한다.
	Activated.Add(TEXT("M01"));
	LSSkillNodeViews::BuildViews(Refs, Activated, Views);
	Sub = FindSkillNodeTestView(Views, TEXT("S01"));
	if (!Sub)
	{
		AddError(TEXT("S01을 찾을 수 없다"));
		return false;
	}

	TestEqual(TEXT("선행 하나만 활성이어도 열린다(ANY)"), static_cast<uint8>(Sub->State), static_cast<uint8>(ELSSkillNodeState::Available));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSSkillNodeAutoLayoutTest,
	"LostSignal.SkillNode.AutoLayout",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSSkillNodeAutoLayoutTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	const FLSSkillNodeIndex Index = MakeSkillNodeTestRing1Index();
	const TArray<const FLSSkillNodeRef*> Refs = CollectSkillNodeTestRefs(Index);

	TSet<FName> Activated;
	TArray<FLSSkillNodeView> Views;
	LSSkillNodeViews::BuildViews(Refs, Activated, Views);

	// 배치 규칙 검증은 회전 0으로 고정한다. 전체 회전은 각도 계산과 분리된 후처리이므로 아래에서 따로 본다.
	// 이렇게 두면 아트 요청으로 기본 회전값이 바뀌어도 이 검증들이 깨지지 않는다.
	FLSSkillNodeLayoutParams Params;
	Params.RotationDegrees = 0.0f;

	TMap<FName, FVector2D> Positions;
	LSSkillNodeLayout::ComputeAutoLayout(Views, Params, Positions);

	TestEqual(TEXT("모든 노드에 좌표가 있다"), Positions.Num(), 9);
	TestTrue(TEXT("코어는 중심"), Positions.FindRef(TEXT("CORE")).IsNearlyZero());

	// 메인 4개는 12시부터 90도 간격이다.
	const float MainRadius = Params.FirstRingRadius;
	TestTrue(TEXT("M01은 12시 방향"), Positions.FindRef(TEXT("M01")).Equals(FVector2D(0.0f, -MainRadius), 0.001f));
	TestTrue(TEXT("M02는 3시 방향"), Positions.FindRef(TEXT("M02")).Equals(FVector2D(MainRadius, 0.0f), 0.001f));
	TestTrue(TEXT("M03은 6시 방향"), Positions.FindRef(TEXT("M03")).Equals(FVector2D(0.0f, MainRadius), 0.001f));
	TestTrue(TEXT("M04는 9시 방향"), Positions.FindRef(TEXT("M04")).Equals(FVector2D(-MainRadius, 0.0f), 0.001f));

	// 서브는 같은 링의 머지 노드라 깊이가 1단 더 깊고, 인접한 두 메인 사이 각도에 놓인다.
	const float SubRadius = Params.FirstRingRadius + Params.InRingDepthStep;
	for (const TCHAR* SubKey : { TEXT("S01"), TEXT("S02"), TEXT("S03"), TEXT("S04") })
	{
		const FVector2D Position = Positions.FindRef(FName(SubKey));
		TestTrue(FString::Printf(TEXT("%s는 메인보다 바깥 반지름"), SubKey),
			FMath::IsNearlyEqual(static_cast<float>(Position.Size()), SubRadius, 0.001f));
	}

	// 랩어라운드 검증. S04 는 9시(M04)와 12시(M01)를 선행으로 갖는다.
	// 원형 평균이면 10시 30분 방향(x<0, y<0)이고, 단순 산술 평균이면 6시 방향(y>0)으로 정반대가 된다.
	const FVector2D WrapAroundPosition = Positions.FindRef(TEXT("S04"));
	TestTrue(TEXT("랩어라운드 노드가 좌상단에 놓인다(원형 평균)"), WrapAroundPosition.X < 0.0f && WrapAroundPosition.Y < 0.0f);

	// 전체 회전. 강체 변환이라 M01 하나로 부호와 크기를 확인하면 충분하다.
	// -45도면 12시에 있던 M01이 좌상단으로 간다(x<0, y<0, 두 성분의 크기가 같다).
	FLSSkillNodeLayoutParams RotatedParams;
	RotatedParams.RotationDegrees = -45.0f;

	TMap<FName, FVector2D> RotatedPositions;
	LSSkillNodeLayout::ComputeAutoLayout(Views, RotatedParams, RotatedPositions);

	const float Diagonal = MainRadius * FMath::Sqrt(0.5f);
	TestTrue(TEXT("-45도 회전이면 M01이 좌상단으로 간다"),
		RotatedPositions.FindRef(TEXT("M01")).Equals(FVector2D(-Diagonal, -Diagonal), 0.001f));

	// 겹침 검사. 어떤 두 노드도 같은 자리에 놓이지 않는다.
	TArray<FName> Keys;
	Positions.GetKeys(Keys);
	for (int32 Left = 0; Left < Keys.Num(); ++Left)
	{
		for (int32 Right = Left + 1; Right < Keys.Num(); ++Right)
		{
			const float Distance = static_cast<float>(FVector2D::Distance(Positions[Keys[Left]], Positions[Keys[Right]]));
			if (Distance < 0.05f)
			{
				AddError(FString::Printf(TEXT("%s와 %s가 너무 가깝다 (%.4f)"),
					*Keys[Left].ToString(), *Keys[Right].ToString(), Distance));
			}
		}
	}

	return true;
}

#if WITH_EDITOR
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLSSkillNodeGraphValidationTest,
	"LostSignal.SkillNode.GraphValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLSSkillNodeGraphValidationTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	// 정상 그래프. 같은 링 연결이 8건이고 그중 하나가 랩어라운드다.
	// Ring 역행 검사를 >= 로 쓰거나 사이클을 무향으로 돌면 여기서 경고가 뜬다.
	const FLSSkillNodeIndex ValidIndex = MakeSkillNodeTestRing1Index();
	TestEqual(TEXT("정상 그래프는 문제가 0건이다"), LSSkillNodes::ValidateGraph(ValidIndex), 0);

	// 아래 실패 케이스들은 의도적으로 [SkillNode] 경고를 남긴다.
	// UE_LOG Warning 은 기본 설정에서 테스트를 실패시키지 않으므로 그대로 둔다(반환값으로 판정한다).

	// 유향 사이클. A -> B -> A.
	FLSSkillNodeIndex CyclicIndex;
	AddSkillNodeTestEntry(CyclicIndex, TEXT("CORE"), ELSSkillNodeKind::Core, 0);
	AddSkillNodeTestEntry(CyclicIndex, TEXT("A"), ELSSkillNodeKind::SubStat, 1, TEXT("B"));
	AddSkillNodeTestEntry(CyclicIndex, TEXT("B"), ELSSkillNodeKind::SubStat, 1, TEXT("A"));
	TestTrue(TEXT("유향 사이클을 잡는다"), LSSkillNodes::ValidateGraph(CyclicIndex) > 0);

	// 미존재 선행.
	FLSSkillNodeIndex DanglingIndex;
	AddSkillNodeTestEntry(DanglingIndex, TEXT("CORE"), ELSSkillNodeKind::Core, 0);
	AddSkillNodeTestEntry(DanglingIndex, TEXT("A"), ELSSkillNodeKind::SubStat, 1, TEXT("MISSING"));
	TestTrue(TEXT("없는 선행을 잡는다"), LSSkillNodes::ValidateGraph(DanglingIndex) > 0);

	// Ring 역행. 1차 링 노드가 2차 링 노드를 선행으로 갖는다.
	FLSSkillNodeIndex BacktrackIndex;
	AddSkillNodeTestEntry(BacktrackIndex, TEXT("CORE"), ELSSkillNodeKind::Core, 0);
	AddSkillNodeTestEntry(BacktrackIndex, TEXT("OUTER"), ELSSkillNodeKind::SubStat, 2, TEXT("CORE"));
	AddSkillNodeTestEntry(BacktrackIndex, TEXT("INNER"), ELSSkillNodeKind::SubStat, 1, TEXT("OUTER"));
	TestTrue(TEXT("링 역행을 잡는다"), LSSkillNodes::ValidateGraph(BacktrackIndex) > 0);

	return true;
}
#endif // WITH_EDITOR

#endif // WITH_DEV_AUTOMATION_TESTS
