# RatSteal Entity — Farmer (농부)

> 범위: **1차 (Patrol→Chase→Attack)**. 감지 수치는 [50_Content_Balance.md](50_Content_Balance.md)가 소유.

## 목적

적 AI인 농부(Farmer)의 상태 머신, 감지 반경, 추적/공격 규칙을 정의한다.

## 상태 머신 (원작 FarmerState)

```text
Patrol   초기 위치 주변을 랜덤 순찰
Chase    플레이어 추적
Attack   공격 지시자(indicator) 생성 후 공격
(Alert)  원작 enum/존은 있으나 DoAlert가 빈 함수 = 미구현 → 채택 안 함
```

## 감지 반경 (원작 CircleCollider, 픽셀)

```text
Patrol  Radius 350   초기 위치 고정 — 순찰 반경
Alert   Radius 450   농부에 부착(따라다님) — 경계 진입
Chase   Radius 650   초기 위치 고정 — 추적 유지 범위(이탈 시 순찰 복귀, leash)
Attack  Radius 200   농부에 부착 — 공격 진입
```

- 존(Zone)은 별도 오브젝트(PatrolZone/AlertZone/ChaseZone/AttackZone)로 생성되어 Overlap으로 상태 전이를 유발.
- Patrol/Chase 존은 **초기 위치 고정**, Alert/Attack 존은 **농부에 부착**되어 따라다닌다.
- **플레이어가 Hide(부쉬) 상태면 감지하지 않는다.** Chase/Attack 중에도 Hide 감지 시 Patrol로 복귀.

## 상태별 동작 (원작 근거)

```text
DoPatrol
- 목표 없거나 도달하면 새 순찰 목표 생성:
  초기위치 중심, 각도 랜덤, 반경 = patrolRadius * pow(rand, biasExp=2)
  (biasExp로 중심 쪽에 가중 — 멀리 덜 감)
- 목표로 이동(속도 200), walk/idle 애니메이션 + 좌우 플립

DoChase
- 플레이어가 Hide면 → Patrol
- 플레이어 방향으로 직선 이동(속도 200), angrywalk 애니메이션
- Chase 존을 벗어나면(OnTriggerExit chaseZone) → Patrol (leash 복귀)

DoAttack
- 플레이어가 Hide면 → indicator 제거 후 Patrol
- attack 애니 재생 중이면 대기
- indicator 없으면: attackInterval(0.5s)마다 플레이어 위치에 지시자 생성
- indicator 있으면: attackDelay(1.5s) 후 Execute → 범위 내 플레이어 HP-1, attack 애니
- 공격 후: 공격존 이탈 상태면 Chase, 아니면 Attack 유지

DoAlert
- 원작 DoAlert()가 빈 함수 = Alert 상태는 선언만 있고 동작이 없음(미구현).
- 채택하지 않는다. 상태 머신은 Patrol/Chase/Attack 3종으로 간다.
- (AlertZone 반경 450도 함께 제거 또는 미사용 처리)
```

## 공격 패턴 (AttackPattern)

```text
지시자 오프셋: {0,0}, {0,+attackRadius}, {0,-attackRadius}  (세로 십자형)
attackDelay     1.5s  (지시자 표시 후 타격까지 — 회피 여유)
attackInterval  0.5s  (지시자 재생성 주기)
공격 피해       HP -1
이동속도        200
스케일          0.35
콜라이더        offset(4,0), size 140 x 400
```

## 전이 요약

```text
Patrol  --(플레이어가 Chase존 진입, 비-Hide)--> Chase
Chase   --(Attack존 진입)--> Attack
Chase   --(Chase존 이탈 / 플레이어 Hide)--> Patrol
Attack  --(공격 후 Attack존 이탈)--> Chase
Attack  --(플레이어 Hide)--> Patrol
```

## UE 매핑 (요약)

```text
Farmer       → ALSRatFarmer (APawn)
상태 머신    → C++ enum + switch (StateTree 미사용 — 경량 미니게임)
감지 존      → USphereComponent 4종 (Overlap Begin/End로 상태 전이)
이동         → 직접 Translate 또는 FloatingPawnMovement
지시자/공격  → ULSRatAttackPattern (타이머 + Overlap 판정)
애니메이션   → UPaperFlipbook (idle/angryidle/walk/angrywalk/attack)
```

본편 몬스터 AI([../../Systems/MonsterAIControlStructure.md](../../Systems/MonsterAIControlStructure.md))의 StateTree/GAS 구조는 **쓰지 않는다.** 미니게임은 단순 enum 상태 머신으로 충분.

## 미해결 / 확인 필요

```text
- 감지 반경 단위: 원작 픽셀 → UE 환산은 플레이로 조정(50_Balance "구현 단계 조정")
- 농부 다수 배치 여부(원작 FarmerManager 존재) → 1차는 1마리
```
