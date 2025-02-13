#pragma once

#include <zUtilF/zUtilF.h>


// some (well, one for now) CEdit subclasses


// --------------------------------------------------------------------------
// CEditWithSelectAll: selects all text on Ctrl+A
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILF CEditWithSelectAll : public CEdit
{
public:
    BOOL PreTranslateMessage(MSG* pMsg) override;
};
