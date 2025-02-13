#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/CSProException.h>


namespace FileIO
{
    struct CLASS_DECL_ZTOOLSO Exception : public CSProException
    {
        using CSProException::CSProException;

        static Exception DirectoryNotFound(NullTerminatedString path);

        static Exception FileNotFound(NullTerminatedString filename);
        static Exception FileOpenError(NullTerminatedString filename);
        static Exception FileNotFullyWritten(NullTerminatedString filename, bool delete_file_from_disk);
        static Exception FileCopyFail(NullTerminatedString source_path, NullTerminatedString destination_path);
        static Exception FileCopyFailDestinationExists(NullTerminatedString source_path, NullTerminatedString destination_path);
    };

    struct FileAndSize
    {
        FILE* file;
        int64_t size;
    };


    /// <summary>
    /// Opens a file, returning the FILE pointer and file size.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO FileAndSize OpenFile(NullTerminatedString filename);

    /// <summary>
    /// Reads an entire file.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO std::unique_ptr<std::vector<std::byte>> Read(NullTerminatedString filename);

    /// <summary>
    /// Reads an entire file, assuming a UTF-8 encoding (BOM or not) and returns the file as a string.
    /// The function can return either a std::wstring or a std::string.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    template<typename StringType = std::wstring>
    CLASS_DECL_ZTOOLSO StringType ReadText(NullTerminatedString filename);

    /// <summary>
    /// Reads a file, assuming a UTF-8 encoding (BOM or not) and returns the file as a string.
    /// The second argument controls the maximum number of bytes to read. An optional message
    /// is appended to the string when the maximum number of bytes is read.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO CString ReadText(NullTerminatedString filename, int64_t max_bytes_to_read,
	                                    const TCHAR* message = nullptr);

    /// <summary>
    /// Opens an input stream for wide character text input based on the contents of a UTF-8 file.
    /// The BOM will be skipped if it exists.
    /// This function and stream operations throw FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO std::unique_ptr<std::wistream> OpenWideTextInputFileStream(NullTerminatedString filename);

    /// <summary>
    /// Opens an input stream for non-wide character text input based on the contents of a UTF-8 file.
    /// The BOM will be skipped if it exists.
    /// This function and stream operations throw FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO std::unique_ptr<std::ifstream> OpenTextInputFileStream(NullTerminatedString filename);


    /// <summary>
    /// Creates any directories that do not exist.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO void CreateDirectories(const std::wstring& directory);
    CLASS_DECL_ZTOOLSO void CreateDirectoriesForFile(NullTerminatedString filename);

    /// <summary>
    /// Opens a file stream for output.
    /// Any directories that do not exist will be created.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO std::unique_ptr<std::ofstream> OpenOutputFileStream(NullTerminatedString filename);

    /// <summary>
    /// Opens a file for output.
    /// Any directories that do not exist will be created.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO FILE* OpenFileForOutput(NullTerminatedString filename);

    /// <summary>
    /// Any directories that do not exist will be created.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO void Write(NullTerminatedString filename, const std::byte* content, size_t content_size);

    /// <summary>
    /// Any directories that do not exist will be created.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    template<typename T>
    void Write(NullTerminatedString filename, const T& content)
    {
        static_assert(sizeof(*content.data()) == sizeof(std::byte));
        Write(filename, reinterpret_cast<const std::byte*>(content.data()), content.size());
    }

    /// <summary>
    /// Writes the text in UTF-8, with or without a BOM.
    /// Any directories that do not exist will be created.
    /// This function throws FileIO::Exception exceptions.
    /// </summary>
    CLASS_DECL_ZTOOLSO void WriteText(NullTerminatedString filename, std::string_view text_content, bool write_utf8_bom);
    CLASS_DECL_ZTOOLSO void WriteText(NullTerminatedString filename, wstring_view text_content, bool write_utf8_bom);
}
