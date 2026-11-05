#include "UI/LordSimulatorRegionOverview.h"
#include "Region/LordSimulatorRegion.h"
#include "Brushes/SlateImageBrush.h"
#include "Engine/Texture2D.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Widgets/SLeafWidget.h"

class SRegionOverview : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SRegionOverview) {} SLATE_END_ARGS()
    void Construct(const FArguments&, UTexture2D* Texture, TFunction<void(FIntPoint)> InSelected)
    {
        Brush.SetResourceObject(Texture);
        Brush.ImageSize = FVector2D(1024, 1024);
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Selected = MoveTemp(InSelected);
    }
    void Present(FIntPoint InCurrent, bool bInAcceptSelection)
    {
        Current = InCurrent;
        bAcceptSelection = bInAcceptSelection;
        Invalidate(EInvalidateWidgetReason::Paint);
    }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1024, 1024); }
    virtual FReply OnMouseMove(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        const FIntPoint NewHovered = CellAt(Geometry, Event);
        if (Hovered != NewHovered) { Hovered = NewHovered; Invalidate(EInvalidateWidgetReason::Paint); }
        return FReply::Handled();
    }
    virtual void OnMouseLeave(const FPointerEvent& Event) override
    {
        SLeafWidget::OnMouseLeave(Event);
        Hovered = FIntPoint(INDEX_NONE, INDEX_NONE);
        Invalidate(EInvalidateWidgetReason::Paint);
    }
    virtual FReply OnMouseButtonDown(const FGeometry& Geometry, const FPointerEvent& Event) override
    {
        if (Event.GetEffectingButton() != EKeys::LeftMouseButton) return FReply::Unhandled();
        const FIntPoint Cell = CellAt(Geometry, Event);
        if (bAcceptSelection && FLordSimulatorRegion::IsValid(Cell)) Selected(Cell);
        return FReply::Handled();
    }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
        FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle&, bool) const override
    {
        FSlateDrawElement::MakeBox(Elements, Layer, Geometry.ToPaintGeometry(), &Brush);
        const FVector2D Size = Geometry.GetLocalSize();
        // Slate line thickness is already expressed in output pixels.
        const float Pixel = 1.0f;
        for (int32 Line = 0; Line <= 10; ++Line)
        {
            const float X = Size.X * Line / 10.0;
            const float Y = Size.Y * Line / 10.0;
            TArray<FVector2D> Vertical = {FVector2D(X, 0), FVector2D(X, Size.Y)};
            TArray<FVector2D> Horizontal = {FVector2D(0, Y), FVector2D(Size.X, Y)};
            FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), Vertical, ESlateDrawEffect::None, FLinearColor::White, false, Pixel);
            FSlateDrawElement::MakeLines(Elements, Layer + 1, Geometry.ToPaintGeometry(), Horizontal, ESlateDrawEffect::None, FLinearColor::White, false, Pixel);
        }
        auto Outline = [&](FIntPoint Cell, FLinearColor Color, int32 OutlineLayer)
        {
            if (!FLordSimulatorRegion::IsValid(Cell)) return;
            const FVector2D Min(Size.X * Cell.X / 10.0, Size.Y * Cell.Y / 10.0);
            const FVector2D Max(Size.X * (Cell.X + 1) / 10.0, Size.Y * (Cell.Y + 1) / 10.0);
            TArray<FVector2D> Points = {Min, FVector2D(Max.X, Min.Y), Max, FVector2D(Min.X, Max.Y), Min};
            FSlateDrawElement::MakeLines(Elements, OutlineLayer, Geometry.ToPaintGeometry(), Points, ESlateDrawEffect::None, Color, false, 3 * Pixel);
        };
        Outline(Current, FLinearColor(0, 1, 1), Layer + 2);
        Outline(Hovered, FLinearColor(1, 1, 0), Layer + 3);
        return Layer + 3;
    }
private:
    static FIntPoint CellAt(const FGeometry& Geometry, const FPointerEvent& Event)
    {
        const FVector2D Point = Geometry.AbsoluteToLocal(Event.GetScreenSpacePosition());
        const FVector2D Size = Geometry.GetLocalSize();
        if (Size.X <= 0 || Size.Y <= 0 || Point.X < 0 || Point.Y < 0 || Point.X >= Size.X || Point.Y >= Size.Y)
            return FIntPoint(INDEX_NONE, INDEX_NONE);
        return FIntPoint(FMath::FloorToInt(Point.X * 10 / Size.X), FMath::FloorToInt(Point.Y * 10 / Size.Y));
    }
    FSlateBrush Brush;
    FIntPoint Current = FIntPoint(INDEX_NONE, INDEX_NONE);
    FIntPoint Hovered = FIntPoint(INDEX_NONE, INDEX_NONE);
    bool bAcceptSelection = true;
    TFunction<void(FIntPoint)> Selected;
};

TSharedRef<SWidget> ULordSimulatorRegionOverview::RebuildWidget()
{
    TWeakObjectPtr<ULordSimulatorRegionOverview> WeakThis(this);
    SlateOverview = SNew(SRegionOverview, OverviewTexture, [WeakThis](FIntPoint Cell)
    {
        if (WeakThis.IsValid()) WeakThis->OnCellSelected.Broadcast(Cell.X, Cell.Y);
    });
    SlateOverview->Present(PresentedCurrent, bPresentedAcceptSelection);
    return SlateOverview.ToSharedRef();
}

void ULordSimulatorRegionOverview::Present(FIntPoint Current, bool bAcceptSelection)
{
    PresentedCurrent = Current;
    bPresentedAcceptSelection = bAcceptSelection;
    if (SlateOverview) SlateOverview->Present(Current, bAcceptSelection);
}

void ULordSimulatorRegionOverview::ReleaseSlateResources(bool bReleaseChildren)
{
    Super::ReleaseSlateResources(bReleaseChildren);
    SlateOverview.Reset();
}
