#pragma once


class TextEditFrame : public CMDIChildWndEx
{
protected:
    TextEditFrame();

public:
    virtual ~TextEditFrame() { }

    virtual TextEditView& GetTextEditView() = 0;

    TextEditDoc& GetTextEditDoc() { return assert_cast<TextEditDoc&>(*GetActiveDocument()); }

protected:
    DECLARE_MESSAGE_MAP()

    void OnMDIActivate(BOOL bActivate, CWnd* pActivateWnd, CWnd* pDeactivateWnd);

    LRESULT OnTextEditFrameActivate(WPARAM wParam, LPARAM lParam);

    void OnUpdateDocumentMustBeSavedToDisk(CCmdUI* pCmdUI);

protected:
    CMainFrame& GetMainFrame() { return *assert_cast<CMainFrame*>(AfxGetMainWnd()); }

private:
    void CheckIfFileIsUpdated();

private:
    WPARAM m_codeFrameActivatePostMessageCounter;
    int64_t m_lastCheckIfFileIsUpdatedTime;
};
