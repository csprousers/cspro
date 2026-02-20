#pragma once


// --------------------------------------------------------------------------
// LogFrame
// --------------------------------------------------------------------------

class LogFrame : public CMDIChildWnd
{
    DECLARE_DYNCREATE(LogFrame)

public:
    void ActivateFrame(int nCmdShow = -1) override;

private:
    bool m_firstActivation = true;
};


// --------------------------------------------------------------------------
// LogView
// --------------------------------------------------------------------------

class LogView : public CFormView
{
    DECLARE_DYNCREATE(LogView)

protected:
    LogView();

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnDestroy();

    void DoDataExchange(CDataExchange* pDX) override;

private:
    Controller& m_controller;

    LoggingListBox m_loggingListBox;
};
