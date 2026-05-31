#include "MPTriggerVolumeComponent.h"

#include "DrawDebugHelpers.h"
#include "MPTriggerVolumeSubsystem.h"

UMPTriggerVolumeComponent::UMPTriggerVolumeComponent()
{
    // Collision Event를 사용하지 않고 Mass Processor에서 위치 기반으로 판정합니다.
    SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetCollisionResponseToAllChannels(ECR_Ignore);

    ShapeColor = FColor::Cyan;

#if WITH_EDITOR
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
#else
    PrimaryComponentTick.bCanEverTick = false;
#endif

    SetVisibility(false);
    SetHiddenInGame(true);
}

void UMPTriggerVolumeComponent::OnRegister()
{
    Super::OnRegister();

    if (UWorld* World = GetWorld(); World && World->IsGameWorld())
    {
        SetHiddenInGame(!bDebugVisibleInPIE);
        SetVisibility(bDebugVisibleInPIE, true);
        bDrawOnlyIfSelected = !bDebugVisibleInPIE;
    }

    UpdateSubsystemRegistration(true);
}

void UMPTriggerVolumeComponent::OnUnregister()
{
    UpdateSubsystemRegistration(false);
    Super::OnUnregister();
}

void UMPTriggerVolumeComponent::UpdateSubsystemRegistration(bool bRegister)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    UMPTriggerVolumeSubsystem* Subsystem = World->GetSubsystem<UMPTriggerVolumeSubsystem>();
    if (!Subsystem)
    {
        return;
    }

    if (bRegister)
    {
        Subsystem->RegisterComponent(this);
    }
    else
    {
        Subsystem->UnregisterComponent(this);
    }
}

#if WITH_EDITOR
void UMPTriggerVolumeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    UWorld* World = GetWorld();
    if (!World || !bDrawDebug)
    {
        return;
    }

    if (!World->IsGameWorld() && !World->IsEditorWorld())
    {
        return;
    }

    DrawDebugBox(World, GetComponentLocation(), GetScaledBoxExtent(), GetComponentRotation().Quaternion(), DebugColor, false, -1.0f, 0, DebugLineThickness);
}
#endif