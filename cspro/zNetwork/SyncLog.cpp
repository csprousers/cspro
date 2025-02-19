#include "stdafx.h"
#include "SyncLog.h"
#include <zPlatformO/PlatformInterface.h>
#include <zToolsO/DebugLogging.h>
#include <zUtilO/Interapp.h>


// This needs to be done in one file to declare Easylogging++ globals
INITIALIZE_EASYLOGGINGPP


std::string SyncLog::GetSyncLogPath()
{
    auto get_directory = []() -> const std::string&
    {
#ifdef WIN_DESKTOP
        // the log file will go in the AppData folder on Windows
        return GetAppDataPath();
#else
        return PlatformInterface::GetInstance()->GetCSEntryDirectory();
#endif
    };

    return Path::Combine(get_directory(), "sync.log");
}


void SyncLog::EnableLogging()
{
    static const bool logging_enabled =
        []()
        {
            el::Configurations configurations;
            configurations.set(el::Level::Global, el::ConfigurationType::Enabled, "true");
            configurations.set(el::Level::Global, el::ConfigurationType::Format, "%datetime %level: %msg");
            configurations.set(el::Level::Global, el::ConfigurationType::ToFile, "true");
            configurations.set(el::Level::Global, el::ConfigurationType::Filename, GetSyncLogPath());
            configurations.set(el::Level::Global, el::ConfigurationType::ToStandardOutput, "false");
            configurations.set(el::Level::Global, el::ConfigurationType::LogFlushThreshold, "0");

            el::Loggers::reconfigureLogger("sync", configurations);

#ifdef _DEBUG
            el::Helpers::installLogDispatchCallback<SyncLog>("SyncLog");
            SyncLog* const dispatcher = el::Helpers::logDispatchCallback<SyncLog>("SyncLog");
            ASSERT(dispatcher != nullptr);
            dispatcher->setEnabled(true);
#endif
            return true;
    }();
}


void SyncLog::handle(const el::LogDispatchData* const data) noexcept
{
    const el::LogMessage* const log_message = data->logMessage();
    ASSERT(log_message != nullptr);

    CSLOG::LogPriority log_priority;

    switch( log_message->level() )
    {
        case el::Level::Trace:
        case el::Level::Debug:
            log_priority = CSLOG::LogPriority::Debug;
            break;

        case el::Level::Fatal:
            log_priority = CSLOG::LogPriority::Fatal;
            break;

        case el::Level::Error:
            log_priority = CSLOG::LogPriority::Error;
            break;

        case el::Level::Warning:
            log_priority = CSLOG::LogPriority::Warning;
            break;

        case el::Level::Verbose:
            log_priority = CSLOG::LogPriority::Verbose;
            break;

        case el::Level::Info:
        default:
            ASSERT(data->logMessage()->level() == el::Level::Info);
            log_priority = CSLOG::LogPriority::Info;
            break;
    }

    const CSLOG cslog(log_message->file().c_str(), log_message->line());
    cslog.LogMessage(log_priority, "Sync", log_message->message().c_str());
}
