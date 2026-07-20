#pragma once

#include "GameplayCueNotify_Static.h"
#include "LSGCN_SpawnNiagara.generated.h"

/**
 * GameplayCueParameters.SourceObject로 전달받은 Niagara 시스템을 지정 위치에 한 번 재생하는 범용 GameplayCue.
 * RawMagnitude가 0보다 크면 Niagara의 균일 스케일 배율로 사용한다.
 * 태그는 BP 파생에서 GameplayCueTag로 바인딩하고, 실제 Niagara 에셋은 호출부 데이터에서 전달한다.
 */
UCLASS()
class LOSTSIGNAL_API ULSGCN_SpawnNiagara : public UGameplayCueNotify_Static
{
	GENERATED_BODY()

public:
	virtual bool OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const override;
};
