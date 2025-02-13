#pragma once

#include <zPackO/PackSpec.h>


class CCSPackApp : public CWinApp
{
public:
    CCSPackApp();

protected:
    BOOL InitInstance() override;
    BOOL ProcessMessageFilter(int iCode, LPMSG lpMsg) override;

private:
    std::tuple<std::unique_ptr<PackSpec>, std::wstring> ProcessCommandLine(const std::wstring& filename, bool pack_flag);

private:
    HACCEL m_hAccelerators;
};
