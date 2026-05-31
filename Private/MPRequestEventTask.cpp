#include "MPRequestEventTask.h"
#include "MPTriggerVolumeRequestEventFragment.h"

void UMPRequestEventTask::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context, const FMassEntityHandle Entity) const
{
    Super::Execute(EntityManager, Context, Entity);

    // 유효하지 않은 GameplayTag는 이벤트로 전달하지 않습니다.
    if (!EventToSend.Tag.IsValid())
    {
        return;
    }

    FMPTriggerVolumeRequestEventFragment* RequestFragment = EntityManager.GetFragmentDataPtr<FMPTriggerVolumeRequestEventFragment>(Entity);
    if (!RequestFragment)
    {
        return;
    }

    // Task는 StateTree를 직접 실행하지 않고, Processor가 Flush할 이벤트만 적재합니다.
    RequestFragment->PendingEvents.Add(EventToSend);
}