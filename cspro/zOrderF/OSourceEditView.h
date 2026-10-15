#pragma once

#include <zEditO/LogicView.h>


class CLASS_DECL_ZORDERF COSourceEditView : public LogicView
{
    DECLARE_DYNCREATE(COSourceEditView)

protected:
    COSourceEditView() noexcept = default; // create from serialization only
};
