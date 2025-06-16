#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/EditCtrl.h>


class CLASS_DECL_ZEDITO MessageEditCtrl : public EditCtrl
{
protected:
    void InitializeControl() override;

    bool CanCommentLine() override { return true; }
};
