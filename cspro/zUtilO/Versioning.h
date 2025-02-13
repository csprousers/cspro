#pragma once

#include <zUtilO/zUtilO.h>


// #define BETA_BUILD // comment out this line if not building a beta release

constexpr double CSPRO_VERSION_NUMBER =        8.0;
#define CSPRO_VERSION_NUMBER_TEXT          _T("8.0")
#define CSPRO_VERSION_NUMBER_DETAILED_TEXT _T("8.0.1")

#define CSPRO_VERSION                      ( _T("CSPro ") CSPRO_VERSION_NUMBER_TEXT )


constexpr bool IsBetaBuild()
{
#ifdef BETA_BUILD
    return true;
#else
    return false;
#endif
}


namespace Versioning
{
    CLASS_DECL_ZUTILO int GetReleaseDate();
    CLASS_DECL_ZUTILO CString GetVersionString(bool include_cspro = false);
    CLASS_DECL_ZUTILO CString GetVersionDetailedString(bool include_cspro = false);
    CLASS_DECL_ZUTILO CString GetReleaseDateString();
}
