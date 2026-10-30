#include "LordSimulator.h"
#include "Logging/LordSimulatorLog.h"
#include "Modules/ModuleManager.h"
#include "Settings/LordSimulatorSettings.h"

class FLordSimulatorModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Logging is initialized first, in Default mode, so it is available to all other Lord Simulator systems.
		FLordSimulatorLog::Initialize();

		LS_LOG_DEFAULT(TEXT("Settings loading started."));
		FLordSimulatorSettings::Load();
		FLordSimulatorLog::SetMode(FLordSimulatorSettings::Get().LogMode);
		LS_LOG_DEFAULT(TEXT("Settings loading completed."));
	}

	virtual void ShutdownModule() override
	{
		LS_LOG_DEFAULT(TEXT("Application shutdown started."));

		// Settings persistence completes while the logging framework is still active.
		FLordSimulatorSettings::Shutdown();
		FLordSimulatorLog::Shutdown();
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLordSimulatorModule, LordSimulator, "LordSimulator");
