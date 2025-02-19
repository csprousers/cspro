#pragma once

#include <zUtilO/zUtilO.h>

enum class FileOverwriteFlag : int;


// some portable routines related to the file system

class CLASS_DECL_ZUTILO PortableFileSystem
{
public:
    static bool IsSharableUri(std::string_view uri_sv);

    // throws exceptions
    static std::string CreateSharableUri(const std::string& path, bool add_write_permission);

    // throws exceptions
    static bool FileCopy(const std::string& source_path_or_sharable_uri, const std::string& destination_path, FileOverwriteFlag file_overwrite_flag);
};
