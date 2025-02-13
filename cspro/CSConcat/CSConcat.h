#pragma once


class CCSConcatApp : public CWinApp
{
public:
    CCSConcatApp();

protected:
    BOOL InitInstance() override;
    BOOL ProcessMessageFilter(int iCode, LPMSG lpMsg)  override;

private:
    HACCEL m_hAccelerators;
};
