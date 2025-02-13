#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/HandleHolder.h>


class CLASS_DECL_ZTOOLSO ProcessRunner
{
public:
    HANDLE Start(std::wstring command_line);

    void Kill();

    DWORD GetExitCode() const;

    std::wstring ReadStdOut() { return ReadFromPipe(m_childStdOutRead); }
    std::wstring ReadStdErr() { return ReadFromPipe(m_childStdErrRead); }

private:
    std::wstring ReadFromPipe(HANDLE pipe);

private:
    HandleHolder m_processHandle;
    HandleHolder m_childStdOutRead;
    HandleHolder m_childStdErrRead;
};
