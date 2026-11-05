#include "Region/LordSimulatorRegion.h"

FString FLordSimulatorRegion::TileId(FIntPoint Tile)
{
    return IsValid(Tile) ? FString::Printf(TEXT("tile_x%d_y%d"), Tile.X, Tile.Y) : FString();
}

FString FLordSimulatorRegion::LevelPath(FIntPoint Tile)
{
    return IsValid(Tile) ? TEXT("/Game/Maps/SouthernCoastRange/") + TileId(Tile) : FString();
}

FVector FLordSimulatorRegion::LocalToWorldMetres(FIntPoint Tile, const FVector& LocalCentimetres)
{
    return FVector(-50000.0 + Tile.X * 10000.0 + LocalCentimetres.X / 100.0,
        50000.0 - Tile.Y * 10000.0 - LocalCentimetres.Y / 100.0, LocalCentimetres.Z / 100.0);
}

void FLordSimulatorRegionSelectionState::Open()
{
    if (bLoading) return;
    bOpen = true;
    bLoadFailed = false;
}

bool FLordSimulatorRegionSelectionState::Dismiss()
{
    if (!bOpen || bLoading) return false;
    bOpen = false;
    bLoadFailed = false;
    return true;
}

ELordSimulatorPlotRequest FLordSimulatorRegionSelectionState::Select(FIntPoint Tile)
{
    if (!bOpen || bLoading || !FLordSimulatorRegion::IsValid(Tile)) return ELordSimulatorPlotRequest::Ignore;
    if (Tile == Current)
    {
        Dismiss();
        return ELordSimulatorPlotRequest::Close;
    }
    Pending = Tile;
    bLoading = true;
    bLoadFailed = false;
    return ELordSimulatorPlotRequest::Load;
}

void FLordSimulatorRegionSelectionState::Complete(bool bSuccess)
{
    if (!bLoading) return;
    if (bSuccess) Current = Pending;
    Pending = FIntPoint(INDEX_NONE, INDEX_NONE);
    bLoading = false;
    bOpen = !bSuccess;
    bLoadFailed = !bSuccess;
}
