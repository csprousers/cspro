#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/UnfloatableDialogBar.h>

class LogicCtrl;
class LogicReferenceView;


// --------------------------------------------------------------------------
// LogicReferenceWnd
//
// This container hosts the logic reference window, LogicReferenceView.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF LogicReferenceWnd : public UnfloatableDialogBar
{
public:
    LogicReferenceWnd();

    bool CreateAndDock(CFrameWnd* frame_wnd, CDocument* doc);

    LogicCtrl* GetLogicCtrl() noexcept { return m_logicCtrl; }

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);

    void OnSizedOrDocked(int cx, int cy, BOOL bFloating, int flags) override;

private:
    CDocument* m_doc;
    LogicReferenceView* m_logicReferenceView;
    LogicCtrl* m_logicCtrl;
};
