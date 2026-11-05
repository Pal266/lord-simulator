#pragma once

#include "Camera/CameraActor.h"
#include "LordSimulatorRegionCamera.generated.h"

class ULordSimulatorRegionSubsystem;

/** Non-colliding rendering-test camera, with no pawn or simulation responsibilities. */
UCLASS(NotBlueprintable)
class LORDSIMULATOR_API ALordSimulatorRegionCamera : public ACameraActor
{
    GENERATED_BODY()
public:
    ALordSimulatorRegionCamera();
    void Initialize(ULordSimulatorRegionSubsystem* InOwner, APlayerController* Controller);
    virtual void Tick(float DeltaSeconds) override;
private:
    void OpenSelection();
    void Faster();
    void Slower();
    TWeakObjectPtr<ULordSimulatorRegionSubsystem> Region;
    TWeakObjectPtr<APlayerController> InputController;
    double SpeedCentimetres = 10000.0;
};
