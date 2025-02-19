#pragma once

#include <zUtilO/zUtilO.h>


// Creates a uniquely named temporary file and deletes it on destruction.

class CLASS_DECL_ZUTILO TemporaryFile
{
private:
    TemporaryFile(std::string file_path, bool delete_on_destruction);

public:
    // Creates a temporary file in the system temp directory.
    // Depending on the platform this may create an empty file on the disk (to prevent another process from using the same name)
    // or may just generate a unique file name.
    TemporaryFile();

    // Creates a temporary file in the specified directory.
    // Depending on the platform this may create an empty file on the disk (to prevent another process from using the same name)
    // or may just generate a unique file name.
    explicit TemporaryFile(const std::string& directory_path);

    // Wraps the supplied path around a TemporaryFile object, so the file will be deleted on destruction.
    static TemporaryFile FromPath(std::string file_path);

    TemporaryFile(const TemporaryFile&) = delete;
    TemporaryFile(TemporaryFile&& rhs) noexcept;

    ~TemporaryFile();

    TemporaryFile& operator=(const TemporaryFile&) = delete;
    TemporaryFile& operator=(TemporaryFile&& rhs) noexcept;

    // Returns the full path of the file.
    const std::string& GetPath() const { return m_filePath; }

    // Moves/renames the file. It will no longer be deleted on destruction.
    void Rename(std::string new_file_path);
    bool Rename_noexcept(std::string new_file_path);

    // Adds a file to a registry of files to be deleting upon program close.
    static void RegisterFileForDeletion(std::string file_path);

private:
    std::string m_filePath;
    bool m_deleteOnDestruction;
};
