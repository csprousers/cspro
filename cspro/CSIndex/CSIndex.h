#pragma once


class CCSIndexApp : public CWinApp
{
public:
    CCSIndexApp();

protected:
    BOOL InitInstance() override;
    BOOL ProcessMessageFilter(int iCode, LPMSG lpMsg) override;

private:
    HACCEL m_hAccelerators;
};
