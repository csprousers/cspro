#pragma once

#include <zFormF/zFormF.h>
#include <zDesignerF/ApplicationLogicView.h>


class CLASS_DECL_ZFORMF CFSourceEditView : public ApplicationLogicView
{
    DECLARE_DYNCREATE(CFSourceEditView)

protected:
    DECLARE_MESSAGE_MAP()

    // LogicView override
    std::unique_ptr<CMenu> GetContextMenu() override;

    void OnViewForm();
    void OnQsfEditor();
    void OnSetFocus(CWnd* pOldWnd);
};
