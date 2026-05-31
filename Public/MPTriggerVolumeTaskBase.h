#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "MPTriggerVolumeTaskBase.generated.h"

class FMassEntityManager;
struct FMassExecutionContext;

/**
 * TriggerVolume Enter / Exit 시 실행할 Task의 기본 클래스입니다.
 *
 * 파생 Task는 DataAsset에 Instanced Object로 등록되며,
 * VolumeTriggerProcessor가 Entity의 Volume 변경을 감지했을 때 실행합니다.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, CollapseCategories)
class MAAASSPARTICLE_API UMPTriggerVolumeTaskBase : public UObject
{
    GENERATED_BODY()

public:
    virtual void Execute(
        FMassEntityManager& EntityManager,
        FMassExecutionContext& Context,
        const FMassEntityHandle Entity) const
    {
    }
};