#pragma once

#include "CoreMinimal.h"
namespace LordSimulatorLocalization { class FState; }

struct FLordSimulatorLanguage
{
	FString Culture;
	FString NativeDisplayName;
};

/** Application-wide policy/state. Text resolution remains entirely owned by Unreal. */
class LORDSIMULATOR_API FLordSimulatorLocalization
{
public:
	static bool Initialize();
	static const TArray<FLordSimulatorLanguage>& GetSupportedLanguages();
	static bool IsSupported(const FString& Culture);
	static bool IsValidSetting(const FString& Culture);
	static const FString& GetConfiguredLanguage();
	static const TOptional<FString>& GetActiveLanguage();
	static bool ChangeLanguage(const FString& Culture);
	static bool IsStartupComplete();

	/** Startup orchestration only: consume the complete, validated settings state. */
	static void ApplyStartupLanguage();
	static void CompleteStartup();
	/** Restore game resources after an editor lifecycle override; never reinitialize or change settings. */
	static void ReapplyActiveLanguage();
private:
	static LordSimulatorLocalization::FState& State();
};
