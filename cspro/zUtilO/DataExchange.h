#pragma once

#include <zUtilO/zUtilO.h>

class ConnectionString;


// --------------------------------------------------
// DDX_Text
// --------------------------------------------------

CLASS_DECL_ZUTILO void DDX_Text(CDataExchange* pDX, int nIDC, std::wstring& text, bool trim_string_on_save = false);

CLASS_DECL_ZUTILO void DDX_Text(CDataExchange* pDX, int nIDC, ConnectionString& connection_string);


// --------------------------------------------------
// DDX_CBString + DDX_CBStringExact
// --------------------------------------------------

CLASS_DECL_ZUTILO void DDX_CBString(CDataExchange* pDX, int nIDC, std::wstring& text);
CLASS_DECL_ZUTILO void DDX_CBStringExact(CDataExchange* pDX, int nIDC, std::wstring& text);
