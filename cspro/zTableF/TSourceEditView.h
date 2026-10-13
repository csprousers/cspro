#pragma once

#include <zTableF/zTableF.h>
#include <zEditO/LogicView.h>


class CLASS_DECL_ZTABLEF CTSourceEditView : public LogicView
{
    DECLARE_DYNCREATE(CTSourceEditView)

protected:
    DECLARE_MESSAGE_MAP()

    void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);

    void OnViewTable();
};
