#pragma once


// --------------------------------------------------------------------------
// ControllerThreadRunningFrame
//
// This is a simple CMDIChildWnd subclass that prevents a window from closing
// while a thread launched by it is in progress.
// --------------------------------------------------------------------------

class ControllerThreadRunningFrame : public CMDIChildWnd
{
    DECLARE_DYNCREATE(ControllerThreadRunningFrame)

protected:
    ControllerThreadRunningFrame();

protected:
    DECLARE_MESSAGE_MAP()

    void OnClose();

private:
    Controller& m_controller;
};
