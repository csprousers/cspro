#pragma once

#include <zToolsO/ProcessRunner.h>


class GenerateTaskProcessRunner
{
public:
    GenerateTaskProcessRunner(GenerateTask& generate_task, std::string process_name, const std::string& log_prefix, std::string (ProcessRunner::*output_read_function)());

    void SetOutputPreprocessor(std::function<void(std::string&)> output_preprocessor) { m_outputPreprocessor = std::move(output_preprocessor); }

    void Run(const std::string& command_line);

private:
    void AddOutputToLog();

private:
    ProcessRunner m_processRunner;
    GenerateTask& m_generateTask;
    std::string m_processName;
    std::string m_logPrefix;
    std::string (ProcessRunner::*const m_outputReadFunction)();
    bool m_addSpacingBeforeNextLoggedLine;
    std::function<void(std::string&)> m_outputPreprocessor;
};
