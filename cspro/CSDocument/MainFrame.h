#pragma once

#include <CSDocument/BuildWnd.h>
#include <CSDocument/DocSetCompiler.h>
#include <CSDocument/HtmlOutputWnd.h>
#include <zHtml/SharedHtmlLocalFileServer.h>
#include <zUtilF/MFCMenuBarWithoutSerializableState.h>

class DocSetSpec;
class TextEditDoc;
class TextEditView;


class CMainFrame : public CMDIFrameWndEx
{
public:
    CMainFrame(GlobalSettings global_settings);

    CSDocumentBuildWnd* GetBuildWnd(bool make_visible_if_not = true);
    HtmlOutputWnd* GetHtmlOutputWnd();

    TextEditDoc* GetActiveDoc();
    DocSetSpec* GetActiveDocSetSpec();
    TextEditView* GetActiveTextEditView();

    std::shared_ptr<DocSetSpec> FindSharedDocSetSpec(std::variant<wstring_view, DocSetSpec*> filename_or_doc_set_spec_ptr, bool open_if_not_found);
    bool IsCSDocPartOfDocSet(DocSetSpec& doc_set_spec, const std::wstring& csdoc_filename, bool match_full_path = true);

    void CompileDocSetSpecIfNecessary(DocSetSpec& doc_set_spec, DocSetCompiler::SpecCompilationType spec_compilation_type,
                                      DocSetCompiler::ErrorIssuerType error_issuer = DocSetCompiler::SuppressErrors { });

    GlobalSettings& GetGlobalSettings() { return m_globalSettings; }

    SharedHtmlLocalFileServer& GetSharedHtmlLocalFileServer() { return m_fileServer; }
    std::wstring CreateHtmlPage(const CDocument& doc, const std::wstring& html);
    std::wstring CreateHtmlCompilationErrorPage(const CDocument& doc);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL PreCreateWindow(CREATESTRUCT& cs) override;

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    void OnActivateApp(BOOL bActive, DWORD dwThreadID);
    void OnDestroy();

    void OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu);
    BOOL OnShowMDITabContextMenu(CPoint point, DWORD dwAllowedItems, BOOL bTabDrop) override;

    // toolbar + status bar + editor handlers
    LRESULT OnSyncToolbarAndWindows(WPARAM wParam, LPARAM lParam);

    void OnStatusBarAssociatedDocSetClick();
    void OnUpdateStatusBarAssociatedDocSet(CCmdUI* pCmdUI);
    LRESULT OnSetStatusBarFilePos(WPARAM wParam, LPARAM lParam);
    void OnUpdateStatusBar(CCmdUI* pCmdUI);
    void OnUpdateKeyOvertype(CCmdUI* pCmdUI);

    // context menu
    void OnUpdateDocumentMustBeSavedToDisk(CCmdUI* pCmdUI);

    void OnCopyFullPath();
    void OnCopyFilename();
    void OnOpenContainingFolder();

    // other handlers
    LRESULT OnFindOpenTextSourceEditable(WPARAM wParam, LPARAM lParam);
    LRESULT OnGetOpenTextSourceEditables(WPARAM wParam, LPARAM lParam);

    // interapp communication
    LRESULT OnIMSAFileOpen(WPARAM wParam, LPARAM lParam);    

    // File menu
    void OnFileNewDocumentSet();

    void OnFileCloseAll();

    void OnFileSaveAll();
    void OnUpdateFileSaveAll(CCmdUI* pCmdUI);

    void OnGlobalSettings();

    // Windows menu
    void OnWindowDocument(UINT nID);
    void OnWindowWindows();

    void OnWindowDockablePane(UINT nID);
    void OnUpdateWindowDockablePane(CCmdUI* pCmdUI);

private:
    void ShowDockablePane(CDockablePane& dockable_pane, BOOL visibility);

    void OnWebMessageReceived(const std::wstring& message);

private:
    const TCHAR* m_className;

    MFCMenuBarWithoutSerializableState m_wndMenuBar;
    CMFCToolBar m_wndMainToolBar;
    CMFCToolBar m_wndDocumentToolBar;
    CMFCStatusBar m_wndStatusBar;

    std::unique_ptr<CSDocumentBuildWnd> m_buildWnd;
    std::unique_ptr<HtmlOutputWnd> m_htmlOutputWnd;

    GlobalSettings m_globalSettings;

    SharedHtmlLocalFileServer m_fileServer;
    std::unique_ptr<VirtualFileMapping> m_virtualFileMapping;
    std::wstring m_compilationErrorPageHtml;
};
