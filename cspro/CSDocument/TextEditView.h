#pragma once

#include <CSDocument/TextEditDoc.h>
#include <zEdit2O/LogicView.h>


class TextEditView : public CLogicView
{
    DECLARE_DYNCREATE(TextEditView)

protected:
    TextEditView() { } // create from serialization only

public:
    TextEditDoc& GetTextEditDoc() { return assert_cast<TextEditDoc&>(*GetDocument()); }

    void SetTextAndSetSavePoint(const std::wstring& text);

protected:
    DECLARE_MESSAGE_MAP()

    void OnInitialUpdate() override;
	void OnActivateView(BOOL bActivate, CView* pActivateView, CView* pDeactiveView) override;

    void OnSavePointReached(Scintilla::NotificationData* pSCNotification) override;
    void OnSavePointLeft(Scintilla::NotificationData* pSCNotification) override;

    bool OnHandleHelp() override;

    void OnFindNext();

    void OnWordWrap();
    void OnUpdateWordWrap(CCmdUI* pCmdUI);
};
