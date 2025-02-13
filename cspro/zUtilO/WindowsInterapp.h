#pragma once

#include <zUtilO/zUtilO.h>

#ifndef WIN_DESKTOP
#error You should not include this file for platforms other than Windows desktop
#endif


CLASS_DECL_ZUTILO void OpenContainingFolder(NullTerminatedString path);

CLASS_DECL_ZUTILO int GetDesignerFontZoomLevel();
CLASS_DECL_ZUTILO void SetDesignerFontZoomLevel(int iZoomLevel);

CLASS_DECL_ZUTILO CString GetDesignerFontName();
CLASS_DECL_ZUTILO void SetDesignerFontName(const TCHAR* font_name);
