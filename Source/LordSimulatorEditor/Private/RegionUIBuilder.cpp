#include "RegionUIBuilder.h"
#include "UI/LordSimulatorRegionOverview.h"
#include "UI/LordSimulatorRegionSelectionWidget.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Internationalization/StringTable.h"
#include "Engine/Texture2D.h"
#include "WidgetBlueprintFactory.h"
#include "Internationalization/StringTableCore.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Styling/CoreStyle.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
bool Save(UObject* Asset)
{
    Asset->MarkPackageDirty();
    FSavePackageArgs Args;
    Args.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(Asset->GetOutermost(), Asset,
        *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}

UCanvasPanelSlot* Place(UCanvasPanel* Root, UWidget* Widget, FAnchors Anchors,
    FVector2D Alignment, FVector2D Size, bool bAutoSize)
{
    UCanvasPanelSlot* Slot = Root->AddChildToCanvas(Widget);
    Slot->SetAnchors(Anchors);
    Slot->SetOffsets(FMargin(0, 0, Size.X, Size.Y));
    Slot->SetAlignment(Alignment);
    Slot->SetAutoSize(bAutoSize);
    return Slot;
}
}

bool PrepareRegionUI(const FString& OverviewFile)
{
    TArray<uint8> Compressed;
    if (!FFileHelper::LoadFileToArray(Compressed, *OverviewFile)) return false;
    auto PNG = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper")).CreateImageWrapper(EImageFormat::PNG);
    TArray64<uint8> Pixels;
    if (!PNG->SetCompressed(Compressed.GetData(), Compressed.Num()) || PNG->GetWidth() != 1024 || PNG->GetHeight() != 1024
        || !PNG->GetRaw(ERGBFormat::Gray, 8, Pixels)) return false;
    const TCHAR* TextureName = TEXT("/Game/UI/Screens/RegionSelection/T_RegionOverview");
    UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Screens/RegionSelection/T_RegionOverview.T_RegionOverview"));
    if (!Texture)
    {
        Texture = NewObject<UTexture2D>(CreatePackage(TextureName), TEXT("T_RegionOverview"), RF_Public | RF_Standalone);
        FAssetRegistryModule::AssetCreated(Texture);
    }
    // Slate samples RGB directly; a single-channel GPU texture renders red.
    TArray64<uint8> ColourPixels;
    ColourPixels.SetNumUninitialized(Pixels.Num() * 4);
    for (int64 Index = 0; Index < Pixels.Num(); ++Index)
    {
        ColourPixels[Index * 4] = ColourPixels[Index * 4 + 1] = ColourPixels[Index * 4 + 2] = Pixels[Index];
        ColourPixels[Index * 4 + 3] = 255;
    }
    Texture->Source.Init(1024, 1024, 1, 1, TSF_BGRA8, ColourPixels.GetData());
    Texture->SRGB = false;
    Texture->CompressionSettings = TC_EditorIcon;
    Texture->MipGenSettings = TMGS_NoMipmaps;
    Texture->LODGroup = TEXTUREGROUP_UI;
    Texture->Filter = TF_Bilinear;
    Texture->PostEditChange();
    if (!Save(Texture)) return false;

    UStringTable* Strings = LoadObject<UStringTable>(nullptr, TEXT("/Game/Localization/ST_LordSimulator.ST_LordSimulator"));
    if (!Strings) return false;
    Strings->GetMutableStringTable()->SetSourceString(TEXT("NewGame.RegionSelection.Title"), TEXT("Select a region"), FString());
    Strings->GetMutableStringTable()->SetSourceString(TEXT("NewGame.RegionSelection.Back"), TEXT("Back"), FString());
    Strings->GetMutableStringTable()->SetSourceString(TEXT("NewGame.RegionSelection.LoadFailed"), TEXT("Unable to load the selected region."), FString());
    if (!Save(Strings)) return false;

    const TCHAR* BlueprintName = TEXT("/Game/UI/Screens/RegionSelection/WBP_RegionSelection");
    UWidgetBlueprint* Blueprint = LoadObject<UWidgetBlueprint>(nullptr,
        TEXT("/Game/UI/Screens/RegionSelection/WBP_RegionSelection.WBP_RegionSelection"));
    if (!Blueprint)
    {
        UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
        Factory->ParentClass = ULordSimulatorRegionSelectionWidget::StaticClass();
        Blueprint = Cast<UWidgetBlueprint>(Factory->FactoryCreateNew(UWidgetBlueprint::StaticClass(),
            CreatePackage(BlueprintName), TEXT("WBP_RegionSelection"), RF_Public | RF_Standalone, nullptr, GWarn));
        if (!Blueprint) return false;
        FAssetRegistryModule::AssetCreated(Blueprint);
    }
    UWidgetTree* Tree = Blueprint->WidgetTree;
    UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("RegionCanvas"));
    Tree->RootWidget = Root;
    UBorder* Background = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Background"));
    Background->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f));
    Background->SetVisibility(ESlateVisibility::HitTestInvisible);
    Place(Root, Background, FAnchors(0, 0, 1, 1), FVector2D::ZeroVector, FVector2D::ZeroVector, false);

    UScaleBox* Scale = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("OverviewFit"));
    Scale->SetStretch(EStretch::ScaleToFit);
    Scale->SetStretchDirection(EStretchDirection::Both);
    Place(Root, Scale, FAnchors(0.1f, 0.1f, 0.9f, 0.9f), FVector2D::ZeroVector, FVector2D::ZeroVector, false);
    USizeBox* Square = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("OverviewSquare"));
    Square->SetWidthOverride(1024);
    Square->SetHeightOverride(1024);
    Scale->SetContent(Square);
    ULordSimulatorRegionOverview* Overview = Tree->ConstructWidget<ULordSimulatorRegionOverview>(ULordSimulatorRegionOverview::StaticClass(), TEXT("Overview"));
    Overview->OverviewTexture = Texture;
    Square->SetContent(Overview);

    UTextBlock* Title = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RegionTitle"));
    UObject* Font = LoadObject<UObject>(nullptr, TEXT("/Engine/EngineFonts/Roboto.Roboto"));
    if (!Font) return false;
    Title->SetFont(FSlateFontInfo(Font, 32, TEXT("Regular")));
    Title->SetJustification(ETextJustify::Center);
    Place(Root, Title, FAnchors(0.5f, 0.05f), FVector2D(0.5, 0.5), FVector2D::ZeroVector, true);
    UButton* Back = Tree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("BackButton"));
    Place(Root, Back, FAnchors(0.5f, 0.95f), FVector2D(0.5, 0.5), FVector2D(200, 40), false);
    UTextBlock* BackText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BackLabel"));
    BackText->SetFont(FSlateFontInfo(Font, 24, TEXT("Regular")));
    BackText->SetJustification(ETextJustify::Center);
    Back->SetContent(BackText);

    UBorder* Failure = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FailurePanel"));
    Failure->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f));
    Failure->SetPadding(FMargin(16));
    Place(Root, Failure, FAnchors(0.5f, 0.5f), FVector2D(0.5, 0.5), FVector2D::ZeroVector, true);
    UTextBlock* FailureText = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("LoadFailedLabel"));
    FailureText->SetFont(FSlateFontInfo(Font, 24, TEXT("Regular")));
    FailureText->SetJustification(ETextJustify::Center);
    FailureText->SetWrapTextAt(600);
    Failure->SetContent(FailureText);
    Failure->SetVisibility(ESlateVisibility::Collapsed);
    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    return Blueprint->Status != BS_Error && Save(Blueprint);
}
