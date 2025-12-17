#pragma once


class MainFrame : public CMDIFrameWnd
{
public:
    MainFrame();

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnActivateApp(BOOL bActive, DWORD dwThreadID);
};
