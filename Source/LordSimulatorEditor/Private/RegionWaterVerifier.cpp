#include "RegionWaterVerifier.h"
#include "Region/LordSimulatorRegion.h"
#include "WaterBodyActor.h"
#include "WaterBodyComponent.h"
#include "WaterBodyRiverActor.h"
#include "WaterBodyLakeActor.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "FileHelpers.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

bool InspectRegionWaterSurfaces(UWorld* World, const TSharedRef<FJsonObject>& Report)
{
    if (!World || !World->PersistentLevel) return false;
    int32 Rivers = 0, Lakes = 0, Meshes = 0;
    int64 Triangles = 0;
    TArray<TSharedPtr<FJsonValue>> Failed;
    for (AActor* Actor : World->PersistentLevel->Actors)
    {
        AWaterBody* Body = Cast<AWaterBody>(Actor);
        if (!Body) continue;
        if (Cast<AWaterBodyRiver>(Body)) ++Rivers;
        else if (Cast<AWaterBodyLake>(Body)) ++Lakes;
        else return false;
        UE::Geometry::FDynamicMesh3 Mesh;
        // Match the engine's FWaterBodyMeshBuilder attribute setup.
        Mesh.EnableAttributes();
        Mesh.Attributes()->EnablePrimaryColors();
        Mesh.Attributes()->EnableTangents();
        if (!Body->GetWaterBodyComponent()->GenerateWaterBodyMesh(Mesh) || Mesh.TriangleCount() == 0)
            Failed.Add(MakeShared<FJsonValueString>(Body->GetActorLabel()));
        else
        {
            ++Meshes;
            Triangles += Mesh.TriangleCount();
        }
    }
    Report->SetNumberField(TEXT("rivers"), Rivers);
    Report->SetNumberField(TEXT("lakes"), Lakes);
    Report->SetNumberField(TEXT("nonempty_surface_meshes"), Meshes);
    Report->SetNumberField(TEXT("surface_triangles"), Triangles);
    Report->SetArrayField(TEXT("failed_bodies"), Failed);
    return Failed.IsEmpty();
}

bool VerifyRegionWater(const FString& Tile)
{
    FIntPoint Cell(INDEX_NONE, INDEX_NONE);
    for (int32 Row = 0; Row < 10; ++Row) for (int32 Column = 0; Column < 10; ++Column)
        if (FLordSimulatorRegion::TileId(FIntPoint(Column, Row)) == Tile) Cell = FIntPoint(Column, Row);
    if (!FLordSimulatorRegion::IsValid(Cell)) return false;
    UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(FLordSimulatorRegion::LevelPath(Cell));
    if (!World || !World->PersistentLevel) return false;
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("tile"), Tile);
    const bool bValid = InspectRegionWaterSurfaces(World, Report);
    FString Output;
    FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&Output));
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("RegionImport/WaterMeshes");
    IFileManager::Get().MakeDirectory(*Directory, true);
    return FFileHelper::SaveStringToFile(Output, *(Directory / (Tile + TEXT(".json")))) && bValid;
}
