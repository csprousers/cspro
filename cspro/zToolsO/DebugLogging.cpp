#include "StdAfx.h"
#include "DebugLogging.h"
#include "PortableFunctions.h"


namespace
{
    constexpr const char* DefaultTag = "CSLOG";

    constexpr bool FormatTraceMessage = !OnWindowsDesktop();
}


void CSLOG::AssertFail(const char* const expression, const char* const file_path, const int line_number)
{
    const std::string message = FormatText("ASSERT failed: %s", expression);

    CSLOG(file_path, line_number).LogMessage(LogPriority::Error, DefaultTag, message.c_str());
}


void CSLOG::LogTraceMessage(const char* const text) const
{
    constexpr LogPriority LogPriorityForTrace = LogPriority::Info;

    if constexpr(FormatTraceMessage)
    {
        const std::string message = FormatText("%s(%d): %s", PortableFunctions::PathGetFilename(m_filePath).c_str(), m_lineNumber, text);
        LogMessage(LogPriorityForTrace, DefaultTag, message.c_str());
    }

    else
    {
        LogMessage(LogPriorityForTrace, DefaultTag, text);
    }    
}


void CSLOG::LogTraceMessage(const CString& text) const
{
    LogTraceMessage(TC::ToUtf8(text).c_str());
}


#if defined(WIN_DESKTOP)

void CSLOG::LogMessage(LogPriority /*log_priority*/, const char* const tag, const char* const text) const
{
#ifdef _DEBUG
    const ATL::CTraceFileAndLineInfo tracer(m_filePath, m_lineNumber);

    if( tag == DefaultTag )
    {
        tracer(TC::ToWide(text).c_str());
    }

    else
    {
        ASSERT(strcmp(tag, "Sync") == 0);
        tracer(L"%s", TC::ToWide(FormatText("%s: %s\n", tag, text)).c_str());
    }
#endif
}


#elif defined(ANDROID)

#include <android/log.h>

void CSLOG::LogMessage(const LogPriority log_priority, const char* const tag, const char* const text) const
{
    static_assert(static_cast<int>(LogPriority::Verbose) == ANDROID_LOG_VERBOSE);
    static_assert(static_cast<int>(LogPriority::Debug) == ANDROID_LOG_DEBUG);
    static_assert(static_cast<int>(LogPriority::Info) == ANDROID_LOG_INFO);
    static_assert(static_cast<int>(LogPriority::Warning) == ANDROID_LOG_WARN);
    static_assert(static_cast<int>(LogPriority::Error) == ANDROID_LOG_ERROR);
    static_assert(static_cast<int>(LogPriority::Fatal) == ANDROID_LOG_FATAL);

    __android_log_print(static_cast<int>(log_priority), tag, "%s", text);
}


#elif defined(WASM)

#include <emscripten/console.h>

void CSLOG::LogMessage(const LogPriority log_priority, const char* /*tag*/, const char* const text) const
{
    void (*console_function)(const char*);

    switch( log_priority )
    {
        case LogPriority::Verbose:
        case LogPriority::Debug:
        case LogPriority::Info:
            console_function = &emscripten_console_log;
            break;

        case LogPriority::Warning:
            console_function = &emscripten_console_warn;
            break;

        case LogPriority::Error:
        case LogPriority::Fatal:
            console_function = &emscripten_console_error;
            break;
    }

    (*console_function)(text);
}


#elif defined(_CONSOLE)

#include <iostream>

void CSLOG::LogMessage(LogPriority /*log_priority*/, const char* /*tag*/, const char* const text) const
{
    std::wcout << TC::ToWide(text).c_str() << std::endl;
}


#endif
