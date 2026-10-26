#pragma once

#include <zEditO/LogicView.h>


// --------------------------------------------------------------------------
// MessageEditView
//
// This view is for editing message files as part of a DesignerDoc.
//
// The view is hosted in a tab page, MessageEditTabViewPage.
// --------------------------------------------------------------------------

class MessageEditView : public LogicView
{
    DECLARE_DYNCREATE(MessageEditView)

protected:
    MessageEditView() noexcept;

protected:
    void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) override;

    // LogicView overrides
    void InitializeLogicControl() override;

private:
    void OnDesignerDocLoaded();
    void OnLogicSettingsChanged();

private:
    DesignerDoc* m_designerDoc; // non-null after OnUpdate's Hint::DesignerDocLoaded
};
