#pragma once

#include "Commandlets/Commandlet.h"
#include "LordSimulatorImportRegionCommandlet.generated.h"

/** Offline content preparation only; never linked into the packaged game. */
UCLASS()
class ULordSimulatorImportRegionCommandlet : public UCommandlet
{
    GENERATED_BODY()
public:
    ULordSimulatorImportRegionCommandlet();
    virtual int32 Main(const FString& Params) override;
};
