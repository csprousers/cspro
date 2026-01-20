#include "StdAfx.h"
#include "TraceHandler.h"
#include <zToolsO/File.h>

#ifdef WIN_DESKTOP
#include "WindowsTraceHandler.h"
#endif


TraceHandler::TraceHandler()
{
}


TraceHandler::~TraceHandler()
{
    if( m_file != nullptr )
        OutputStartStopMessage(false);
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


bool TraceHandler::TurnOnFileTrace(const std::string& file_path, const bool append)
{
    // close any previously open file
    m_file.reset();

    try
    {
        auto file = std::make_unique<FileIO::TextFile>();
        file->OpenForTextWriting(file_path, append);
        m_file = std::move(file);

        OutputStartStopMessage(true);

        return true;
    }

    catch(...)
    {
        return false;
    }
}


void TraceHandler::OutputStartStopMessage(const bool start)
{
    ASSERT(m_file != nullptr);

    Output(FormatText("Trace %s at %s",
                      start ? "started" : "stopped",
                      FormatTimestamp(GetTimestamp<double>()).c_str()),
           OutputType::SystemText);
}


void TraceHandler::Output(SharableString text, const OutputType output_type)
{
    // when a new style is being printed we'll put an extra line break
    if( output_type != m_lastOutputType )
    {
        if( m_lastOutputType.has_value() )
            OutputLine(SharableString::CreateBlankString());

        m_lastOutputType = output_type;
    }

    if( output_type == OutputType::UserText )
    {
        constexpr std::string_view UserTextLinePrefix_sv = "TRACE   ";
        OutputLine(UserTextLinePrefix_sv, *text);
    }

    else
    {
        OutputLine(std::move(text));
    }
}


void TraceHandler::OutputLine(const SharableString text)
{
    ASSERT(text->find_first_of("\r\n") == std::string::npos);

    if( m_file != nullptr )
        m_file->WriteLine(*text);
}


void TraceHandler::OutputLine(std::string_view line_prefix_sv, const std::string_view text_sv)
{
    // when newlines are used, write them to different lines
    if( SO::ContainsNewlineCharacter(text_sv) )
    {
        bool first_line = true;

        SO::ForeachLine(text_sv, true,
            [&](const std::string_view line_sv)
            {
                OutputLine(SO::Concatenate(line_prefix_sv, line_sv));

                if( first_line )
                {
                    // replace the line prefix with spaces
                    line_prefix_sv = SO::GetRepeatingCharacterString(' ', SO::WideLength(line_prefix_sv));
                    first_line = false;
                }
            });
    }

    else
    {
        OutputLine(SO::Concatenate(line_prefix_sv, text_sv));
    }
}
