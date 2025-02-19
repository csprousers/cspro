#pragma once

#include <zZipo/zZipo.h>
#include <zZipo/ZipException.h>
#include <zToolsO/BinaryBlock.h>

class ZipImpl;


// --------------------------------------------------------------------------
// ZipReader + ZipCreator + ZipUtility
//
// These classes provide a simple interface for reading or creating ZIP
// files. The ZIP file is closed in the destructor (or the Close method).
//
// The constructor and methods will throw ZipException exceptions on error.
// --------------------------------------------------------------------------


// --------------------------------------------------------------------------
// ZipReader
// --------------------------------------------------------------------------

class CLASS_DECL_ZZIPO ZipReader
{
    friend class ZipCreator;
    friend class ZipUtility;

public:
    // Indicates if the data is the header of a ZIP file.
    static bool IsZipHeader(cs::string_sz data);

    // Opens the ZIP file.
    ZipReader(const std::string& file_path);

    // Opens the ZIP archive stored in memory.
    ZipReader(const char* archive_buffer, size_t archive_length, const char* file_path_for_error_message = nullptr);

    ZipReader(const ZipReader&) = delete;
    ZipReader(ZipReader&&);

    ~ZipReader();

    // Reads the specific file into memory.
    BinaryBlock Read(const std::string& path_in_zip);

    // Extracts the specific file to the output path, overwriting any existing file.
    // Directories needed to write the output file will be created as necessary.
    void Extract(const std::string& path_in_zip, const std::string& output_path);

    // Extracts all files to the output directory, overwriting any existing files.
    // Directories needed to write the output files will be created as necessary.
    // The number of files extracted is returned.
    int ExtractAll(const std::string& output_directory);

    // Runs the callback function for each file in the ZIP file.
    // The callback, which receives the file path, should return true to continue processing.
    void ForeachFilePath(const std::function<bool(const char*)>& callback_function);

    // Runs the callback function for each file in the ZIP file.
    // The callback, which receives the file path and contents, should return true to continue processing.
    void ForeachFileContents(const std::function<bool(const char*, BinaryBlock)>& callback_function);

    // Finds the path within the ZIP file for the given filename; the search is case-insensitive.
    // If not found, std::nullopt is returned.
    std::optional<std::string> FindPathInZip(std::string_view filename_sv);

private:
    std::unique_ptr<ZipImpl> ReleaseZipImpl();

    template<typename FileStat>
    BinaryBlock Read(const FileStat& file_stat, cs::string_sz path_in_zip);

    template<typename CF>
    void ForeachFileWorker(const CF& callback_function);

private:
    std::unique_ptr<ZipImpl> m_impl;
};


// --------------------------------------------------------------------------
// ZipCreator
// --------------------------------------------------------------------------

class CLASS_DECL_ZZIPO ZipCreator
{
    friend class ZipUtility;

public:
    // Creates a new ZIP file, deleting the file if it already exists.
    // Directories needed to write the output file will be created as necessary.
    // An exception is thrown if the file cannot be created, or the existing file cannot be deleted.
    ZipCreator(std::string file_path);

    ZipCreator(const ZipCreator&) = delete;
    ZipCreator(ZipCreator&&);

    ~ZipCreator();

    // Ends creation of the ZIP file. This method is also called in the destructor, but when called
    // manually, an exception is thrown if there is an error finalizing the ZIP file.
    void Close();

    // Adds the files with the file paths in the ZIP specified.
    // The number of files (not directories) compressed is returned.
    int AddFiles(const std::vector<std::string>& file_paths, const std::vector<std::string>& file_paths_in_zip);

    // Adds the files without specifying the file paths in the ZIP.
    // The Path::GetCommonRoot function is used to determine the file paths in the ZIP.
    // An exception is thrown if there is no common root.
    // The number of files (not directories) compressed is returned.
    int AddFiles(const std::vector<std::string>& file_paths);

    // Adds the files from another ZIP file.
    // An exception is thrown if the file does not exist in the ZIP file.
    void AddFiles(ZipReader& zip_reader, const std::vector<std::string>& file_paths_in_zip);

    // Adds the content in the buffer as a file in the ZIP file.
    void AddContent(std::string file_path_in_zip, const void* buffer, size_t buffer_size);
    void AddContent(std::string file_path_in_zip, std::string_view text_sv);

private:
    static void Close(std::unique_ptr<ZipImpl>& impl, const std::string& file_path);

    // Makes sure that the file path is properly defined and creates directories as needed to store the file.
    void NormalizeFilePathAndCreateDirectoriesForFile(std::string& file_path_in_zip, std::set<std::string>* directories_in_zip);

private:
    std::unique_ptr<ZipImpl> m_impl;
    std::string m_filePath;
};



// --------------------------------------------------------------------------
// ZipUtility
// --------------------------------------------------------------------------

class CLASS_DECL_ZZIPO ZipUtility
{
public:
    // Opens a base ZIP file and merges into it the contents of another ZIP file (combine_file_path).
    // If a file exists in both ZIP files, the one in base_file_path is maintained.
    // Exceptions are thrown on error, and an error can leave the base ZIP file in a bad state.
    static void Combine(const std::string& base_file_path, const std::string& combine_file_path);
};
