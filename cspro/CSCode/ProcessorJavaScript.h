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

    // Compile will call with bytecode as null.
    // Run will provide space for the compiled bytecode.
    bool CompileRunWorker(JavaScript::Bytecode* bytecode);

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

    void OnPrint(SharableString text) override;

private:
    OutputWnd& m_outputWnd;
};
