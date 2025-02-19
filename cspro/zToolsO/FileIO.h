#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/CSProException.h>
#include <zToolsO/TextEncoding.h>


namespace FileIO
{
    class File;

    class CLASS_DECL_ZTOOLSO Exception : public CSProException
    {
    public:
        using CSProException::CSProException;

        static Exception DirectoryNotFound(InterfaceString directory_path);

        static Exception FileNotFound(InterfaceString file_path);
        static Exception FileOpenError(InterfaceString file_path);
        static Exception FileCreateError(InterfaceString file_path);
        static Exception FileReadError(InterfaceString file_path);
        static Exception FileNotFullyWritten(InterfaceString file_path, bool delete_file_from_disk);
        static Exception FileCopyFail(InterfaceString source_path, InterfaceString destination_path);
        static Exception FileCopyFailDestinationExists(InterfaceString source_path, InterfaceString destination_path);
        static Exception FileMoveFail(InterfaceString source_path, InterfaceString destination_path);
        static Exception FileDeleteFail(InterfaceString file_path);
    };

    struct FileAndSize
    {
        FILE* file;
        int64_t size;
    };


    // Opens a file, returning the FILE pointer and file size.
    // This function throws FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO FileAndSize OpenFile(InterfaceString file_path);

    // Reads an entire file.
    // This function throws FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO std::unique_ptr<std::vector<std::byte>> Read(InterfaceString file_path);
    CLASS_DECL_ZTOOLSO BinaryBlock ReadBinary(InterfaceString file_path);

    // Reads an entire file, using the encoding specified in text_encoding if no BOM exists,
    // and returns the file as a UTF-8 string.
    // This function throws FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO std::string ReadText(InterfaceString file_path, TextEncoding text_encoding = TextEncoding::Type::Utf8);

    // Reads a file, assuming a UTF-8 encoding (BOM or not) and returns the file as a string.
    // The second argument controls the maximum number of bytes to read. An optional message
    // is appended to the string when the maximum number of bytes is read.
    // This function throws FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO std::string ReadText(InterfaceString file_path, int64_t max_bytes_to_read,
	                                        const char* message = nullptr);

    // Opens an input stream for wide character text input based on the contents of a UTF-8 file.
    // The BOM will be skipped if it exists.
    // This function and stream operations throw FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO std::unique_ptr<std::wistream> OpenWideTextInputFileStream(InterfaceString file_path);

    // Opens an input stream for non-wide character text input based on the contents of a UTF-8 file.
    // The BOM will be skipped if it exists.
    // If file_size is not null, it will be set to the size of the file.
    // This function and stream operations throw FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO std::unique_ptr<std::ifstream> OpenTextInputFileStream(InterfaceString file_path, std::streampos* file_size = nullptr);


    // Creates any directories that do not exist.
    // These function throws FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO void CreateDirectories(InterfaceString directory_path);
    CLASS_DECL_ZTOOLSO void CreateDirectoriesForFile(InterfaceString file_path);

    // Opens a file stream for output.
    // Any directories that do not exist will be created.
    // This function throws FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO std::unique_ptr<std::ofstream> OpenOutputFileStream(InterfaceString file_path);

    // Opens a file for output.
    // Any directories that do not exist will be created.
    // This function throws FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO FILE* OpenFileForOutput(InterfaceString file_path);

    // Writes the data to a file.
    // Any directories that do not exist will be created.
    // This function throws FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO void Write(InterfaceString file_path, const std::byte* content, size_t content_size);

    // Writes the data to a file.
    // Any directories that do not exist will be created.
    // This function throws FileIO::Exception exceptions.
    template<typename T>
    void Write(InterfaceString file_path, const T& content)
    {
        static_assert(sizeof(*content.data()) == sizeof(std::byte));
        Write(std::move(file_path), reinterpret_cast<const std::byte*>(content.data()), content.size());
    }

    // Writes the text in UTF-8, with or without a BOM.
    // Any directories that do not exist will be created.
    // This function throws FileIO::Exception exceptions.
    CLASS_DECL_ZTOOLSO void WriteText(InterfaceString file_path, std::string_view text_content_sv, bool write_utf8_bom);
}
