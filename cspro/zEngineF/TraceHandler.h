#pragma once

#include <zEngineF/zEngineF.h>

class CStdioFileUnicode;


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

    bool TurnOnFileTrace(const std::wstring& filename, bool append);

    void Output(const std::wstring& text, OutputType output_type);

protected:
    virtual void OutputLine(const std::wstring& text);
    void OutputLine(std::wstring line_prefix, const std::wstring& text);

private:
    void OutputStartStopMessage(bool start);

private:
    std::unique_ptr<CStdioFileUnicode> m_file;
    std::optional<OutputType> m_lastOutputType;
};
