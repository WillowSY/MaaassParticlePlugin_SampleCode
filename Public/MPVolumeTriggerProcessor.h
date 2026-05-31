#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "MPVolumeTriggerProcessor.generated.h"

class UMPTriggerVolumeSubsystem;
class UMPTriggerVolumeComponent;
class UMassStateTreeSubsystem;
class UMassSignalSubsystem;

/**
 * MassEntity 위치를 기준으로 TriggerVolume 진입/이탈을 판정하는 Processor입니다.
 */
UCLASS()
class MAAASSPARTICLE_API UMPVolumeTriggerProcessor : public UMassProcessor
{
    GENERATED_BODY()

public:
    UMPVolumeTriggerProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager>& EntityManager) override;
    virtual void Execute(FMassEntityManager& EntityManager, FMassExecutionContext& Context) override;

private:
    void ExecuteVolumeTransition(
        FMassEntityManager& EntityManager,
        FMassExecutionContext& Context,
        const int32 EntityIndex,
        const FMassEntityHandle Entity,
        UMPTriggerVolumeComponent* PreviousVolume,
        UMPTriggerVolumeComponent* CurrentVolume,
        UMassStateTreeSubsystem* StateTreeSubsystem,
        UMassSignalSubsystem* SignalSubsystem);

private:
    FMassEntityQuery EntityQuery;

    // Entity별 공간 쿼리에서 임시 배열 할당을 줄이기 위한 재사용 버퍼입니다.
    TArray<UMPTriggerVolumeComponent*> CandidateBuffer;

    UPROPERTY(Transient)
    TObjectPtr<UMPTriggerVolumeSubsystem> VolumeSubsystem = nullptr;
};