#pragma once

#include "CoreMinimal.h"

/** Geographic coordinates, independent of loaded rendering levels and actor lifetimes. */
struct LORDSIMULATOR_API FLordSimulatorRegion
{
    static constexpr int32 GridSize = 10;
    static bool IsValid(FIntPoint Tile) { return Tile.X >= 0 && Tile.X < 10 && Tile.Y >= 0 && Tile.Y < 10; }
    static FString TileId(FIntPoint Tile);
    static FString LevelPath(FIntPoint Tile);
    static FVector LocalToWorldMetres(FIntPoint Tile, const FVector& LocalCentimetres);
};

enum class ELordSimulatorPlotRequest : uint8 { Ignore, Close, Load };

/** Transition state shared by real loading and deterministic failure-path tests. */
struct LORDSIMULATOR_API FLordSimulatorRegionSelectionState
{
    FIntPoint Current = FIntPoint(INDEX_NONE, INDEX_NONE);
    FIntPoint Pending = FIntPoint(INDEX_NONE, INDEX_NONE);
    bool bOpen = false;
    bool bLoading = false;
    bool bLoadFailed = false;

    void Open();
    bool Dismiss();
    ELordSimulatorPlotRequest Select(FIntPoint Tile);
    void Complete(bool bSuccess);
};
