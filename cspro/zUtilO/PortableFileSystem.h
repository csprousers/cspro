#pragma once

#include <zUtilO/zUtilO.h>


// some portable routines related to the file system

class CLASS_DECL_ZUTILO PortableFileSystem
{
public:
    static bool IsSharableUri(wstring_view uri_sv);

    // throws exceptions
    static std::wstring CreateSharableUri(const std::wstring& path, bool add_write_permission);

    // throws exceptions
    static void FileCopy(const std::wstring& source_path_or_sharable_uri, const std::wstring& destination_path, bool overwrite);
};
