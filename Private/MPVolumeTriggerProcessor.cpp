#include "MassEntity/Processors/TriggerVolume/MPVolumeTriggerProcessor.h"

#include "MassCommonFragments.h"
#include "MassSignalSubsystem.h"
#include "MassSignalTypes.h"
#include "MassStateTreeFragments.h"
#include "StateTreeExecutionContext.h"

#include "MPTriggerVolumeComponent.h"
#include "MPTriggerVolumeEventData.h"
#include "MPTriggerVolumeRequestEventFragment.h"
#include "MPTriggerVolumeSubsystem.h"
#include "MPTriggerVolumeTaskBase.h"

UMPVolumeTriggerProcessor::UMPVolumeTriggerProcessor()
{
    ExecutionOrder.ExecuteInGroup = UE::Mass::ProcessorGroupNames::SyncWorldToMass;
    bAutoRegisterWithProcessingPhases = true;
    
    // 대부분의 위치 쿼리에서 후보 Volume 수는 많지 않으므로, 기본 버퍼를 미리 확보합니다.
    CandidateBuffer.Reserve(16);
    
    EntityQuery.RegisterWithProcessor(*this);
}

void UMPVolumeTriggerProcessor::ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager)
{
    EntityQuery.Initialize(EntityManager);
    
    EntityQuery.AddRequirement<FTransformFragment>(EMassFragmentAccess::ReadOnly);
    EntityQuery.AddRequirement<FMPTriggerVolumeRequestEventFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddRequirement<FMassStateTreeInstanceFragment>(EMassFragmentAccess::ReadWrite);
    EntityQuery.AddConstSharedRequirement<FMassStateTreeSharedFragment>(EMassFragmentPresence::All);
    
    EntityQuery.RegisterWithProcessor(*this);
}

void UMPVolumeTriggerProcessor::Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context)
{
    UWorld* World = EntityManager.GetWorld();
    if (!World)
    {
        return;
    }

    if (!VolumeSubsystem)
    {
        VolumeSubsystem = World->GetSubsystem<UMPTriggerVolumeSubsystem>();
    }

    UMassStateTreeSubsystem* StateTreeSubsystem = World->GetSubsystem<UMassStateTreeSubsystem>();
    UMassSignalSubsystem* SignalSubsystem = World->GetSubsystem<UMassSignalSubsystem>();    

    if (!VolumeSubsystem || !StateTreeSubsystem || !SignalSubsystem)
    {
        return;
    }

    EntityQuery.ForEachEntityChunk(EntityManager, Context, [this, &EntityManager, StateTreeSubsystem, SignalSubsystem](FMassExecutionContext& ChunkContext)
    {
        const TConstArrayView<FTransformFragment> TransformFragments = ChunkContext.GetFragmentView<FTransformFragment>();
        
        for (int32 EntityIndex = 0; EntityIndex < ChunkContext.GetNumEntities(); ++EntityIndex)
        {
            const FMassEntityHandle Entity = ChunkContext.GetEntity(EntityIndex);
            if (!Entity.IsValid())
            {
                continue;
            }                
            
            const FVector Location = TransformFragments[EntityIndex].GetTransform().GetLocation();

            TWeakObjectPtr<UMPTriggerVolumeComponent> PreviousVolume = VolumeSubsystem->GetVolumeForEntity(Entity);

            UMPTriggerVolumeComponent* CurrentVolume = VolumeSubsystem->FindBestInteractionSourceAtLocation(Location, CandidateBuffer);
                
            if (PreviousVolume.Get() == CurrentVolume)
            {
                continue;
            }
                
            ExecuteVolumeTransition(
               EntityManager,
               ChunkContext,
               EntityIndex,
               Entity,
               PreviousVolume.Get(),
               CurrentVolume,
               StateTreeSubsystem,
               SignalSubsystem);

            VolumeSubsystem->UpdateEntityCurrentVolume(Entity, CurrentVolume);
        }
    });
}

void UMPVolumeTriggerProcessor::ExecuteVolumeTransition(
    FMassEntityManager& EntityManager,
    FMassExecutionContext& Context,
    int32 EntityIndex,
    FMassEntityHandle Entity,
    UMPTriggerVolumeComponent* PreviousVolume,
    UMPTriggerVolumeComponent* CurrentVolume,
    UMassStateTreeSubsystem* StateTreeSubsystem,
    UMassSignalSubsystem* SignalSubsystem)
{
    TArrayView<FMPTriggerVolumeRequestEventFragment> RequestFragments =
        Context.GetMutableFragmentView<FMPTriggerVolumeRequestEventFragment>();

    const TConstArrayView<FMassStateTreeInstanceFragment> InstanceFragments =
        Context.GetFragmentView<FMassStateTreeInstanceFragment>();

    const FMassStateTreeSharedFragment& SharedFragment =
        Context.GetConstSharedFragment<FMassStateTreeSharedFragment>();

    auto ExecuteTasks = [&EntityManager, &Context, Entity](const UMPTriggerVolumeEventData* EventData)
    {
        if (!EventData)
        {
            return;
        }

        for (const TObjectPtr<UMPTriggerVolumeTaskBase>& Task : EventData->Tasks)
        {
            if (Task)
            {
                Task->Execute(EntityManager, Context, Entity);
            }
        }
    };

    if (PreviousVolume)
    {
        ExecuteTasks(PreviousVolume->ExitInteraction);
    }

    if (CurrentVolume)
    {
        ExecuteTasks(CurrentVolume->EnterInteraction);
    }

    FMPTriggerVolumeRequestEventFragment& RequestFragment = RequestFragments[EntityIndex];
    if (RequestFragment.PendingEvents.IsEmpty())
    {
        return;
    }

    const FMassStateTreeInstanceHandle& InstanceHandle = InstanceFragments[EntityIndex].InstanceHandle;
    const UStateTree* StateTreeAsset = SharedFragment.StateTree;

    if (!InstanceHandle.IsValid() || !StateTreeAsset)
    {
        RequestFragment.PendingEvents.Reset();
        return;
    }

    FStateTreeInstanceData* InstanceData = StateTreeSubsystem->GetInstanceData(InstanceHandle);
    if (!InstanceData)
    {
        RequestFragment.PendingEvents.Reset();
        return;
    }

    FStateTreeMinimalExecutionContext StateTreeContext(*StateTreeSubsystem, *StateTreeAsset, *InstanceData);

    for (const FStateTreeEvent& Event : RequestFragment.PendingEvents)
    {
        StateTreeContext.SendEvent(Event.Tag, Event.Payload, NAME_None);
        SignalSubsystem->SignalEntities(UE::Mass::Signals::StateTreeActivate, { Entity });
    }

    RequestFragment.PendingEvents.Reset();
}
