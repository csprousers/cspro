#pragma once

#include <DataManager/Settings.h>
#include <zUtilO/ConnectionStringFileSimulator.h>
#include <zUtilF/ApplicationShutdownRunner.h>
#include <zUtilF/MFCMenuBarWithoutSerializableState.h>

class ObjectTransporter;
class SharedHtmlLocalFileServer;
class TemporaryFile;


class CMainFrame : public CMDIFrameWndEx
{
public:
    CMainFrame(std::shared_ptr<ConnectionStringFileSimulator> connection_string_file_simulator);
    ~CMainFrame();

    Settings& GetSettings() { return m_settings; }

    ConnectionStringFileSimulator& GetConnectionStringFileSimulator() { return *m_connectionStringFileSimulator; }

    void OpenDataSource(const ConnectionString& connection_string);

    void SetStatusBarPaneText(UINT indicator, const wchar_t* text);
    void ClearStatusBarPaneText();

    SharedHtmlLocalFileServer& GetSharedHtmlLocalFileServer() { ASSERT(m_fileServer != nullptr); return *m_fileServer; }

    bool HandleAcceleratorKeyFromWebView2(const CaseHoldingDoc& case_holding_doc, UINT key, INT lParam);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreCreateWindow(CREATESTRUCT& cs) override;

    int OnCreate(LPCREATESTRUCT lpCreateStruct);

    LRESULT DefWindowProc(UINT nMsg, WPARAM wParam, LPARAM lParam) override;
    void OnUpdateFrameTitle(BOOL bAddToTitle) override;

    void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);

    // File menu
    void OnFileOpen();
    void OnFileOpenDataSource();
    void OnFileOpenCSWebData();
    void OnFileCloseAll();
    void OnFileDownloadDataSource();
    void OnFileManageCredentials();

    // Windows menu
    void OnWindowWindows();

    // status bar handlers
    void OnUpdateStatusBar(CCmdUI* pCmdUI);

    // other message handlers
    LRESULT OnRunStartupActions(WPARAM wParam, LPARAM lParam);
    LRESULT OnDisplayErrorMessage(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetObjectTransporter(WPARAM wParam, LPARAM lParam);
    LRESULT OnRunOnUIThread(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetApplicationShutdownRunner(WPARAM wParam, LPARAM lParam);

    // interapp communication
    LRESULT OnIMSAFileOpen(WPARAM wParam, LPARAM lParam);

private:
    void SetUpAcceleratorKeyLookup();

private:
    const wchar_t* m_className;

    Settings m_settings;

    MFCMenuBarWithoutSerializableState m_wndMenuBar;
    CMFCStatusBar m_wndStatusBar;

    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;
    std::unique_ptr<ObjectTransporter> m_objectTransporter;
    ApplicationShutdownRunner m_applicationShutdownRunner;

    struct AcceleratorKeyData { enum class ReturnToWebView2 { Never, Potentially, Always };
                                std::variant<WORD, std::function<void()>> command_id_or_callback;
                                std::tuple<bool, bool, bool> alt_control_shift; ReturnToWebView2 result; };
    std::map<UINT, std::vector<AcceleratorKeyData>> m_acceleratorKeyMap;

    std::shared_ptr<ConnectionStringFileSimulator> m_connectionStringFileSimulator;
};
