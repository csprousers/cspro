#pragma once

#include <zUtilF/ApplicationShutdownRunner.h>
#include <zUtilF/MFCMenuBarWithoutSerializableState.h>

class HtmlViewCtrl;
class Runtime;
class WindowsRuntimeHost;


class CMainFrame : public CFrameWnd
{
    DECLARE_DYNCREATE(CMainFrame)

public:
    CMainFrame();
    ~CMainFrame();

public:
    // called by RuntimeView from OnInitialUpdate to start the first runtime
    void Initialize(WindowsRuntimeHost& runtime_host, const std::string& path_from_command_line);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreCreateWindow(CREATESTRUCT& cs) override;
    int OnCreate(LPCREATESTRUCT lpCreateStruct);

    void OnClose();

    void OnFileOpenApplication();
    void OnFileOpenDirectory();
    void OnFileCloseRuntime();

    LRESULT OnCloseRuntime(WPARAM wParam, LPARAM lParam);

    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);

    LRESULT OnGetApplicationShutdownRunner(WPARAM wParam, LPARAM lParam);

private:
    MFCMenuBarWithoutSerializableState m_wndMenuBar;
    WindowsRuntimeHost* m_runtimeHost;

    ApplicationShutdownRunner m_applicationShutdownRunner;
};
