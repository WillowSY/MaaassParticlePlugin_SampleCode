#pragma once

#include "CoreMinimal.h"
#include "MPTriggerVolumeTaskBase.h"
#include "StateTreeEvents.h"
#include "MPRequestEventTask.generated.h"

/**
 * TriggerVolume Enter / Exit 시 Entity의 Mass StateTree에 전달할 이벤트를 적재하는 Task입니다.
 *
 * 이 Task는 StateTree를 직접 실행하지 않습니다.
 * Event를 Request Fragment에 추가하고, 실제 전달은 VolumeTriggerProcessor의 Flush 단계에서 처리합니다.
 */
UCLASS()
class MAAASSPARTICLE_API UMPRequestEventTask : public UMPTriggerVolumeTaskBase
{
	GENERATED_BODY()

public:
	virtual void Execute(
		FMassEntityManager& EntityManager,
		FMassExecutionContext& Context,
		const FMassEntityHandle Entity) const override;

protected:
	/** StateTree Transition 조건과 매칭되는 이벤트입니다. */
	UPROPERTY(EditAnywhere, Category = "Event")
	FStateTreeEvent EventToSend;
};