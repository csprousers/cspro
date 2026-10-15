#pragma once

#include <zFormF/zFormF.h>
#include <zEditO/LogicView.h>


class CLASS_DECL_ZFORMF CFSourceEditView : public LogicView
{
    DECLARE_DYNCREATE(CFSourceEditView)

protected:
    DECLARE_MESSAGE_MAP()

    // LogicView override
    std::unique_ptr<CMenu> GetContextMenu() override;

    void OnViewForm();
    void OnQsfEditor();
    void OnSetFocus(CWnd* pOldWnd);
    void OnFormatLogic();
    void OnUpdateFormatLogic(CCmdUI* pCmdUI);
};
