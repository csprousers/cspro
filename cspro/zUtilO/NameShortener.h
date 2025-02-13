#pragma once
#include <zUtilO/zUtilO.h>

namespace CSProNameShortener
{
    CString CLASS_DECL_ZUTILO CSProToUnicode(CString cspro_name, int maximum_name_size = 32);
    CString CLASS_DECL_ZUTILO UnicodeToCSPro(CString full_name);
}
