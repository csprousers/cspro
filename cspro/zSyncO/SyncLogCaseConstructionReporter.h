#pragma once

#include <zCaseO/StringBasedCaseConstructionReporter.h>
#include <zNetwork/SyncLog.h>


class SyncLogCaseConstructionReporter : public StringBasedCaseConstructionReporter
{
protected:
    void WriteString(const std::string& key, const std::string message) override
    {
        SYNCLOG_ERROR << "Error constructing case:";
        SYNCLOG_ERROR << "*** [" << key << "]";
        SYNCLOG_ERROR << "*** " << message;
    }
};
