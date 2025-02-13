#pragma once


class CParadataConcatApp : public CWinApp
{
public:
    CParadataConcatApp();

protected:
    BOOL InitInstance() override;
    BOOL ProcessMessageFilter(int iCode, LPMSG lpMsg) override;

private:
    void RunProgram();

private:
    HACCEL m_hAccelerators;
};


extern CParadataConcatApp theApp;
