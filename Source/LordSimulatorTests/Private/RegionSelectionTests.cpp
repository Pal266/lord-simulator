#include "Region/LordSimulatorRegion.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordSimulatorRegionTopologyTest, "LordSimulator.Region.Topology",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLordSimulatorRegionTopologyTest::RunTest(const FString&)
{
    TSet<FString> Levels;
    for (int32 Row = 0; Row < 10; ++Row) for (int32 Column = 0; Column < 10; ++Column)
    {
        const FIntPoint Cell(Column, Row);
        TestTrue(TEXT("Every overview cell is valid"), FLordSimulatorRegion::IsValid(Cell));
        const FString Level = FLordSimulatorRegion::LevelPath(Cell);
        TestFalse(TEXT("Every cell selects a distinct level"), Levels.Contains(Level));
        Levels.Add(Level);
        const FVector NW = FLordSimulatorRegion::LocalToWorldMetres(Cell, FVector::ZeroVector);
        const FVector SE = FLordSimulatorRegion::LocalToWorldMetres(Cell, FVector(1000000, 1000000, 0));
        TestEqual(TEXT("Plots are exactly 10 km east-west"), SE.X - NW.X, 10000.0);
        TestEqual(TEXT("Increasing local Y goes south"), SE.Y - NW.Y, -10000.0);
        if (Column < 9) TestEqual(TEXT("East edge joins next plot's west edge"),
            SE.X, FLordSimulatorRegion::LocalToWorldMetres(FIntPoint(Column + 1, Row), FVector::ZeroVector).X);
        if (Row < 9) TestEqual(TEXT("South edge joins next row's north edge"),
            SE.Y, FLordSimulatorRegion::LocalToWorldMetres(FIntPoint(Column, Row + 1), FVector::ZeroVector).Y);
    }
    TestEqual(TEXT("Exactly 100 separately named levels"), Levels.Num(), 100);
    TestEqual(TEXT("Northwest identity"), FLordSimulatorRegion::LevelPath(FIntPoint(0, 0)), FString(TEXT("/Game/Maps/SouthernCoastRange/tile_x0_y0")));
    TestEqual(TEXT("Southeast identity"), FLordSimulatorRegion::LevelPath(FIntPoint(9, 9)), FString(TEXT("/Game/Maps/SouthernCoastRange/tile_x9_y9")));
    TestEqual(TEXT("Northwest geographic position"), FLordSimulatorRegion::LocalToWorldMetres(FIntPoint(0, 0), FVector::ZeroVector), FVector(-50000, 50000, 0));
    TestEqual(TEXT("Southeast geographic position"), FLordSimulatorRegion::LocalToWorldMetres(FIntPoint(9, 9), FVector(1000000, 1000000, 0)), FVector(50000, -50000, 0));
    TestTrue(TEXT("Invalid cells have no load destination"), FLordSimulatorRegion::LevelPath(FIntPoint(10, 0)).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLordSimulatorRegionTransitionsTest, "LordSimulator.Region.Transitions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLordSimulatorRegionTransitionsTest::RunTest(const FString&)
{
    FLordSimulatorRegionSelectionState State;
    TestTrue(TEXT("Selection unavailable before opening screen"), State.Select(FIntPoint(1, 2)) == ELordSimulatorPlotRequest::Ignore);
    State.Open();
    TestTrue(TEXT("Initial Back closes to menu"), State.Dismiss());
    TestFalse(TEXT("Initial dismissal creates no plot"), FLordSimulatorRegion::IsValid(State.Current));
    State.Open();
    TestTrue(TEXT("Initial selection requests load"), State.Select(FIntPoint(1, 2)) == ELordSimulatorPlotRequest::Load);
    TestTrue(TEXT("Duplicate click ignored while loading"), State.Select(FIntPoint(3, 4)) == ELordSimulatorPlotRequest::Ignore);
    TestFalse(TEXT("Back cannot dismiss pending load"), State.Dismiss());
    TestTrue(TEXT("Screen stays visible while loading"), State.bOpen);
    State.Complete(false);
    TestTrue(TEXT("Missing initial landscape shows failure on selection screen"), State.bOpen && State.bLoadFailed);
    TestFalse(TEXT("Failure never accepts an empty plot"), FLordSimulatorRegion::IsValid(State.Current));
    State.Select(FIntPoint(1, 2));
    State.Complete(true);
    TestEqual(TEXT("Success commits only requested plot"), State.Current, FIntPoint(1, 2));
    TestFalse(TEXT("Successful load dismisses overview"), State.bOpen);
    State.Open();
    TestTrue(TEXT("In-plot Back dismisses overview"), State.Dismiss());
    TestEqual(TEXT("Dismissal preserves geographic identity"), State.Current, FIntPoint(1, 2));
    State.Open();
    TestTrue(TEXT("Current-cell selection closes without loading"), State.Select(State.Current) == ELordSimulatorPlotRequest::Close);
    TestFalse(TEXT("Current-cell selection does not start a load"), State.bLoading);
    State.Open();
    State.Select(FIntPoint(8, 7));
    State.Complete(false);
    TestEqual(TEXT("Failed switch retains previous plot identity"), State.Current, FIntPoint(1, 2));
    TestTrue(TEXT("Failed switch retains overview and failure text"), State.bOpen && State.bLoadFailed);
    State.Select(FIntPoint(8, 7));
    State.Complete(true);
    TestEqual(TEXT("Successful switch commits destination"), State.Current, FIntPoint(8, 7));
    return true;
}
