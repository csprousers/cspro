#pragma once

#include <zToolsO/ProcessRunner.h>


class GenerateTaskProcessRunner
{
public:
    GenerateTaskProcessRunner(GenerateTask& generate_task, std::wstring process_name, const std::wstring& log_prefix, std::wstring (ProcessRunner::*output_read_function)());

    void SetOutputPreprocessor(std::function<void(std::wstring&)> output_preprocessor) { m_outputPreprocessor = std::move(output_preprocessor); }

    void Run(std::wstring command_line);

private:
    void AddOutputToLog();

private:
    ProcessRunner m_processRunner;
    GenerateTask& m_generateTask;
    const std::wstring m_processName;
    const std::wstring m_logPrefix;
    std::wstring (ProcessRunner::*const m_outputReadFunction)();
    bool m_addSpacingBeforeNextLoggedLine;
    std::function<void(std::wstring&)> m_outputPreprocessor;
};
