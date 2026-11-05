#pragma once

#include "Components/Widget.h"
#include "LordSimulatorRegionOverview.generated.h"

class UTexture2D;
class SRegionOverview;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FLordSimulatorRegionCellSelected, int32, Column, int32, Row);

/** A single data-driven grid, avoiding 100 independently polling UI widgets. */
UCLASS()
class LORDSIMULATOR_API ULordSimulatorRegionOverview : public UWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, Category="Overview") TObjectPtr<UTexture2D> OverviewTexture;
    UPROPERTY() FLordSimulatorRegionCellSelected OnCellSelected;
    void Present(FIntPoint Current, bool bAcceptSelection);
    virtual void ReleaseSlateResources(bool bReleaseChildren) override;
protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
private:
    TSharedPtr<SRegionOverview> SlateOverview;
    FIntPoint PresentedCurrent = FIntPoint(INDEX_NONE, INDEX_NONE);
    bool bPresentedAcceptSelection = true;
};
