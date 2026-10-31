#pragma once

#include "CommonActivatableWidget.h"
#include "UI/LordSimulatorUILayer.h"
#include "LordSimulatorActivatableWidget.generated.h"

/** CommonUI lifecycle for screens requiring activation, focus, and routed input. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class LORDSIMULATOR_API ULordSimulatorActivatableWidget : public UCommonActivatableWidget
{
	GENERATED_BODY()
public:
	ULordSimulatorActivatableWidget();
	virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
protected:
	virtual UWidget* NativeGetDesiredFocusTarget() const override;
private:
	friend class ULordSimulatorPrimaryUILayout;
	void ConfigureForLayer(ELordSimulatorUILayer Layer);
	TOptional<ELordSimulatorUILayer> ManagedLayer;
};
