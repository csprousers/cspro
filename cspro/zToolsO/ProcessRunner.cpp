#include "StdAfx.h"
#include "ProcessRunner.h"


HANDLE ProcessRunner::Start(std::wstring command_line)
{
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HandleHolder child_stdout_write;
    HandleHolder child_stdout_read;
    HandleHolder child_stderr_read;
    HandleHolder child_stderr_write;

    if( !CreatePipe(&child_stdout_read, &child_stdout_write, &saAttr, 0) )
        return nullptr;

    // Don't inherit read handle
    if( !SetHandleInformation(child_stdout_read, HANDLE_FLAG_INHERIT, 0) )
        return nullptr;

    if( !CreatePipe(&child_stderr_read, &child_stderr_write, &saAttr, 0) )
        return nullptr;

    // Don't inherit read handle
    if( !SetHandleInformation(child_stderr_read, HANDLE_FLAG_INHERIT, 0) )
        return nullptr;

    PROCESS_INFORMATION piProcInfo = { };

    STARTUPINFO siStartInfo = { };
    siStartInfo.cb = sizeof(STARTUPINFO);
    siStartInfo.hStdError = child_stderr_write;
    siStartInfo.hStdOutput = child_stdout_write;
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    BOOL success = CreateProcess(nullptr,
                                 command_line.data(), // command line 
                                 nullptr,             // process security attributes 
                                 nullptr,             // primary thread security attributes 
                                 TRUE,                // handles are inherited 
                                 CREATE_NO_WINDOW,    // creation flags 
                                 nullptr,             // use parent's environment 
                                 nullptr,             // use parent's current directory 
                                 &siStartInfo,        // STARTUPINFO pointer 
                                 &piProcInfo);        // receives PROCESS_INFORMATION 

    if( !success )
        return nullptr;

    CloseHandle(piProcInfo.hThread);

    m_childStdOutRead = std::move(child_stdout_read);
    m_childStdErrRead = std::move(child_stderr_read);

    m_processHandle = HandleHolder(piProcInfo.hProcess);

    return m_processHandle;
}


void ProcessRunner::Kill()
{
    TerminateProcess(m_processHandle, 1);
}


DWORD ProcessRunner::GetExitCode() const
{
    DWORD exit_code;
    GetExitCodeProcess(m_processHandle, &exit_code);
    return exit_code;
}


std::wstring ProcessRunner::ReadFromPipe(HANDLE pipe)
{
    constexpr size_t BufferIncrementSize = 2048;
    std::vector<char> buffer(BufferIncrementSize);
    size_t current_buffer_index = 0;
    DWORD bytes_read;

    while( ReadFile(pipe, buffer.data() + current_buffer_index, buffer.size() - current_buffer_index, &bytes_read, nullptr) && bytes_read != 0 )
    {
        current_buffer_index += bytes_read;
        buffer.resize(buffer.size() + BufferIncrementSize);
    }

    return UTF8Convert::UTF8ToWide(buffer.data(), current_buffer_index);
}
