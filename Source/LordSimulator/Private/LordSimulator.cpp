#include "LordSimulator.h"
#include "Logging/LordSimulatorLog.h"
#include "Modules/ModuleManager.h"
#include "Settings/LordSimulatorSettings.h"
#include "Localization/LordSimulatorLocalization.h"

#if WITH_EDITOR
#include "Editor.h"
#include "Engine/World.h"
#endif

class FLordSimulatorModule : public FDefaultGameModuleImpl
{
public:
	virtual void StartupModule() override
	{
		// Logging is initialized first, in Default mode, so it is available to all other Lord Simulator systems.
		FLordSimulatorLog::Initialize();
		FLordSimulatorLocalization::Initialize();

		LS_LOG_DEFAULT(TEXT("Settings loading started."));
		FLordSimulatorSettings::Load();
		FLordSimulatorLog::SetMode(FLordSimulatorSettings::Get().LogMode);
		FLordSimulatorLocalization::ApplyStartupLanguage();
		LS_LOG_DEFAULT(TEXT("Settings loading completed."));
		FLordSimulatorLocalization::CompleteStartup();
#if WITH_EDITOR
		if (GIsEditor && !IsRunningCommandlet())
		{
			// BeginPIE delegates have no ordering guarantee. World initialization is after
			// the engine's BeginPIE preview override and before actors can begin play.
			WorldInitializationHandle = FWorldDelegates::OnPreWorldInitialization.AddLambda([](UWorld* World, const UWorld::InitializationValues)
			{
				if (World->WorldType == EWorldType::PIE) FLordSimulatorLocalization::ReapplyActiveLanguage();
			});
			// ShutdownPIE runs after all EndPIE callbacks, including Unreal's preview reset.
			ShutdownPIEHandle = FEditorDelegates::ShutdownPIE.AddLambda([](bool)
			{
				FLordSimulatorLocalization::ReapplyActiveLanguage();
			});
		}
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_EDITOR
		FWorldDelegates::OnPreWorldInitialization.Remove(WorldInitializationHandle);
		FEditorDelegates::ShutdownPIE.Remove(ShutdownPIEHandle);
#endif
		LS_LOG_DEFAULT(TEXT("Application shutdown started."));

		// Settings persistence completes while the logging framework is still active.
		FLordSimulatorSettings::Shutdown();
		FLordSimulatorLog::Shutdown();
	}
#if WITH_EDITOR
private:
	FDelegateHandle WorldInitializationHandle;
	FDelegateHandle ShutdownPIEHandle;
#endif
};

IMPLEMENT_PRIMARY_GAME_MODULE(FLordSimulatorModule, LordSimulator, "LordSimulator");
