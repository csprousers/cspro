#pragma once

#include <zEngineO/Compiler/CompilerHelper.h>


class PublishDateCompilerHelper : public CompilerHelper
{
public:
    PublishDateCompilerHelper(LogicCompiler& logic_compiler);

    double GetPublishDate() const { return m_publishDate; }

protected:
    bool IsCacheable() const override { return false; }

private:
    double m_publishDate;
};



inline PublishDateCompilerHelper::PublishDateCompilerHelper(LogicCompiler& logic_compiler)
    :   CompilerHelper(logic_compiler)
{
    struct tm tp;
    time_t t;

    t = time(&t);

#pragma warning(push)
#pragma warning(disable:4996)
    memcpy(reinterpret_cast<void*>(&tp), reinterpret_cast<const void*>(localtime(&t)), sizeof(tp));
#pragma warning(pop)

    m_publishDate = ( ( tp.tm_year + 1900 ) * 100 + ( tp.tm_mon + 1 ) ) * 100 + tp.tm_mday;
    m_publishDate *= 1000000;
    m_publishDate += ( tp.tm_hour * 100 + tp.tm_min ) * 100 + tp.tm_sec;
}
