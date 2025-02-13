#pragma once


class CReFmtApp : public CWinApp
{
public:
    CReFmtApp();

protected:
    BOOL InitInstance() override;
    BOOL ProcessMessageFilter(int iCode, LPMSG lpMsg) override;

private:
    HACCEL m_hAccelerators;
};
