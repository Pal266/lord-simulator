#pragma once

#include "UI/LordSimulatorActivatableWidget.h"
#include "LordSimulatorRegionSelectionWidget.generated.h"

class UButton;
class UBorder;
class UTextBlock;
class ULordSimulatorRegionOverview;
struct FLordSimulatorRegionSelectionState;

/** The Widget Blueprint owns layout; C++ owns commands, localization and presentation updates. */
UCLASS(Abstract, Blueprintable, meta=(DisableNativeTick))
class LORDSIMULATOR_API ULordSimulatorRegionSelectionWidget : public ULordSimulatorActivatableWidget
{
    GENERATED_BODY()
public:
    ULordSimulatorRegionSelectionWidget();
    void Present(const FLordSimulatorRegionSelectionState& State);
protected:
    virtual void NativeOnInitialized() override;
    virtual FReply NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event) override;
    virtual bool NativeOnHandleBackAction() override;
    virtual UWidget* NativeGetDesiredFocusTarget() const override;
private:
    UFUNCTION() void Back();
    UFUNCTION() void SelectCell(int32 Column, int32 Row);
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RegionTitle;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> BackLabel;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> LoadFailedLabel;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UBorder> FailurePanel;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UButton> BackButton;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<ULordSimulatorRegionOverview> Overview;
};
