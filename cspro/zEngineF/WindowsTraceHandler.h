#pragma once

#include <zEngineF/TraceHandler.h>
#include <zUtilF/LoggingListBox.h>

class TraceWnd;


// --------------------------------------------------------------------------
// WindowsTraceHandler
// --------------------------------------------------------------------------

class WindowsTraceHandler : public TraceHandler 
{
public:
    WindowsTraceHandler();
    ~WindowsTraceHandler();

    bool TurnOnWindowTrace() override;

    void DeleteTraceWindow();

protected:
    void OutputLine(const std::wstring& text) override;

private:
    TraceWnd* m_traceWnd;
};



// --------------------------------------------------------------------------
// TraceLoggingListBox
// --------------------------------------------------------------------------

class TraceLoggingListBox : public LoggingListBox
{
public:
    TraceLoggingListBox();

    bool GetAlwaysOnTop() const { return m_alwaysOnTop; }

protected:
    void AddAdditionalContextMenuItems(CMenu& popup_menu) override;

protected:
    DECLARE_MESSAGE_MAP()

    void OnAlwaysOnTop();
    void OnUpdateAlwaysOnTop(CCmdUI* pCmdUI);

private:
    bool m_alwaysOnTop;
};



// --------------------------------------------------------------------------
// TraceWnd
// --------------------------------------------------------------------------

class TraceWnd : public CFrameWnd
{
public:
    TraceWnd(WindowsTraceHandler& trace_handler);

    bool CreateTraceControl();

    void AddText(std::wstring text);

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnSize(UINT nType, int cx, int cy);
    void OnNcDestroy();

private:
    WindowsTraceHandler& m_traceHandler;
    TraceLoggingListBox m_traceLoggingListBox;
};
