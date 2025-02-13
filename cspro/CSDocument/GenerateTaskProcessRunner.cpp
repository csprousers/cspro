#include "StdAfx.h"
#include "GenerateTaskProcessRunner.h"


GenerateTaskProcessRunner::GenerateTaskProcessRunner(GenerateTask& generate_task, std::wstring process_name, const std::wstring& log_prefix, std::wstring (ProcessRunner::*output_read_function)())
    :   m_generateTask(generate_task),
        m_processName(std::move(process_name)),
        m_logPrefix(log_prefix + _T(": ")),
        m_outputReadFunction(output_read_function),
        m_addSpacingBeforeNextLoggedLine(true)
{
}


void GenerateTaskProcessRunner::Run(std::wstring command_line)
{
    HANDLE process_handle = m_processRunner.Start(std::move(command_line));

    if( process_handle == nullptr )
        throw CSProException(_T("There was a problem running %s."), m_processName.c_str());

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
    std::wstring output = (m_processRunner.*m_outputReadFunction)();

    if( output.empty() )
        return;

    if( m_outputPreprocessor )
        m_outputPreprocessor(output);

    SO::ForeachLine(output, true,
        [&](const std::wstring& line)
        {
            if( m_addSpacingBeforeNextLoggedLine )
            {
                m_generateTask.GetInterface().LogText(std::wstring());
                m_addSpacingBeforeNextLoggedLine = false;
            }

            m_generateTask.GetInterface().LogText(m_logPrefix + line);
            return true;
        });
}
