# 로비 화면 구조

## 목적

이 문서는 로비 상단 메뉴와 배타 패널 전환, 뒤로가기 입력, 패널 진입 시 갱신, 포커스 가드의 구조 계약을 소유한다.
패널 내부의 아이템 조작은 [InventoryLogic.md](InventoryLogic.md), 최상위 뷰포트 레이어는
[UILayerStructure.md](UILayerStructure.md)가 소유한다. 클래스와 필드의 단일 출처는
`Source/LostSignal/UI/Lobby/LSLobbyMenuWidget.h`다.

## 화면 구성

`WBP_LobbyMenu` 하나가 로비 화면의 루트다. 상단 아이콘 바는 항상 보이고, 가운데 `TabSwitcher`와
`QuestPanelHost` 중 하나만 활성화된다. 보유 골드는 패널과 무관하게 계속 표시하고, 임무 시작 버튼은
로비 기본 상태에서만 표시한다.

| 상단 항목 | 바인딩 이름 | 여는 패널 |
|---|---|---|
| 로비 | `LobbyTab` | 없음 |
| 칩 세팅 | `ChipTab` | 칩 스테이션 |
| 정비 | `SupplyTab` | 에이베리 보급소 |
| 캐릭터 | `CharacterTab` | 스킬 로드아웃 |
| 가방 | `BagTab` | 인벤토리 + 물품창고 |
| 퀘스트 | `QuestTab` | 퀘스트 |
| 지도 | `MapTab` | 없음, 미구현 안내 |
| 설정 | `SettingsTab` | 배타 패널이 아닌 설정 오버레이 |

패널 종류는 `ELSLobbyPanel`로 표현하지만 enum 값과 스위처 인덱스는 대응하지 않는다.
`ULSLobbyMenuWidget::ShowPanel`은 `SetActiveWidget`에 바인딩된 페이지 포인터를 넘긴다. 페이지 순서를
바꾸더라도 동작이 달라져서는 안 된다. 로비 상태에서는 스위처 자체를 `Collapsed`로 둔다.

`WBP_LobbyMenu`의 `TabSwitcher`는 디자인 타임에 비워 둔다. 로비 루트는 클래스 디폴트의
`ChipStationPanelClass`, `StorePanelClass`, `SkillLoadoutPanelClass`를 생성해 스위처 직속 자식으로
추가한다. 가방은 `LobbyInventoryClass`와 `LobbyStorageClass`를 각각 생성한 뒤 C++ 런타임 오버레이
하나에 함께 넣어 단일 스위처 페이지로 등록한다. 퀘스트는 `QuestPanelClass`를 생성해 WBP에 바인딩된
빈 `QuestPanelHost` Border의 자식으로 넣는다. 따라서 각 패널의 큰 위젯 트리가 로비 디자이너에
펼쳐지지 않으면서 퀘스트 위치는 WBP가 정한다.

보유 골드는 루트의 `GoldText`에 표시한다. 초기값은 `ULSSaveSubsystem::GetGold()`에서 읽고,
`OnGoldChanged`를 구독해 구매·판매·제작 등으로 값이 바뀌면 즉시 갱신한다.

## 패널을 여는 계약

패널 전환의 유일한 입구는 `ShowPanel`이다.

1. 떠나는 패널이 별도 레이어에 띄운 다이얼로그를 닫는다.
2. 대상 페이지를 포인터로 활성화하고 현재 패널 상태를 갱신한다.
3. 선택 탭과 배경을 갱신한다.
4. `RefreshPanelOnOpen`에서 대상 패널의 최신 데이터를 다시 읽는다.

같은 패널 재클릭은 현재 스크롤·필터 상태를 보존하기 위해 무시한다. 다른 패널을 거쳐 돌아오면 반드시
`RefreshPanelOnOpen`을 다시 탄다. 특히 칩 패널의 풀 리빌드는 저장 슬롯 인덱스가 낡는 문제를 막는
방어선이므로 제거하면 안 된다.

`ValidatePanelBindings`는 로비 생성 시 다음 계약을 검사한다.

- 상단 탭, 골드 텍스트 등 루트 필수 위젯이 모두 바인딩됐는가
- 패널 WBP 클래스 4개와 가방 내부 WBP 클래스 2개가 클래스 디폴트에 지정됐고 정상 생성됐는가
- 런타임 패널 페이지 4개가 `TabSwitcher`의 직속 자식인가
- 생성한 퀘스트가 `QuestPanelHost`의 자식인가
- 디자인 타임에 넣어 둔 유령 페이지가 없는가
- 루트가 전달하는 공용 확인 다이얼로그 클래스가 지정됐는가

## 입력 계단

한 입력으로 두 화면 단계를 건너뛰지 않는다.

- `Esc`: 정비의 자판기/제작대 → 정비 기능 선택 → 로비(패널 없음) → 설정
- `Tab`: 정비의 자판기/제작대 → 정비 기능 선택, 그 외에는 가방 패널 토글
- `Insert`: 프로토콜 디버그 패널 토글

## 포커스와 모달

로비 루트는 `Tab`/`Esc` 입력을 유지하기 위해 포커스가 메뉴 밖으로 샜을 때 매 틱 회수한다. 다만 설정,
루트 안내창, 패널 자체 다이얼로그가 떠 있으면 회수하지 않는다. 현재 패널 모달 종합 대상은 칩 스테이션,
보급소, 로비 인벤토리 알림이다.

새 패널을 추가할 때는 다음을 함께 확인한다.

- 패널이 별도 뷰포트 레이어에 모달을 띄우는가
- 띄운다면 `HasActivePanelModal`의 포커스 예외에 합류했는가
- 패널을 떠날 때 `ClosePanelModal`이 그 모달을 닫는가
- `RefreshPanelOnOpen`에 필요한 최신 데이터 갱신이 있는가

자세한 원인과 진단 기록은
[UILobbyModalFocusReclaim.md](../Troubleshooting/UILobbyModalFocusReclaim.md)를 본다.

## WBP 아트 계약

- `WBP_LobbyMenu`의 `TabSwitcher`는 자식을 하나도 두지 않는다. C++이 런타임 패널 4개를 생성한다.
- `QuestPanelHost`는 원하는 위치에 빈 Border로 배치한다. C++이 `QuestPanelClass` 인스턴스를 자식으로 넣는다.
- 가방은 별도 WBP를 만들지 않는다. C++이 `LobbyInventoryClass`와 `LobbyStorageClass`를 생성해 같은
  런타임 오버레이에 겹쳐 넣으므로, 두 WBP 루트가 전체 화면 기준 배치를 소유한다.
- 패널 페이지 컨테이너와 장식 배경은 드롭을 받을 이유가 없으면 `SelfHitTestInvisible`로 둔다.
- 상단 탭 바, `GoldText`, 우하단 `MissionStartButton`은 스위처보다 높은 화면 계층에 둔다.
- `MissionStartButton`은 `LobbyTab`의 로비 기본 상태에서만 `Visible`이고 다른 패널·지도 안내·설정에서는 `Collapsed`다.
- `BackButton`, `InventoryButton`, `DestinationButton`은 사용하지 않는다. 뒤로가기와 가방 토글은 키 입력,
  목적지 선택은 `MapTab`이 소유한다.
- `ConfirmDialogClass`, `SettingsWidgetClass`, 패널 WBP 클래스 4개, 가방 내부 WBP 클래스 2개, 패널 배경 브러시는
  `WBP_LobbyMenu` 클래스 디폴트에서 매핑한다.
