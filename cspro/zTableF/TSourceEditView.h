#pragma once

#include <zTableF/zTableF.h>
#include <zDesignerF/ApplicationLogicView.h>


class CLASS_DECL_ZTABLEF CTSourceEditView : public ApplicationLogicView
{
    DECLARE_DYNCREATE(CTSourceEditView)

protected:
    // LogicView override
    std::unique_ptr<CMenu> GetContextMenu() override;
};
