#include "stdafx.h"
#include "ReadOnlyEditCtrl.h"


void ReadOnlyEditCtrl::InitializeControl()
{
    SetAccelerators(IDR_READ_ONLY_EDIT);

    BaseInitControl(SCLEX_NULL);

    SetMargins(0);
    SetReadOnly(TRUE);
}
