#pragma once
#include "CoreMinimal.h"
class UWorld;
class FJsonObject;

bool InspectRegionWaterSurfaces(UWorld* World, const TSharedRef<FJsonObject>& Report);

/** Read an existing prepared level and require a nonempty native surface mesh for every water body. */
bool VerifyRegionWater(const FString& Tile);
