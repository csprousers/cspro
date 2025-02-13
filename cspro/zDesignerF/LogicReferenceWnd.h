#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zDesignerF/UnfloatableDialogBar.h>
#include <zEdit2O/ReadOnlyEditCtrl.h>


class CLASS_DECL_ZDESIGNERF LogicReferenceWnd : public UnfloatableDialogBar
{
public:
    LogicReferenceWnd();

    bool CreateAndDock(CFrameWnd* pFrameWnd);

    ReadOnlyEditCtrl* GetEditCtrl() { return &m_editCtrl; }

protected:
    DECLARE_MESSAGE_MAP()

    int OnCreate(LPCREATESTRUCT lpCreateStruct);
    BOOL OnInitDialog() override;

private:
    ReadOnlyEditCtrl m_editCtrl;
};
