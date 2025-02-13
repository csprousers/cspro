#pragma once

#include <zEdit2O/zEdit2O.h>
#include <zEdit2O/EditCtrl.h>


class CLASS_DECL_ZEDIT2O ReadOnlyEditCtrl : public EditCtrl
{
public:
    void ClearReadOnlyText();
    void AppendReadOnlyText(wstring_view text_sv);

protected:
    void InitializeControl() override;

    bool CanCommentLine() override { return false; }
};
