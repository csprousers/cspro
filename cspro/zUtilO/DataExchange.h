#pragma once

#include <zUtilO/zUtilO.h>

class ConnectionString;
class SyncConnectionString;


// --------------------------------------------------------------------------
// DDX_Check
// --------------------------------------------------------------------------

CLASS_DECL_ZUTILO void DDX_Check(CDataExchange* pDX, int nIDC, bool& value);


// --------------------------------------------------------------------------
// DDX_Text
// --------------------------------------------------------------------------

CLASS_DECL_ZUTILO void DDX_Text(CDataExchange* pDX, int nIDC, std::wstring& text, bool trim_string_on_save = false);
CLASS_DECL_ZUTILO void DDX_Text(CDataExchange* pDX, int nIDC, std::string& text, bool trim_string_on_save = false);

// When saving, newlines are converted to '\n' and when loading, newlines are converted to "\r\n".
CLASS_DECL_ZUTILO void DDX_TextOnlyLF(CDataExchange* pDX, int nIDC, std::string& text, bool trim_string_on_save = false);

CLASS_DECL_ZUTILO void DDX_Text(CDataExchange* pDX, int nIDC, ConnectionString& connection_string);
CLASS_DECL_ZUTILO void DDX_Text(CDataExchange* pDX, int nIDC, SyncConnectionString& sync_connection_string);


// --------------------------------------------------------------------------
// DDX_CBString + DDX_CBStringExact
// --------------------------------------------------------------------------

CLASS_DECL_ZUTILO void DDX_CBString(CDataExchange* pDX, int nIDC, std::wstring& text);
CLASS_DECL_ZUTILO void DDX_CBStringExact(CDataExchange* pDX, int nIDC, std::wstring& text);
CLASS_DECL_ZUTILO void DDX_CBStringExact(CDataExchange* pDX, int nIDC, std::string& text);


// --------------------------------------------------------------------------
// DDV_MaxChars
// --------------------------------------------------------------------------

CLASS_DECL_ZUTILO void DDV_MaxChars(CDataExchange* pDX, std::string& text, int nChars);
