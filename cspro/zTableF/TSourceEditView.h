#pragma once

#include <zTableF/zTableF.h>
#include <zEditO/LogicView.h>


class CLASS_DECL_ZTABLEF CTSourceEditView : public LogicView
{
    DECLARE_DYNCREATE(CTSourceEditView)

protected:
    // LogicView override
    std::unique_ptr<CMenu> GetContextMenu() override;
};
