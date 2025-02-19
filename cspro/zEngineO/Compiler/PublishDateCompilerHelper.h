#pragma once

#include <zEngineO/Compiler/CompilerHelper.h>


class PublishDateCompilerHelper : public CompilerHelper
{
public:
    PublishDateCompilerHelper(LogicCompiler& logic_compiler);

    int64_t GetPublishDate() const { return m_publishDate; }

protected:
    bool IsCacheable() const override { return false; }

private:
    const int64_t m_publishDate;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline PublishDateCompilerHelper::PublishDateCompilerHelper(LogicCompiler& logic_compiler)
    :   CompilerHelper(logic_compiler),
        m_publishDate(DateTime::TimeToYYYYMMDDHHMMSS(DateTime::Now(), true))
{
    // the publish date is represented using the local timezone
}
