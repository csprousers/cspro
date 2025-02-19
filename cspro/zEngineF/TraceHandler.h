#pragma once

#include <zEngineF/zEngineF.h>

namespace FileIO { class TextFile; }


// the base trace handler class implements the file trace handler (for all environmnets)
// and a subclass will implement a version for Windows that can use a window

class CLASS_DECL_ZENGINEF TraceHandler
{
public:
    enum class OutputType { LogicText, SystemText, UserText };

protected:
    TraceHandler();

public:
    virtual ~TraceHandler();

    static std::unique_ptr<TraceHandler> CreateTraceHandler();

    virtual bool TurnOnWindowTrace();

    bool TurnOnFileTrace(const std::string& file_path, bool append);

    void Output(SharableString text, OutputType output_type);

protected:
    virtual void OutputLine(SharableString text);
    void OutputLine(std::string_view line_prefix_sv, std::string_view text_sv);

private:
    void OutputStartStopMessage(bool start);

private:
    std::unique_ptr<FileIO::TextFile> m_file;
    std::optional<OutputType> m_lastOutputType;
};
