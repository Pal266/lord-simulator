#pragma once

#include "CoreMinimal.h"
#include "LordSimulatorUILayer.generated.h"

UENUM(BlueprintType)
enum class ELordSimulatorUILayer : uint8
{
	Game,
	Menu,
	Modal,
	PassiveOverlay
};
