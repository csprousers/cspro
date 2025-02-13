#pragma once

#include <zZipo/zZipo.h>

struct mz_zip_archive_tag;


// a simple class for reading zip files that keeps the zip file open for the duration of the class;
// the constructor and methods will throw CSProException exceptions on error

class CLASS_DECL_ZZIPO ZipReader
{
public:
    ZipReader(NullTerminatedString filename);
    ~ZipReader();

    std::unique_ptr<std::vector<std::byte>> Read(wstring_view path_in_zip);

    // runs the callback function for each file in the zip file; the callback should
    // return true to continue processing
    void ForeachFile(const std::function<bool(const std::wstring&)>& callback);

    // find the path within the zip of the given filename; if not found, std::nullopt is returned
    std::optional<std::wstring> FindPathInZip(wstring_view filename);

private:
    std::unique_ptr<mz_zip_archive_tag> m_zipArchive;
};
