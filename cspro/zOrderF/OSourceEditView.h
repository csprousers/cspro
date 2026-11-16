#pragma once

#include <zDesignerF/ApplicationLogicView.h>


class CLASS_DECL_ZORDERF COSourceEditView : public ApplicationLogicView
{
    DECLARE_DYNCREATE(COSourceEditView)

protected:
    COSourceEditView() noexcept = default; // create from serialization only
};
