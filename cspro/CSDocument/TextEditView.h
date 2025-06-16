#pragma once

#include <CSDocument/TextEditDoc.h>
#include <zEditO/LogicView.h>


class TextEditView : public CLogicView
{
    DECLARE_DYNCREATE(TextEditView)

protected:
    TextEditView() { } // create from serialization only

public:
    TextEditDoc& GetTextEditDoc() { return assert_cast<TextEditDoc&>(*GetDocument()); }

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
