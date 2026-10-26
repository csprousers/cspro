#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/EditCtrl.h>


class CLASS_DECL_ZEDITO ReadOnlyEditCtrl : public EditCtrl
{
protected:
    void InitializeControl() override;
};
