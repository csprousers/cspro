#pragma once

#include <zJavaScript/Executor.h>

class CodeDoc;
class OutputWnd;


// --------------------------------------------------------------------------
// ProcessorJavaScript
// --------------------------------------------------------------------------

class ProcessorJavaScript
{
public:
    ProcessorJavaScript(CodeDoc& code_doc);

    void Compile();
    void Run();

private:
    JavaScript::ModuleType GetModuleType();

    // Compile will call with byte_code as null; Run will provide space for the compiled byte code
    bool CompileRunWorker(JavaScript::ByteCode* byte_code);

private:
    CodeDoc& m_codeDoc;
    JavaScript::Executor m_executor;
};


// --------------------------------------------------------------------------
// OutputWndJavaScriptPrinter
// --------------------------------------------------------------------------

class OutputWndJavaScriptPrinter : public JavaScript::Printer
{
public:
    OutputWndJavaScriptPrinter(OutputWnd& output_wnd);

    void OnPrint(const std::string& text) override;

private:
    OutputWnd& m_outputWnd;
};
