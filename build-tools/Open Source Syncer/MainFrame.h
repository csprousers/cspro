#pragma once


class MainFrame : public CMDIFrameWnd
{
public:
    MainFrame();

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnClose();

    LRESULT OnUpdateStatusBar(WPARAM wParam, LPARAM lParam);

    LRESULT OnOperationInitialize(WPARAM wParam, LPARAM lParam);
    LRESULT OnOperationComplete(WPARAM wParam, LPARAM lParam);

    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);

private:
    Controller& m_controller;

    CStatusBar m_wndStatusBar;
};
