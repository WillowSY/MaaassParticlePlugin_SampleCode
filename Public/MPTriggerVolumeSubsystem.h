#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MassEntityHandle.h"
#include "MPHierarchicalBoundsHashGrid3D.h"
#include "MPTriggerVolumeSubsystem.generated.h"

class UMPTriggerVolumeComponent;

/** Runtime Grid에서 Component를 제거하기 위해 필요한 캐시 정보입니다. */
USTRUCT()
struct FMPTriggerVolumeGridEntry
{
    GENERATED_BODY()

    int32 GridItemIndex = INDEX_NONE;
    FBox CachedBounds = FBox(ForceInit);
};

/**
 * TriggerVolume의 등록, 공간 조회, Entity별 현재 Volume 상태를 관리하는 WorldSubsystem입니다.
 */
UCLASS()
class MAAASSPARTICLE_API UMPTriggerVolumeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    void RegisterComponent(UMPTriggerVolumeComponent* Component);
    void UnregisterComponent(UMPTriggerVolumeComponent* Component);

    UMPTriggerVolumeComponent* FindBestInteractionSourceAtLocation(
        const FVector& Location,
        TArray<UMPTriggerVolumeComponent*>& CandidateBuffer) const;

    TWeakObjectPtr<UMPTriggerVolumeComponent> GetVolumeForEntity(const FMassEntityHandle Entity) const;
    void UpdateEntityCurrentVolume(const FMassEntityHandle Entity, UMPTriggerVolumeComponent* VolumeComponent);

private:
    bool IsLocationInsideComponentBounds(const UMPTriggerVolumeComponent* Component, const FVector& Location) const;

private:
    TMPHierarchicalBoundsHashGrid3D<UMPTriggerVolumeComponent*> RuntimeVolumeGrid;

    TMap<TWeakObjectPtr<UMPTriggerVolumeComponent>, FMPTriggerVolumeGridEntry> RuntimeComponentGridData;
    TMap<FMassEntityHandle, TWeakObjectPtr<UMPTriggerVolumeComponent>> EntityToVolumeMap;
};