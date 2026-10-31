#pragma once

#include "CommonUserWidget.h"
#include "LordSimulatorUserWidget.generated.h"

/** Default presentation base. Feature data arrives through explicit events/update calls. */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class LORDSIMULATOR_API ULordSimulatorUserWidget : public UCommonUserWidget
{
	GENERATED_BODY()
};
