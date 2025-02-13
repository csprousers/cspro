#include "stdafx.h"
#include "ReadOnlyEditCtrl.h"


void ReadOnlyEditCtrl::InitializeControl()
{
    SetAccelerators(IDR_READ_ONLY_EDIT);
    SetCtxMenuId(IDR_READ_ONLY_EDIT);

    BaseInitControl(SCLEX_NULL);

    SetMargins(0);
    SetReadOnly(TRUE);
}


void ReadOnlyEditCtrl::ClearReadOnlyText()
{
    SetReadOnly(FALSE);
    ClearAll();
    SetReadOnly(TRUE);
}


void ReadOnlyEditCtrl::AppendReadOnlyText(wstring_view text_sv)
{
    SetReadOnly(FALSE);
    AppendText(text_sv.length(), text_sv.data());
    SetReadOnly(TRUE);
}
