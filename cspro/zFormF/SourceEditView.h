#pragma once

#include <zFormF/zFormF.h>
#include <zEditO/LogicView.h>


class CLASS_DECL_ZFORMF CFSourceEditView : public LogicView
{
    DECLARE_DYNCREATE(CFSourceEditView)

protected:
    DECLARE_MESSAGE_MAP()

    void OnContextMenu(CWnd* pWnd, CPoint point);
    void OnViewForm();
    void OnQsfEditor();
    void OnSetFocus(CWnd* pOldWnd);
    void OnShiftF10();
    void OnFormatLogic();
    void OnUpdateFormatLogic(CCmdUI* pCmdUI);
};
