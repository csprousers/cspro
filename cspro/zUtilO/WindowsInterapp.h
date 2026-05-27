#pragma once

#include <zUtilO/zUtilO.h>

#ifndef _AFX
#error You should not include this file for platforms other than Windows desktop
#endif


template<bool ThrowExceptionOnError = true>
CLASS_DECL_ZUTILO std::conditional_t<ThrowExceptionOnError, void, bool> OpenFileInAssociatedApplication(const InterfaceString& file_path);

CLASS_DECL_ZUTILO void OpenContainingFolder(NullTerminatedString path);
CLASS_DECL_ZUTILO void OpenContainingFolder(std::string_view path_sv);

CLASS_DECL_ZUTILO int GetDesignerFontZoomLevel();
CLASS_DECL_ZUTILO void SetDesignerFontZoomLevel(int zoom_level);

CLASS_DECL_ZUTILO CString GetDesignerFontName();
CLASS_DECL_ZUTILO void SetDesignerFontName(NullTerminatedString font_name);
