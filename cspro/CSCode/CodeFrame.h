#pragma once

#include <zUtilF/SplitterWndWithDifferingViews.h>

class CLogicCtrl;
class CodeDoc;
class ReportPreviewer;


class CodeFrame : public CMDIChildWndEx
{
	DECLARE_DYNCREATE(CodeFrame)

protected:
    CodeFrame(); // create from serialization only

public:
    ~CodeFrame();

    CodeDoc& GetCodeDoc() { return assert_cast<CodeDoc&>(*GetActiveDocument()); }

    CodeView& GetActiveCodeView() { return *assert_cast<CodeView*>(m_splitterWnd.GetActivePane()); }

    void PopulateRunMenu(CMenu& popup_menu);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext) override;

    void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);

    void OnSize(UINT nType, int cx, int cy);

    LRESULT OnCodeFrameActivate(WPARAM wParam, LPARAM lParam);
    LRESULT OnSyncCodeFrameSplitter(WPARAM wParam = 0, LPARAM lParam = 0);

    // Language menu handlers
    // --------------------------------------------------------------------------
    void OnLanguageType(UINT nID);
    void OnUpdateLanguageType(CCmdUI* pCmdUI);

    // Code menu handlers
    // --------------------------------------------------------------------------
    void OnStringEncoder();
    void OnPathAdjuster();

    void OnDeprecationWarnings(UINT nID);
    void OnUpdateDeprecationWarnings(CCmdUI* pCmdUI);

    // Run menu handlers
    // --------------------------------------------------------------------------
    void OnRunRun();
    void OnUpdateRunRun(CCmdUI* pCmdUI);

    void OnRunReportPreview();
    void OnUpdateRunReportPreview(CCmdUI* pCmdUI);

    void OnRunActionInvokerDisplayResultsAsJson();
    void OnUpdateRunActionInvokerDisplayResultsAsJson(CCmdUI* pCmdUI);
    void OnRunActionInvokerAbortOnFirstException();
    void OnUpdateRunActionInvokerAbortOnFirstException(CCmdUI* pCmdUI);

    void OnRunLogicVersion(UINT nID);
    void OnUpdateRunLogicVersion(CCmdUI* pCmdUI);

    void OnRunSpecFileType(UINT nID);
    void OnUpdateRunSpecFileType(CCmdUI* pCmdUI);

    void OnRunJavaScriptModuleType(UINT nID);
    void OnUpdateRunJavaScriptModuleType(CCmdUI* pCmdUI);

    // Context menu handlers
    // --------------------------------------------------------------------------
    void OnUpdateDocumentMustBeSavedToDisk(CCmdUI* pCmdUI);

    void OnCopyFullPath();
    void OnOpenContainingFolder();
    void OnOpenInAssociatedApplication();

private:
    void CheckIfFileIsUpdated();

private:
    SplitterWndWithDifferingViews m_splitterWnd;

    WPARAM m_codeFrameActivatePostMessageCounter;
    int64_t m_lastCheckIfFileIsUpdatedTime;

    std::unique_ptr<ReportPreviewer> m_reportPreviewer;
};
