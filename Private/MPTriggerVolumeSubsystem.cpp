#include "MPTriggerVolumeSubsystem.h"
#include "MPTriggerVolumeComponent.h"

void UMPTriggerVolumeSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Level 0: 작은 TriggerVolume / Level 1~2: 넓은 범위 확장용
    const TArray<double> CellSizes = { 100.0, 1000.0, 10000.0 };
    RuntimeVolumeGrid.Init(CellSizes.Num(), CellSizes);
}

void UMPTriggerVolumeSubsystem::RegisterComponent(UMPTriggerVolumeComponent* Component)
{
    if (!Component || !Component->GetOwner())
    {
        return;
    }

    if (RuntimeComponentGridData.Contains(Component))
    {
        return;
    }

    const FBox Bounds = Component->Bounds.GetBox();
    const int32 ItemIndex = RuntimeVolumeGrid.Add(Component, Bounds, 0);

    FMPTriggerVolumeGridEntry& Entry = RuntimeComponentGridData.FindOrAdd(Component);
    Entry.GridItemIndex = ItemIndex;
    Entry.CachedBounds = Bounds;
}

void UMPTriggerVolumeSubsystem::UnregisterComponent(UMPTriggerVolumeComponent* Component)
{
    if (!Component)
    {
        return;
    }

    const FMPTriggerVolumeGridEntry* Entry = RuntimeComponentGridData.Find(Component);
    if (!Entry)
    {
        return;
    }

    RuntimeVolumeGrid.Remove(Entry->GridItemIndex, Entry->CachedBounds);
    RuntimeComponentGridData.Remove(Component);

    for (auto It = EntityToVolumeMap.CreateIterator(); It; ++It)
    {
        if (It.Value().Get() == Component)
        {
            It.RemoveCurrent();
        }
    }

}

UMPTriggerVolumeComponent* UMPTriggerVolumeSubsystem::FindBestInteractionSourceAtLocation(
    const FVector& Location,
    TArray<UMPTriggerVolumeComponent*>& CandidateBuffer) const
{
    constexpr int32 QueryLevel = 0;

    CandidateBuffer.Reset();
    RuntimeVolumeGrid.FindOverlapping(Location, QueryLevel, CandidateBuffer);

    UMPTriggerVolumeComponent* BestComponent = nullptr;
    int32 HighestPriority = TNumericLimits<int32>::Lowest();

    for (UMPTriggerVolumeComponent* Candidate : CandidateBuffer)
    {
        if (!Candidate)
        {
            continue;
        }

        // Grid는 후보군만 줄이는 Broad Phase입니다. 실제 포함 여부는 Component 기준으로 다시 검사합니다.
        if (!IsLocationInsideComponentBounds(Candidate, Location))
        {
            continue;
        }

        if (!BestComponent || Candidate->Priority > HighestPriority)
        {
            BestComponent = Candidate;
            HighestPriority = Candidate->Priority;
        }
    }

    return BestComponent;
}

bool UMPTriggerVolumeSubsystem::IsLocationInsideComponentBounds(const UMPTriggerVolumeComponent* Component, const FVector& Location) const
{
    if (!Component)
    {
        return false;
    }
    
    // 회전된 BoxComponent도 정확히 판정하기 위해 월드 좌표를 Component Local Space로 변환합니다.
    const FVector LocalLocation = Component->GetComponentTransform().InverseTransformPosition(Location);
    const FVector Extent = Component->GetUnscaledBoxExtent();

    return FMath::Abs(LocalLocation.X) <= Extent.X
        && FMath::Abs(LocalLocation.Y) <= Extent.Y
        && FMath::Abs(LocalLocation.Z) <= Extent.Z;
}

TWeakObjectPtr<UMPTriggerVolumeComponent> UMPTriggerVolumeSubsystem::GetVolumeForEntity(const FMassEntityHandle Entity) const
{
    return EntityToVolumeMap.FindRef(Entity);
}

void UMPTriggerVolumeSubsystem::UpdateEntityCurrentVolume(const FMassEntityHandle Entity, UMPTriggerVolumeComponent* VolumeComponent)
{
    if (!Entity.IsValid())
    {
        return;
    }

    if (VolumeComponent)
    {
        EntityToVolumeMap.Add(Entity, VolumeComponent);
    }
    else
    {
        EntityToVolumeMap.Remove(Entity);
    }
}