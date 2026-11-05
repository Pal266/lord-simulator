#pragma once

#include "Subsystems/LocalPlayerSubsystem.h"
#include "Region/LordSimulatorRegion.h"
#include "TimerManager.h"
#include "LordSimulatorRegionSubsystem.generated.h"

class ALordSimulatorRegionCamera;
class ULevelStreamingDynamic;
class ULordSimulatorRegionSelectionWidget;

/** Owns this local player's temporary terrain-view navigation, never world simulation state. */
UCLASS()
class LORDSIMULATOR_API ULordSimulatorRegionSubsystem : public ULocalPlayerSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    bool OpenSelection();
    void DismissSelection();
    void SelectTile(FIntPoint Tile);
    bool CanMoveCamera() const;
    const FLordSimulatorRegionSelectionState& GetSelectionState() const { return State; }
private:
    friend struct FLordSimulatorRegionTestAccess;
#if WITH_DEV_AUTOMATION_TESTS
    TFunction<bool(const FString&)> PackageAvailabilityForTesting;
#endif
    UFUNCTION() void DestinationLoaded();
    UFUNCTION() void DestinationShown();
    UFUNCTION() void PreviousUnloaded();
    void FailLoad();
    void CloseScreen();
    void UpdateScreen();
    bool CheckDestination() const;
    FLordSimulatorRegionSelectionState State;
    UPROPERTY(Transient) TObjectPtr<ULordSimulatorRegionSelectionWidget> Screen;
    UPROPERTY(Transient) TObjectPtr<ULevelStreamingDynamic> CurrentLevel;
    UPROPERTY(Transient) TObjectPtr<ULevelStreamingDynamic> Destination;
    UPROPERTY(Transient) TObjectPtr<ALordSimulatorRegionCamera> Camera;
    FVector DestinationCameraLocation = FVector::ZeroVector;
    FTimerHandle LoadTimeout;
};
