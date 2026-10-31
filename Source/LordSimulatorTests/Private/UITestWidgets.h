#pragma once

#include "UI/LordSimulatorUserWidget.h"
#include "UI/LordSimulatorActivatableWidget.h"
#include "UITestWidgets.generated.h"

// Native fixtures live only in the editor test module, never in packaged content.
UCLASS()
class ULordSimulatorUITestWidget : public ULordSimulatorUserWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override;
};

UCLASS()
class ULordSimulatorUITestScreen : public ULordSimulatorActivatableWidget
{
	GENERATED_BODY()
protected:
	virtual void NativeOnInitialized() override;
};
