#include "StdAfx.h"
#include "GenerateTaskProcessRunner.h"


GenerateTaskProcessRunner::GenerateTaskProcessRunner(GenerateTask& generate_task, std::string process_name, const std::string& log_prefix,
                                                     std::string (ProcessRunner::*output_read_function)())
    :   m_generateTask(generate_task),
        m_processName(std::move(process_name)),
        m_logPrefix(log_prefix + ": "),
        m_outputReadFunction(output_read_function),
        m_addSpacingBeforeNextLoggedLine(true)
{
}


void GenerateTaskProcessRunner::Run(const std::string& command_line)
{
    HANDLE process_handle = m_processRunner.Start(command_line);

    if( process_handle == nullptr )
        throw CSProException("There was a problem running %s.", m_processName.c_str());

    constexpr DWORD UpdateCheckMilliseconds = 50;

    while( WaitForSingleObject(process_handle, UpdateCheckMilliseconds) == WAIT_TIMEOUT )
    {
        if( m_generateTask.IsCanceled() )
        {
            m_processRunner.Kill();
            return;
        }

        AddOutputToLog();
    }

    AddOutputToLog();
}


void GenerateTaskProcessRunner::AddOutputToLog()
{
    std::string output = (m_processRunner.*m_outputReadFunction)();

    if( output.empty() )
        return;

    if( m_outputPreprocessor )
        m_outputPreprocessor(output);

    SO::ForeachLine(output, true,
        [&](const std::string_view line_sv)
        {
            if( m_addSpacingBeforeNextLoggedLine )
            {
                m_generateTask.GetInterface().LogText(SharableString::CreateBlankString());
                m_addSpacingBeforeNextLoggedLine = false;
            }

            m_generateTask.GetInterface().LogText(SO::Concatenate(m_logPrefix, line_sv));
        });
}
