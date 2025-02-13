#pragma once

#include <zUtilO/zUtilO.h>


// Creates a uniquely named temporary file and deletes it on destruction.

class CLASS_DECL_ZUTILO TemporaryFile
{
private:
    TemporaryFile(std::wstring path, bool delete_on_destruction);

public:
    // Creates a temporary file in the system temp directory.
    // Depending on the platform this may create an empty file on the disk (to prevent another process from using the same name)
    // or may just generate a unique file name.
    TemporaryFile();

    // Creates a temporary file in the specified directory.
    // Depending on the platform this may create an empty file on the disk (to prevent another process from using the same name)
    // or may just generate a unique file name.
    explicit TemporaryFile(NullTerminatedString directory);

    // Wraps the supplied path around a TemporaryFile object, so the file will be deleted on destruction.
    static TemporaryFile FromPath(std::wstring path);

    TemporaryFile(const TemporaryFile& rhs) = delete;
    TemporaryFile(TemporaryFile&& rhs) noexcept;

    ~TemporaryFile();

    TemporaryFile& operator=(const TemporaryFile&) = delete;
    TemporaryFile& operator=(TemporaryFile&& rhs) noexcept;

    /// <summary>Return full path of the file</summary>
    const std::wstring& GetPath() const { return m_path; }

    // Moves/renames the file. It will no longer be deleted on destruction.
    bool Rename(std::wstring new_path);

    // Adds a file to a registry of files to be deleting upon program close.
    static void RegisterFileForDeletion(std::wstring filename);

private:
    std::wstring m_path;
    bool m_deleteOnDestruction;
};
