#pragma once

#include "CoreMinimal.h"
#include "Logging/LordSimulatorLog.h"

/**
 * The complete Lord Simulator settings state.
 * Every setting has a default value, defined by its member initializer.
 */
struct FLordSimulatorSettingsData
{
	/** Persistent setting `LogMode`. Default: `Default`. */
	ELordSimulatorLogMode LogMode = ELordSimulatorLogMode::Default;

	bool operator==(const FLordSimulatorSettingsData& Other) const
	{
		return LogMode == Other.LogMode;
	}

	bool operator!=(const FLordSimulatorSettingsData& Other) const
	{
		return !(*this == Other);
	}
};

/**
 * Lord Simulator settings system.
 *
 * Persistent settings are stored in <Documents known folder>\Lord Simulator\settings.ini (INI format).
 * The file is always read and written as one complete settings state; individual settings are never
 * loaded from the file on their own. Runtime changes modify only the in-memory settings object.
 *
 * Settings failures never stop the application.
 */
class LORDSIMULATOR_API FLordSimulatorSettings
{
public:
	/**
	 * Loads the complete settings state from the settings file into the in-memory settings object.
	 * Creates the settings file with default values when it does not exist.
	 */
	static void Load();

	/**
	 * Compares the settings file with the in-memory settings object and writes the complete
	 * in-memory settings object to the settings file when they differ.
	 */
	static void Shutdown();

	/** Returns the current in-memory settings object. */
	static const FLordSimulatorSettingsData& Get();

	/** Changes `LogMode` in the in-memory settings object. */
	static void SetLogMode(ELordSimulatorLogMode LogMode);
};
