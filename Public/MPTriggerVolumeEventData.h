#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MPTriggerVolumeTaskBase.h"
#include "MPTriggerVolumeEventData.generated.h"

/**
 * TriggerVolume Enter / Exit 시 실행할 Task 목록을 정의하는 DataAsset입니다.
 *
 * Volume별 이벤트 내용을 C++에 하드코딩하지 않고,
 * 에디터에서 Task 조합으로 구성할 수 있도록 분리했습니다.
 */
UCLASS(Blueprintable, BlueprintType, meta = (DisplayName = "MP TriggerVolume Event Data"))
class MAAASSPARTICLE_API UMPTriggerVolumeEventData : public UDataAsset
{
    GENERATED_BODY()

public:
    /** Volume 이벤트가 발생했을 때 순서대로 실행할 Task 목록입니다. */
    UPROPERTY(EditAnywhere, Instanced, Category = "TriggerVolume", meta = (DisplayName = "TriggerVolume Tasks"))
    TArray<TObjectPtr<UMPTriggerVolumeTaskBase>> Tasks;
};
