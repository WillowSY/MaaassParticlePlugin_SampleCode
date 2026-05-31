#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "MPTriggerVolumeEventData.h"
#include "MPTriggerVolumeComponent.generated.h"

/**
 * MassEntity가 진입/이탈할 수 있는 레벨 배치용 TriggerVolume Component입니다.
 *
 * C++은 런타임 등록/해제와 Debug Draw를 담당하고,
 * Enter / Exit 이벤트 구성은 DataAsset으로 분리했습니다.
 */
UCLASS(ClassGroup=(MaaassParticle), meta=(BlueprintSpawnableComponent))
class MAAASSPARTICLE_API UMPTriggerVolumeComponent : public UBoxComponent
{
    GENERATED_BODY()

public:
    UMPTriggerVolumeComponent();

protected:
    virtual void OnRegister() override;
    virtual void OnUnregister() override;

#if WITH_EDITOR
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
#endif

public:
    /** PIE에서 TriggerVolume 디버그 박스를 표시할지 여부입니다. */
    UPROPERTY(EditAnywhere, Category="MP TriggerVolume|Debug")
    bool bDebugVisibleInPIE = true;

    /** Entity가 Volume에 진입했을 때 실행할 Task 목록입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MP TriggerVolume")
    TObjectPtr<UMPTriggerVolumeEventData> EnterInteraction;

    /** Entity가 Volume에서 이탈했을 때 실행할 Task 목록입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MP TriggerVolume")
    TObjectPtr<UMPTriggerVolumeEventData> ExitInteraction;

    /** 여러 Volume이 겹칠 때 선택 우선순위입니다. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="MP TriggerVolume")
    int32 Priority = 0;

    UPROPERTY(EditAnywhere, Category="MP TriggerVolume|Debug")
    bool bDrawDebug = true;

    UPROPERTY(EditAnywhere, Category="MP TriggerVolume|Debug", meta=(EditCondition="bDrawDebug"))
    FColor DebugColor = FColor::Yellow;

    UPROPERTY(EditAnywhere, Category="MP TriggerVolume|Debug", meta=(EditCondition="bDrawDebug", ClampMin="0.0"))
    float DebugLineThickness = 2.0f;

private:
    /** WorldSubsystem에 Component를 등록하거나 해제합니다. */
    void UpdateSubsystemRegistration(bool bRegister);
};