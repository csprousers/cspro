#pragma once

#include <DataManager/CaseHoldingDoc.h>

class CaseProvider;
class CaseTask;
class ContentCreator;
class HtmlView;


class CaseHoldingFrame : public CMDIChildWndEx
{
protected:
    CaseHoldingFrame();

public:
    ~CaseHoldingFrame();

    bool IsShowingPage(UINT command_id) const;
    bool IsShowingPageOrNothing(UINT command_id) const;

    void PopulateViewOptionsMenu(CMenu& popup_menu);

protected:
    // methods that subclasses must override
    virtual CaseHoldingDoc& GetCaseHoldingDoc() = 0;
    virtual HtmlView& GetHtmlView() = 0;

    virtual std::unique_ptr<CaseProvider> CreateCaseProvider() = 0;

protected:
    DECLARE_MESSAGE_MAP()

    // File menu
    void OnFileExtractDictionary();

    void OnFileExtractNotes();
    void OnFileExtractNotes(std::shared_ptr<CaseProvider> case_provider);

    void OnFileExtractBinaryData();
    void OnFileExtractBinaryData(std::shared_ptr<CaseProvider> case_provider);
    void OnUpdateFileExtractBinaryData(CCmdUI* pCmdUI);

    void OnFileSaveView();
    void OnUpdateFileSaveView(CCmdUI* pCmdUI);

    void OnFilePrintView();
    void OnUpdateFilePrintView(CCmdUI* pCmdUI);

    // View menu
    void OnViewDataSummaryPage();
    void OnViewLogicHelperPage();
    void OnUpdateViewDataSourcePage(CCmdUI* pCmdUI);

    void OnViewCasePage(UINT nID);
    void OnUpdateViewCasePage(CCmdUI* pCmdUI);

    // View Options menu
    void OnViewOptionsLanguage(UINT nID);
    void OnUpdateViewOptionsLanguage(CCmdUI* pCmdUI);

    void OnViewOptions(UINT nID);
    void OnUpdateViewOptions(CCmdUI* pCmdUI);

    // message handlers
    LRESULT OnUpdateContentOnCaseListingSettingsChange(WPARAM wParam, LPARAM lParam);
    LRESULT OnUpdateContentOnCaseListingSelectionsChange(WPARAM wParam, LPARAM lParam);
    LRESULT OnShowDefaultPage(WPARAM wParam, LPARAM lParam);
    LRESULT OnProcessWebViewMessage(WPARAM wParam, LPARAM lParam);

protected:
    void RunCaseTask(std::shared_ptr<CaseProvider> case_provider, std::unique_ptr<CaseTask> case_task);

private:
    void ViewPage(std::unique_ptr<ContentCreator> content_creator);
    void UpdatePage();

    static std::string CreateHtmlPageForException(const CSProException& exception);

private:
    std::unique_ptr<ContentCreator> m_contentCreator;
};
