#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/TextFormatter.h>


// a class to facilitate tracing, and asserting, on all platforms

#ifdef _DEBUG

#define CSLOG_TRACE CSLOG(__FILE__, __LINE__)

#define CSLOG_ASSERT(expression) (void)( ( !!( expression ) ) || ( CSLOG::AssertFail(#expression, __FILE__, __LINE__), 0) )

#endif


class CSLOG
{
    friend class SyncLog;

public:
    CSLOG(const char* file_path, const int line_number);

    template<typename... Args>
    void operator()(const char* formatter, Args const&... args);

    template<typename... Args>
    void operator()(const wchar_t* formatter, Args const&... args);

    CLASS_DECL_ZTOOLSO static void AssertFail(const char* expression, const char* file_path, int line_number);

private:
    CLASS_DECL_ZTOOLSO void LogTraceMessage(const char* text) const;
    CLASS_DECL_ZTOOLSO void LogTraceMessage(const CString& text) const;

    // the values match the ANDROID_LOG_... values
    enum class LogPriority { Verbose = 2, Debug = 3, Info = 4, Warning = 5, Error = 6, Fatal = 7 };

    CLASS_DECL_ZTOOLSO void LogMessage(LogPriority log_priority, const char* tag, const char* text) const;

private:
    const char* const m_filePath;
	const int m_lineNumber;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CSLOG::CSLOG(const char* const file_path, const int line_number)
    :   m_filePath(file_path),
        m_lineNumber(line_number)
{
}


template<typename... Args>
void CSLOG::operator()(const char* const formatter, Args const&... args)
{
    if constexpr(sizeof...(Args) == 0)
    {
        LogTraceMessage(formatter);
    }

    else
    {
        LogTraceMessage(FormatText(formatter, args...).c_str());
    }
}


template<typename... Args>
void CSLOG::operator()(const wchar_t* formatter, Args const&... args)
{
#ifdef WIN_DESKTOP
    ATL::CTraceFileAndLineInfo(m_filePath, m_lineNumber)(formatter, args...);
#else
    LogTraceMessage(FormatText(formatter, args...));
#endif
}
