#include "LordSimulatorImportRegionCommandlet.h"
#include "RegionWaterVerifier.h"
#include "Algo/Reverse.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Factories/MaterialFactoryNew.h"
#include "FileHelpers.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeInfo.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "MaterialEditingLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "UObject/SavePackage.h"
#include "WaterBodyLakeActor.h"
#include "WaterBodyRiverActor.h"
#include "WaterBodyRiverComponent.h"
#include "WaterSplineComponent.h"
#include "WaterSplineMetadata.h"
#include "WaterZoneActor.h"

DEFINE_LOG_CATEGORY_STATIC(LogLordSimulatorImport, Log, All);

namespace
{
constexpr int32 Vertices = 4033;
constexpr double ScaleXY = 1000000.0 / 4032.0;

bool SaveAsset(UObject* Asset)
{
    UPackage* Package = Asset->GetOutermost();
    Package->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Package, Asset,
        *FPackageName::LongPackageNameToFilename(Package->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}

UMaterial* TerrainMaterial()
{
    const TCHAR* Name = TEXT("/Game/Maps/SouthernCoastRange/M_TerrainVerification");
    if (UMaterial* Existing = LoadObject<UMaterial>(nullptr, TEXT("/Game/Maps/SouthernCoastRange/M_TerrainVerification.M_TerrainVerification"))) return Existing;
    UPackage* Package = CreatePackage(Name);
    UMaterialFactoryNew* Factory = NewObject<UMaterialFactoryNew>();
    UMaterial* Material = Cast<UMaterial>(Factory->FactoryCreateNew(UMaterial::StaticClass(), Package,
        TEXT("M_TerrainVerification"), RF_Public | RF_Standalone, nullptr, GWarn));
    if (!Material) return nullptr;
    auto* Color = Cast<UMaterialExpressionConstant3Vector>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionConstant3Vector::StaticClass()));
    auto* Roughness = Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionConstant::StaticClass()));
    auto* Metallic = Cast<UMaterialExpressionConstant>(UMaterialEditingLibrary::CreateMaterialExpression(Material, UMaterialExpressionConstant::StaticClass()));
    Color->Constant = FLinearColor(0.45f, 0.45f, 0.45f);
    Roughness->R = 1.0f;
    Metallic->R = 0.0f;
    UMaterialEditingLibrary::ConnectMaterialProperty(Color, TEXT(""), MP_BaseColor);
    UMaterialEditingLibrary::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness);
    UMaterialEditingLibrary::ConnectMaterialProperty(Metallic, TEXT(""), MP_Metallic);
    Material->PostEditChange();
    FAssetRegistryModule::AssetCreated(Material);
    return SaveAsset(Material) ? Material : nullptr;
}

bool ReadHeightmap(const FString& Filename, TArray<uint16>& Heights)
{
    TArray<uint8> Compressed;
    if (!FFileHelper::LoadFileToArray(Compressed, *Filename)) return false;
    auto Wrapper = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper")).CreateImageWrapper(EImageFormat::PNG);
    if (!Wrapper->SetCompressed(Compressed.GetData(), Compressed.Num()) || Wrapper->GetWidth() != Vertices
        || Wrapper->GetHeight() != Vertices || Wrapper->GetBitDepth() != 16 || Wrapper->GetFormat() != ERGBFormat::Gray) return false;
    TArray64<uint8> Raw;
    if (!Wrapper->GetRaw(ERGBFormat::Gray, 16, Raw) || Raw.Num() != Vertices * Vertices * sizeof(uint16)) return false;
    Heights.SetNumUninitialized(Vertices * Vertices);
    FMemory::Memcpy(Heights.GetData(), Raw.GetData(), Raw.Num());
    return true;
}

bool CheckHeights(ALandscape* Landscape, const TArray<uint16>& Expected)
{
    ULandscapeInfo* Info = Landscape->GetLandscapeInfo();
    if (!Info || Landscape->LandscapeComponents.Num() != 1024) return false;
    TArray<uint16> Actual;
    Actual.SetNumZeroed(Expected.Num());
    FLandscapeEditDataInterface Edit(Info, false);
    Edit.GetHeightDataFast(0, 0, Vertices - 1, Vertices - 1, Actual.GetData(), Vertices);
    return Actual == Expected && Landscape->GetActorLocation().IsNearlyZero()
        && Landscape->GetActorScale3D().Equals(FVector(ScaleXY, ScaleXY, 1000.0), 0.000001);
}

bool PlaceWater(UWorld* World, const TSharedPtr<FJsonObject>& Data, int32& RiverCount, int32& LakeCount)
{
    FActorSpawnParameters WaterSpawn;
    WaterSpawn.CustomPreSpawnInitialization = [](AActor* Actor)
    {
        // Set this before WaterEditor observes actor creation, so it cannot create a terrain brush/layer.
        if (AWaterBody* Body = Cast<AWaterBody>(Actor)) Body->GetWaterBodyComponent()->bAffectsLandscape = false;
    };
    AWaterZone* Zone = World->SpawnActor<AWaterZone>(FVector(500000, 500000, 0), FRotator::ZeroRotator);
    if (!Zone) return false;
    Zone->SetZoneExtent(FVector2D(1020000, 1020000));
    Zone->SetActorLabel(TEXT("GeneratedWaterZone"));
    for (const TSharedPtr<FJsonValue>& Value : Data->GetArrayField(TEXT("rivers")))
    {
        const auto River = Value->AsObject();
        const auto& SourcePoints = River->GetArrayField(TEXT("points_cm"));
        const auto& Widths = River->GetArrayField(TEXT("width_m"));
        if (SourcePoints.Num() < 2 || Widths.Num() != SourcePoints.Num()) return false;
        TArray<FVector> Points;
        for (const auto& Point : SourcePoints)
        {
            const auto& XYZ = Point->AsArray();
            if (XYZ.Num() != 3) return false;
            Points.Emplace(XYZ[0]->AsNumber(), XYZ[1]->AsNumber(), XYZ[2]->AsNumber());
        }
        AWaterBodyRiver* Body = World->SpawnActor<AWaterBodyRiver>(Points[0], FRotator::ZeroRotator, WaterSpawn);
        if (!Body) return false;
        Body->SetActorLabel(River->GetStringField(TEXT("id")));
        UWaterBodyRiverComponent* Component = CastChecked<UWaterBodyRiverComponent>(Body->GetWaterBodyComponent());
        Component->bAffectsLandscape = false;
        UWaterSplineComponent* Spline = Body->GetWaterSpline();
        Spline->SetSplinePoints(Points, ESplineCoordinateSpace::World, false);
        Spline->SetClosedLoop(false, false);
        Spline->UpdateSpline();
        UWaterSplineMetadata* Metadata = Body->GetWaterSplineMetadata();
        if (!Metadata || Metadata->RiverWidth.Points.Num() != Points.Num()) return false;
        for (int32 Index = 0; Index < Points.Num(); ++Index)
        {
            Component->SetRiverWidthAtSplineInputKey(Index, Widths[Index]->AsNumber() * 100.0);
            Metadata->RiverWidth.Points[Index].InterpMode = CIM_Linear;
        }
        FOnWaterBodyChangedParams Change;
        Change.bShapeOrPositionChanged = true;
        Component->OnWaterBodyChanged(Change);
        for (int32 Index = 0; Index < Points.Num(); ++Index)
        {
            if (!Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World).Equals(Points[Index], 0.01)
                || !FMath::IsNearlyEqual(Component->GetRiverWidthAtSplineInputKey(Index), float(Widths[Index]->AsNumber() * 100.0), 0.01f)) return false;
        }
        if (Component->AffectsLandscape()) return false;
        ++RiverCount;
    }
    for (const TSharedPtr<FJsonValue>& Value : Data->GetArrayField(TEXT("lakes")))
    {
        const auto Lake = Value->AsObject();
        const double Z = Lake->GetNumberField(TEXT("level_m")) * 100.0;
        TArray<FVector> Points;
        for (const auto& Point : Lake->GetArrayField(TEXT("outline_cm")))
        {
            const auto& XY = Point->AsArray();
            if (XY.Num() != 2) return false;
            Points.Emplace(XY[0]->AsNumber(), XY[1]->AsNumber(), Z);
        }
        if (Points.Num() < 3) return false;
        // Water's positive fill rule requires counter-clockwise outlines. Reversing
        // traversal preserves every exported vertex and the exact closed boundary.
        double TwiceArea = 0;
        for (int32 Index = 0; Index < Points.Num(); ++Index)
        {
            const FVector& A = Points[Index];
            const FVector& B = Points[(Index + 1) % Points.Num()];
            TwiceArea += A.X * B.Y - B.X * A.Y;
        }
        if (TwiceArea < 0) Algo::Reverse(Points);
        AWaterBodyLake* Body = World->SpawnActor<AWaterBodyLake>(Points[0], FRotator::ZeroRotator, WaterSpawn);
        if (!Body) return false;
        Body->SetActorLabel(Lake->GetStringField(TEXT("id")));
        UWaterBodyComponent* Component = Body->GetWaterBodyComponent();
        Component->bAffectsLandscape = false;
        UWaterSplineComponent* Spline = Body->GetWaterSpline();
        Spline->SetSplinePoints(Points, ESplineCoordinateSpace::World, false);
        for (int32 Index = 0; Index < Points.Num(); ++Index) Spline->SetSplinePointType(Index, ESplinePointType::Linear, false);
        Spline->SetClosedLoop(true, false);
        Spline->UpdateSpline();
        FOnWaterBodyChangedParams Change;
        Change.bShapeOrPositionChanged = true;
        Component->OnWaterBodyChanged(Change);
        if (Component->AffectsLandscape() || Spline->GetNumberOfSplinePoints() != Points.Num()) return false;
        for (int32 Index = 0; Index < Points.Num(); ++Index)
            if (!Spline->GetLocationAtSplinePoint(Index, ESplineCoordinateSpace::World).Equals(Points[Index], 0.01)) return false;
        ++LakeCount;
    }
    return RiverCount == Data->GetArrayField(TEXT("rivers")).Num() && LakeCount == Data->GetArrayField(TEXT("lakes")).Num();
}
}

ULordSimulatorImportRegionCommandlet::ULordSimulatorImportRegionCommandlet()
{
    IsClient = false;
    IsEditor = true;
    IsServer = false;
    LogToConsole = true;
}

int32 ULordSimulatorImportRegionCommandlet::Main(const FString& Params)
{
    FString SourceRegion, Tile;
    if (!FParse::Value(*Params, TEXT("SourceRegion="), SourceRegion) || !FParse::Value(*Params, TEXT("Tile="), Tile)) return 1;
    int32 Col = INDEX_NONE, Row = INDEX_NONE;
    for (int32 Y = 0; Y < 10; ++Y) for (int32 X = 0; X < 10; ++X)
        if (Tile == FString::Printf(TEXT("tile_x%d_y%d"), X, Y)) { Col = X; Row = Y; }
    if (Col == INDEX_NONE) return 1;
    FString Text;
    TSharedPtr<FJsonObject> Water;
    const FString TilePath = FPaths::Combine(SourceRegion, TEXT("export"), Tile);
    if (!FFileHelper::LoadFileToString(Text, *FPaths::Combine(TilePath, TEXT("water.json")))
        || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Water) || !Water.IsValid()
        || Water->GetStringField(TEXT("plot")) != Tile) return 2;
    TArray<uint16> Heights;
    if (!ReadHeightmap(FPaths::Combine(TilePath, TEXT("heightmap.png")), Heights)) return 3;
    UMaterial* Material = TerrainMaterial();
    if (!Material || !GEditor) return 4;
    UWorld* World = GEditor->NewMap(false);
    if (!World) return 5;
    ALandscape* Landscape = World->SpawnActor<ALandscape>();
    Landscape->LandscapeMaterial = Material;
    Landscape->SetActorScale3D(FVector(ScaleXY, ScaleXY, 1000.0));
    Landscape->StaticLightingLOD = 1;
    Landscape->SetActorLabel(Tile);
    Landscape->Tags.Add(FName(*Tile));
    TMap<FGuid, TArray<uint16>> HeightLayers;
    HeightLayers.Add(FGuid(), Heights);
    TMap<FGuid, TArray<FLandscapeImportLayerInfo>> PaintLayers;
    PaintLayers.Add(FGuid(), {});
    Landscape->Import(FGuid::NewGuid(), 0, 0, Vertices - 1, Vertices - 1, 2, 63, HeightLayers,
        *FPaths::Combine(TilePath, TEXT("heightmap.png")), PaintLayers, ELandscapeImportAlphamapType::Additive, TArrayView<const FLandscapeLayer>());
    if (!CheckHeights(Landscape, Heights)) return 6;
    int32 Rivers = 0, Lakes = 0;
    if (!PlaceWater(World, Water, Rivers, Lakes)) return 7;
    if (!InspectRegionWaterSurfaces(World, MakeShared<FJsonObject>())) return 12;
    // Compare every imported vertex again: water must not change even one encoded height.
    if (!CheckHeights(Landscape, Heights)) return 8;
    auto* Sun = World->SpawnActor<ADirectionalLight>(FVector::ZeroVector, FRotator(-45, -45, 0));
    Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sun->GetLightComponent()->SetLightColor(FLinearColor::White);
    Sun->GetLightComponent()->SetIntensity(3.0f);
    auto* Sky = World->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetIntensity(1.0f);
    Sky->GetLightComponent()->SetRealTimeCaptureEnabled(false);
    const FString Package = TEXT("/Game/Maps/SouthernCoastRange/") + Tile;
    if (!UEditorLoadingAndSavingUtils::SaveMap(World, Package)) return 9;
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("tile"), Tile);
    Report->SetStringField(TEXT("level"), Package);
    Report->SetNumberField(TEXT("vertices"), Vertices);
    Report->SetNumberField(TEXT("rivers"), Rivers);
    Report->SetNumberField(TEXT("lakes"), Lakes);
    Report->SetBoolField(TEXT("all_encoded_heights_unchanged"), true);
    Report->SetNumberField(TEXT("scale_xy_cm"), ScaleXY);
    Report->SetNumberField(TEXT("scale_z"), 1000);
    FString ReportText;
    FJsonSerializer::Serialize(Report, TJsonWriterFactory<>::Create(&ReportText));
    const FString ReportFolder = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("RegionImport"));
    IFileManager::Get().MakeDirectory(*ReportFolder, true);
    if (!FFileHelper::SaveStringToFile(ReportText, *FPaths::Combine(ReportFolder, Tile + TEXT(".json")))) return 10;
    UE_LOG(LogLordSimulatorImport, Display, TEXT("VERIFIED %s: 4033x4033, 1024 components, %d rivers, %d lakes, all heights unchanged, north-up X=east Y=south"), *Tile, Rivers, Lakes);
    return 0;
}
