#pragma once


class MainFrame : public CMDIFrameWnd
{
public:
    MainFrame();

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);

    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);
};
