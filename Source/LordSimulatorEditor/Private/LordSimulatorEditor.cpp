#include "Modules/ModuleManager.h"
#include "HAL/IConsoleManager.h"
#include "LordSimulatorImportRegionCommandlet.h"
#include "RegionUIBuilder.h"
#include "RegionWaterVerifier.h"

static FAutoConsoleCommand VerifyRegionWaterCommand(
    TEXT("LordSimulator.VerifyRegionWater"), TEXT("Offline surface verification: Tile=tile_x{column}_y{row}"),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        FString Tile;
        const FString Params = FString::Join(Args, TEXT(" "));
        const bool bSuccess = FParse::Value(*Params, TEXT("Tile="), Tile) && VerifyRegionWater(Tile);
        UE_LOG(LogTemp, Display, TEXT("LordSimulator.VerifyRegionWater result: %d"), bSuccess ? 0 : 1);
    }));

// The same importer can run in an already-open editor, without spawning another process.
static FAutoConsoleCommand ImportRegionCommand(
    TEXT("LordSimulator.ImportRegion"),
    TEXT("Offline import: SourceRegion=<directory> Tile=tile_x{column}_y{row}"),
    FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
    {
        ULordSimulatorImportRegionCommandlet* Importer = NewObject<ULordSimulatorImportRegionCommandlet>();
        const FString Params = FString::Join(Args, TEXT(" "));
        FString Overview;
        const int32 Result = FParse::Value(*Params, TEXT("OverviewFile="), Overview)
            ? (PrepareRegionUI(Overview) ? 0 : 11) : Importer->Main(Params);
        UE_LOG(LogTemp, Display, TEXT("LordSimulator.ImportRegion result: %d"), Result);
    }));
IMPLEMENT_MODULE(FDefaultModuleImpl, LordSimulatorEditor)
