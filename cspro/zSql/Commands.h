#pragma once


namespace Sqlite::Commands
{
    constexpr const char* BeginTransaction       = "BEGIN;";
    constexpr const char* EndTransaction         = "COMMIT;";

    constexpr const char* SetJournalModeToOff    = "PRAGMA journal_mode = OFF;";
    constexpr const char* SetJournalModeToDelete = "PRAGMA journal_mode = DELETE;";
    constexpr const char* SetSynchronousOff      = "PRAGMA synchronous = OFF;";
    constexpr const char* SetSynchronousFull     = "PRAGMA synchronous = FULL;";
}
