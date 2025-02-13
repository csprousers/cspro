#pragma once

#include <zEdit2O/zEdit2O.h>
#include <zEdit2O/EditCtrl.h>


class CLASS_DECL_ZEDIT2O MessageEditCtrl : public EditCtrl
{
protected:
    void InitializeControl() override;

    bool CanCommentLine() override { return true; }
};
