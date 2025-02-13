#pragma once

#include <zCaseO/StringBasedCaseConstructionReporter.h>
#include <zToolsO/Utf8Convert.h>
#include <easyloggingwrapper.h>


class SyncLogCaseConstructionReporter : public StringBasedCaseConstructionReporter
{
protected:
    void WriteString(NullTerminatedString key, NullTerminatedString message) override
    {
        CLOG(ERROR, "sync") << "Error constructing case:";
        CLOG(ERROR, "sync") << "*** [" << UTF8Convert::WideToUTF8(key) << "]";
        CLOG(ERROR, "sync") << "*** " << UTF8Convert::WideToUTF8(message);
    }
};
