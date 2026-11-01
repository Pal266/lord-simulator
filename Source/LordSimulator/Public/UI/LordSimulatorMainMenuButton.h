#pragma once

#include "Components/Button.h"
#include "LordSimulatorMainMenuButton.generated.h"

/**
 * Main-menu button whose focusability is configured by production C++ before
 * the underlying Slate button is constructed.
 */
UCLASS(meta = (DisableNativeTick))
class LORDSIMULATOR_API ULordSimulatorMainMenuButton : public UButton
{
	GENERATED_BODY()

public:
	/** Must be called during owning-widget initialization, before Slate construction. */
	void InitializeFocusability(bool bFocusable)
	{
		InitIsFocusable(bFocusable);
	}
};
