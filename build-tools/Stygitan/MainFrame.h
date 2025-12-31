#pragma once


class MainFrame : public CMDIFrameWnd
{
public:
    MainFrame();

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnClose();

    LRESULT OnOpenContainingFolder(WPARAM wParam, LPARAM lParam);

    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);
};
