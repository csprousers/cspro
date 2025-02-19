#pragma once


class ParadataConcatApp : public CWinApp
{
public:
    ParadataConcatApp();

protected:
    BOOL InitInstance() override;
    BOOL ProcessMessageFilter(int iCode, LPMSG lpMsg) override;

private:
    void RunProgram();

private:
    HACCEL m_hAccelerators;
};
