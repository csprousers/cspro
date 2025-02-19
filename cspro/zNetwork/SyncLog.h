#pragma once

#include <zNetwork/zNetwork.h>

// avoiding macro pollution
#pragma push_macro("DEBUG")
#pragma push_macro("INFO")
#pragma push_macro("WARNING")
#pragma push_macro("ERROR")
#pragma push_macro("FATAL")
#pragma push_macro("TRACE")
#pragma push_macro("VERBOSE")

#include <external/easylogging/easylogging++.h>

#pragma pop_macro("DEBUG")
#pragma pop_macro("INFO")
#pragma pop_macro("WARNING")
#pragma pop_macro("ERROR")
#pragma pop_macro("FATAL")
#pragma pop_macro("TRACE")
#pragma pop_macro("VERBOSE")


class SyncLog : public el::LogDispatchCallback
{
public:
    ZNETWORK_API static std::string GetSyncLogPath();

    ZNETWORK_API static void EnableLogging();

protected:
    // el::LogDispatchCallback overrides
    void handle(const el::LogDispatchData* data) noexcept override;
};


#define SYNCLOG_ERROR   el::base::Writer(el::Level::Error, __FILE__, __LINE__, ELPP_FUNC, el::base::DispatchAction::NormalLog).construct(1, "sync")
#define SYNCLOG_INFO    el::base::Writer(el::Level::Info, __FILE__, __LINE__, ELPP_FUNC, el::base::DispatchAction::NormalLog).construct(1, "sync")
#define SYNCLOG_WARNING el::base::Writer(el::Level::Warning, __FILE__, __LINE__, ELPP_FUNC, el::base::DispatchAction::NormalLog).construct(1, "sync")
