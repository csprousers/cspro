#pragma once

namespace ActionInvoker { class JsonExecutor; }


class ProcessorActionInvoker
{
public:
    static void ValidateJson(CodeView& code_view);

    static void Run(CodeDoc& code_doc);

private:
    static bool ValidateJson(CodeView& code_view, ActionInvoker::JsonExecutor* json_executor);
};
