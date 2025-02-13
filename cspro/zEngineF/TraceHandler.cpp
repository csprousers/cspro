#include "StdAfx.h"
#include "TraceHandler.h"
#include <zUtilO/Specfile.h>

#ifdef WIN_DESKTOP
#include "WindowsTraceHandler.h"
#endif


TraceHandler::TraceHandler()
{
}


TraceHandler::~TraceHandler()
{
    if( m_file != nullptr )
    {
        OutputStartStopMessage(false);
        m_file->Close();
    }
}


std::unique_ptr<TraceHandler> TraceHandler::CreateTraceHandler()
{
#ifdef WIN_DESKTOP
    return std::make_unique<WindowsTraceHandler>();
#else
    return std::unique_ptr<TraceHandler>(new TraceHandler);
#endif
}


bool TraceHandler::TurnOnWindowTrace()
{
    return false;
}


bool TraceHandler::TurnOnFileTrace(const std::wstring& filename, bool append)
{
    try
    {
        // close any previously open file
        if( m_file != nullptr )
            m_file->Close();

        UINT open_flags = CFile::modeWrite | CFile::shareDenyWrite | CFile::modeCreate;

        if( append )
            open_flags |= CFile::modeNoTruncate;

        m_file = std::make_unique<CStdioFileUnicode>();

        if( m_file->Open(filename.c_str(), open_flags) )
        {
            m_file->SeekToEnd();
            OutputStartStopMessage(true);
            return true;
        }
    }
    catch(...) { }

    m_file.reset();

    return false;
}


void TraceHandler::OutputStartStopMessage(bool start)
{
    ASSERT(m_file != nullptr);

    Output(FormatTextCS2WS(_T("Trace %s at %s"),
                           start ? _T("started") : _T("stopped"),
                           FormatTimestamp<std::wstring>(GetTimestamp(), "%c").c_str()),
           OutputType::SystemText);
}


void TraceHandler::Output(const std::wstring& text, OutputType output_type)
{
    // when a new style is being printed we'll put an extra line break
    if( output_type != m_lastOutputType )
    {
        if( m_lastOutputType.has_value() )
            OutputLine(SO::EmptyString);

        m_lastOutputType = output_type;
    }

    if( output_type == OutputType::UserText )
    {
        constexpr wstring_view UserTextLinePrefix_sv = _T("TRACE   ");
        OutputLine(UserTextLinePrefix_sv, text);
    }

    else
    {
        OutputLine(text);
    }
}


void TraceHandler::OutputLine(const std::wstring& text)
{
    ASSERT(text.find_first_of(_T("\r\n")) == std::wstring::npos);

    if( m_file != nullptr )
        m_file->WriteLine(text);
}


void TraceHandler::OutputLine(std::wstring line_prefix, const std::wstring& text)
{
    // when newlines are used, write them to different lines
    if( SO::ContainsNewlineCharacter(text) )
    {
        bool first_line = true;

        SO::ForeachLine(text, true,
            [&](const std::wstring& line)
            {
                OutputLine(line_prefix + line);

                if( first_line )
                {
                    // replace the line prefix with spaces
                    line_prefix = SO::GetRepeatingCharacterString(' ', line_prefix.length());
                    first_line = false;
                }

                return true;
            });
    }

    else
    {
        OutputLine(line_prefix + text);
    }

}
