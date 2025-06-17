#pragma once

#include <zEditO/zEditO.h>
#include <zEditO/EditCtrl.h>


class CLASS_DECL_ZEDITO ReadOnlyEditCtrl : public EditCtrl
{
public:
    void ClearReadOnlyText();
    void AppendReadOnlyText(std::string_view text_sv);

protected:
    void InitializeControl() override;

    bool CanCommentLine() override { return false; }
};
