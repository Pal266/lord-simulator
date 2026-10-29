#pragma once

#include "CoreMinimal.h"

/** Lord Simulator project logging modes. */
enum class ELordSimulatorLogMode : uint8
{
	/** Writes entries designated for Default. */
	Default,
	/** Writes entries designated for Default plus Development-only entries. */
	Development
};

/**
 * Lord Simulator project logging framework.
 *
 * Writes Lord Simulator log entries only (no general Unreal Engine log output) to
 * <Documents known folder>\Lord Simulator\log.log, UTF-8 encoded, one session per file.
 * Every entry has the format: [YYYY-MM-DD HH:MM:SS] Message (local system time).
 *
 * Logging failures never stop the application: every failure is silently ignored.
 */
class LORDSIMULATOR_API FLordSimulatorLog
{
public:
	/** Ensures the log directory exists, starts a new log file for this session and writes the startup entry. */
	static void Initialize();

	/** Closes the log file. */
	static void Shutdown();

	/** Returns the active logging mode. */
	static ELordSimulatorLogMode GetMode();

	/** Returns true when Development-only entries are written. */
	static bool IsDevelopmentMode();

	/** Writes an entry designated for Default mode (written in both Default and Development modes). */
	static void LogDefault(const FString& Message);

	/** Writes an entry designated for Development mode only (ignored in Default mode). */
	static void LogDevelopment(const FString& Message);
};

/** Writes a log entry designated for Default mode. */
#define LS_LOG_DEFAULT(Message) FLordSimulatorLog::LogDefault(Message)

/**
 * Writes a Development-only log entry.
 * The message expression is evaluated only when Development mode is active,
 * so performance-heavy diagnostics cost nothing in Default mode.
 */
#define LS_LOG_DEVELOPMENT(Message) \
	do \
	{ \
		if (FLordSimulatorLog::IsDevelopmentMode()) \
		{ \
			FLordSimulatorLog::LogDevelopment(Message); \
		} \
	} while (0)
