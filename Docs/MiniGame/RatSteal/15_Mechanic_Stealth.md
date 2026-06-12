# RatSteal Mechanic — Stealth (은신)

> 범위: **1차** (Farmer 추적 해제의 핵심). Farmer 감지는 [11_Entity_Farmer.md](11_Entity_Farmer.md)가 소유.

## 목적

플레이어가 Farmer의 추적을 끊는 은신(부쉬) 규칙을 정의한다.

## 규칙 (원작)

```text
부쉬(Bush) 진입  Visibility = Hide
부쉬 이탈        Visibility = Visible
효과             Hide 상태면 Farmer가 감지하지 않음.
                 Chase/Attack 중에도 Hide 감지 시 Patrol로 복귀.
```

## 설계 의도

```text
- 추격당할 때 부쉬로 숨어 리셋하는 도주 수단
- 부쉬 배치가 곧 도주 동선 설계(30_Level_Layout)
```

## 미정 (확정 필요)

```text
- 부쉬 안에서 이동/훔치기 가능 여부
- 은신 중 시각 피드백(반투명 등)
- 부쉬 개수/배치 → 30_Level_Layout
```
