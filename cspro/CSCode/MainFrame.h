#pragma once

#include <CSCode/BuildWnd.h>
#include <CSCode/HtmlViewerWnd.h>
#include <CSCode/OutputWnd.h>
#include <zUtilO/WindowHelpers.h>
#include <zUtilF/ApplicationShutdownRunner.h>
#include <zUtilF/MFCMenuBarWithoutSerializableState.h>

class CDataDict;
class ChmFileVirtualFile;
class CodeDoc;
class DocumentVirtualFileMappingHandler;
class EngineUIProcessor;
class LanguageSettingsPersister;
class ObjectTransporter;


class CMainFrame : public CMDIFrameWndEx
{
public:
    CMainFrame();
    ~CMainFrame();

    CSCodeBuildWnd* GetBuildWnd(bool make_visible_if_not = true);
    OutputWnd* GetOutputWnd();
    HtmlViewerWnd* GetHtmlViewerWnd();

    CodeDoc* GetActiveCodeDoc();

    CodeDoc* FindDocument(const std::variant<const CDocument*, std::wstring>& document_or_filename);
    bool ActivateDocument(CodeView* code_view);

    bool IsRunOperationInProgress() const { return ( m_currentRunOperation != nullptr && m_currentRunOperation->IsRunning() ); }
    void RegisterRunOperationAndRun(std::shared_ptr<RunOperation> run_operation);

    LanguageSettingsPersister& GetLanguageSettingsPersister();

    SharedHtmlLocalFileServer& GetSharedHtmlLocalFileServer();

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    void OnClose();

    void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    BOOL OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bTabDrop) override;

    // File menu handlers
    void OnFileOpen();

    void OnFileCloseAll();

    void OnFileSaveAll();
    void OnUpdateFileSaveAll(CCmdUI* pCmdUI);

    void OnFileLocalhostSettings();

    // Run menu handlers
    void OnRunStop();
    void OnUpdateRunStop(CCmdUI* pCmdUI);

    void OnRunHtmlDialogTemplates();

    // Windows menu handlers
    void OnWindowDocument(UINT nID);
    void OnWindowWindows();

    void OnWindowDockablePane(UINT nID);
    void OnUpdateWindowDockablePane(CCmdUI* pCmdUI);

    // context menu handlers
    void OnCloseAllButThis();
    void OnLocalhostMapping(UINT nID);

    // toolbar + status bar + editor handlers
    LRESULT OnSyncToolbar(WPARAM wParam, LPARAM lParam);

    LRESULT OnSetStatusBarFilePos(WPARAM wParam, LPARAM lParam);
    LRESULT OnSetStatusBarFileType(WPARAM wParam, LPARAM lParam);
    void OnUpdateStatusBar(CCmdUI* pCmdUI);
    void OnUpdateKeyOvertype(CCmdUI* pCmdUI);

    // other message handlers
    LRESULT OnIsReservedWord(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetCodeText(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetCodeTextForDoc(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetSharedDictionaryConst(WPARAM wParam, LPARAM lParam);

    LRESULT OnStartLocalhost(WPARAM wParam, LPARAM lParam);
    LRESULT OnRunOperationComplete(WPARAM wParam, LPARAM lParam);
    LRESULT OnModelessDialogDestroyed(WPARAM wParam, LPARAM lParam);

    LRESULT OnGetObjectTransporter(WPARAM wParam, LPARAM lParam);
    LRESULT OnEngineUI(WPARAM wParam, LPARAM lParam);
    LRESULT OnRunOnUIThread(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetApplicationShutdownRunner(WPARAM wParam, LPARAM lParam);

private:
    void ShowDockablePane(CDockablePane& dockable_pane, BOOL visibility);

    void SetStatusBarPaneText(UINT indicator, const TCHAR* text);

    std::wstring GetLocalhostMappingUrlForActiveDocument();

private:
    MFCMenuBarWithoutSerializableState m_wndMenuBar;
    CMFCToolBar m_wndMainToolBar;
    CMFCToolBar m_wndCodeToolBar;
    CMFCStatusBar m_wndStatusBar;

    std::unique_ptr<CSCodeBuildWnd> m_buildWnd;
    std::unique_ptr<OutputWnd> m_outputWnd;
    
    std::unique_ptr<HtmlViewerWnd> m_htmlViewerWnd;
    std::unique_ptr<ChmFileVirtualFile> m_chmFileVirtualFile;

    std::unique_ptr<LanguageSettingsPersister> m_languageSettingsPersister;

    std::unique_ptr<SharedHtmlLocalFileServer> m_fileServer;
    std::vector<std::tuple<std::wstring, std::unique_ptr<DocumentVirtualFileMappingHandler>>> m_documentVirtualFileMappingHandlers;

    std::shared_ptr<RunOperation> m_currentRunOperation;

    enum class ModelessDialogType { HtmlDialogTemplates };
    std::map<ModelessDialogType, WindowHelpers::ModelessDialogHolder> m_modelessDialogHolders;

    std::map<StringNoCase, std::tuple<int64_t, std::shared_ptr<const CDataDict>>> m_loadedDictionaries;

    std::unique_ptr<ObjectTransporter> m_objectTransporter;
    std::unique_ptr<EngineUIProcessor> m_engineUIProcessor;
    ApplicationShutdownRunner m_applicationShutdownRunner;
};
