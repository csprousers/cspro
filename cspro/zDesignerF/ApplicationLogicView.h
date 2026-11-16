#pragma once

#include <zDesignerF/zDesignerF.h>
#include <zEditO/LogicView.h>


// --------------------------------------------------------------------------
// ApplicationLogicView
//
// This LogicView subclass is the parent class for the main logic window
// shown for applications.
// --------------------------------------------------------------------------

class CLASS_DECL_ZDESIGNERF ApplicationLogicView : public LogicView
{
    DECLARE_DYNAMIC(ApplicationLogicView)

protected:
    ApplicationLogicView() = default;

protected:
    DECLARE_MESSAGE_MAP()

    void OnFormatLogic();
};
