#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/HandleHolder.h>


class CLASS_DECL_ZTOOLSO ProcessRunner
{
public:
    HANDLE Start(std::wstring command_line);
    HANDLE Start(std::string_view command_line_sv) { return Start(TC::ToWide(command_line_sv)); }

    void Kill();

    DWORD GetExitCode() const;

    std::string ReadStdOut() { return ReadFromPipe(m_childStdOutRead); }
    std::string ReadStdErr() { return ReadFromPipe(m_childStdErrRead); }

private:
    std::string ReadFromPipe(HANDLE pipe);

private:
    HandleHolder m_processHandle;
    HandleHolder m_childStdOutRead;
    HandleHolder m_childStdErrRead;
};
