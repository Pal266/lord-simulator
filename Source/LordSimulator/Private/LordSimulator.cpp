#include "LordSimulator.h"
#include "Logging/LordSimulatorLog.h"
#include "Modules/ModuleManager.h"

class FLordSimulatorModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Logging is initialized first so it is available to all other Lord Simulator systems.
		FLordSimulatorLog::Initialize();
	}

	virtual void ShutdownModule() override
	{
		LS_LOG_DEFAULT(TEXT("Application shutdown started."));
		FLordSimulatorLog::Shutdown();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLordSimulatorModule, LordSimulator, "LordSimulator");
