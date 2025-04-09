#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/Path.h>

// some functions here will eventually be moved to the Path class ... see PATH_TODO


enum class FileOverwriteFlag : int
{
    Always,   // the destination file is always overwritten
    Never,    // the destination file is never overwritten (skipped)
    Fail,     // an exception is thrown if the destination file exists
    Different // the destination file is overwritten when different from the source file
};


namespace PortableFunctions
{
    // Opens a file on the disk, returning null on error.
    CLASS_DECL_ZTOOLSO FILE* FileOpen(InterfaceString file_path, InterfaceString mode, int share_flag = INT_MIN);

    // Renames a file on the disk.
    // Returns true on success.
    CLASS_DECL_ZTOOLSO bool FileRename(InterfaceString old_file_path, InterfaceString new_file_path);

    // Renames a file on the disk. If the file cannot be renamed, an exception is thrown
    CLASS_DECL_ZTOOLSO void FileRenameWithExceptions(InterfaceString old_file_path, InterfaceString new_file_path);

    // Renames a directory on the disk.
    // Returns true on success.
    CLASS_DECL_ZTOOLSO bool DirectoryRename(InterfaceString old_directory, InterfaceString new_directory);

    // Copies a file on the disk.
    // Returns true on success.
    CLASS_DECL_ZTOOLSO bool FileCopy(InterfaceString old_file_path, InterfaceString new_file_path, bool fail_if_exists);

    // Copies a file on the disk, throwing exceptions on error.
    // If file_overwrite_flag is FileOverwriteFlag::Different, the file is copied if the file size or modified date differs.
    // The return value indicates if a file was actually copied.
    CLASS_DECL_ZTOOLSO bool FileCopyWithExceptions(InterfaceString old_file_path, InterfaceString new_file_path, FileOverwriteFlag file_overwrite_flag,
                                                   std::tuple<int64_t, int64_t>* out_file_size_and_modified_time = nullptr);

    // Deletes a file from the disk.
    // Returns true on success.
    CLASS_DECL_ZTOOLSO bool FileDelete(InterfaceString file_path);

    // Deletes a file on the disk. If the file exists but cannot be deleted, an exception is thrown.
    CLASS_DECL_ZTOOLSO void FileDeleteWithExceptions(InterfaceString file_path);

    // Deletes a directory from the disk, with an option to delete all files and subdirectories within the directory.
    // Returns true on success.
    CLASS_DECL_ZTOOLSO bool DirectoryDelete(InterfaceString directory, bool delete_all_paths_within_directory = false);

    ///<summary>Truncate file on disk</summary>
    ///Returns true on success.
    CLASS_DECL_ZTOOLSO bool FileTruncate(FILE* pFile, int64_t lFileSize);

    // Returns the size of the file in bytes.
    // By default, this returns int64_t, and -1 if unable to read the file.
    // It can also return std::optional<uint64_t>, and std::nullopt if unable to read the file.
    template<typename T = int64_t>
    CLASS_DECL_ZTOOLSO T FileSize(InterfaceString file_path);

    // Returns a text representation of the file size, or a blank string if unable to read the file.
    CLASS_DECL_ZTOOLSO std::string FileSizeString(int64_t file_size);
    inline std::string FileSizeString(InterfaceString file_path) { return FileSizeString(FileSize(file_path)); }

    // Returns true if the file exists (as a file or directory).
    CLASS_DECL_ZTOOLSO bool FileExists(InterfaceString path);

    // Returns true if the file exists and is not a directory.
    CLASS_DECL_ZTOOLSO bool FileIsRegular(InterfaceString path);

    // Returns true if the file exists and is a directory.
    CLASS_DECL_ZTOOLSO bool FileIsDirectory(InterfaceString path);

    // Returns the last modified date/time of a file. The time will be 0 if unable to read file.
    template<bool ThrowExceptionOnError = false>
    CLASS_DECL_ZTOOLSO int64_t FileModifiedTime(InterfaceString file_path);

    // Returns the size in bytes and the last modified date/time of a file. The size will be -1 if unable to read file.
    CLASS_DECL_ZTOOLSO std::tuple<int64_t, int64_t> FileSizeAndModifiedTime(InterfaceString file_path);

    // Touches the file with the current time.
    CLASS_DECL_ZTOOLSO bool FileTouch(InterfaceString file_path);

    // Returns the full path of a file with a unique name in the given directory.
    // Depending on the platform this may create an empty file on disk (to prevent another process from using the same name)
    // or may just generate a unique file name.
    CLASS_DECL_ZTOOLSO std::string FileTempPath(const std::string& directory_path);

    // Returns the full path of a file with a unique name in the given directory and with the specified extension.
    // The extension can be provided with or without a dot.
    // Unlike FileTempPath, this does not create an empty file on the disk, but all calls to the function
    // will return a filename that was not previously returned by this function.
    // If a callback is specified, the function should return true if the name is unique.
    CLASS_DECL_ZTOOLSO std::string GetUniqueFilePathInDirectory(std::string_view directory_sv, std::string_view extension_sv, const char* filename_prefix = nullptr,
                                                                std::function<bool(const std::string&)> uniqueness_check_callback = { });

    // Returns the "command-line string for the current process," or a blank string in the portable environments.
    CLASS_DECL_ZTOOLSO std::string GetCommandLine();

    // Returns MD5 Message-Digest (RFC 1321) of a file.
    CLASS_DECL_ZTOOLSO std::string FileMd5(InterfaceString file_path);

    // Returns MD5 Message-Digest (RFC 1321) of a block of memory.
    CLASS_DECL_ZTOOLSO std::string BinaryMd5(const std::byte* contents, size_t size);
    inline std::string BinaryMd5(const std::vector<std::byte>& contents) { return BinaryMd5(contents.data(), contents.size()); }
    inline std::string BinaryMd5(const BinaryBlock& contents)            { return BinaryMd5(contents.data(), contents.size()); }

    // Returns MD5 Message-Digest (RFC 1321) of a string.
    CLASS_DECL_ZTOOLSO std::string StringMd5(std::string_view text_sv);

    ///<summary>Get current file position as offset from file start in bytes. 64 bit version of ftell.</summary>
    CLASS_DECL_ZTOOLSO int64_t ftelli64(FILE* stream);

    // Set file position as offset in bytes from origin (SEEK_CUR, SEEK_END, SEEK_SET). 64 bit version of fseek.
    CLASS_DECL_ZTOOLSO int fseeki64(FILE* stream, int64_t offset, int origin);

    // Creates a directory. The parent directories already must exist.
    CLASS_DECL_ZTOOLSO bool PathMakeDirectory(InterfaceString directory_path);

    // Recursively make directories to ensure that path exists.
    CLASS_DECL_ZTOOLSO bool PathMakeDirectories(InterfaceString directory_path);

    // Extracts the filename from a path (removing the directory). A trailing slash is ignored.
    // Works with both / and \ (unlike Windows PathRemoveFileSpec).
    // PATH_TODO instead of using PathGetFilename, use Path::GetFilename.
    inline std::string PathGetFilename(std::string_view path_sv) { return Path::GetFilename(path_sv); }
    CLASS_DECL_ZTOOLSO const TCHAR* PathGetFilename(NullTerminatedString path);

    // Extracts the directory from a path (removing the filename). The trailing slash is retained.
    // Works with both / and \ (unlike Windows PathStripPath).
    CLASS_DECL_ZTOOLSO std::string PathGetDirectory(std::string_view path_sv);
    template<typename T = std::wstring>
    CLASS_DECL_ZTOOLSO T PathGetDirectory(wstring_view path_sv);

    // Strips the extension from the path.
    // PATH_TODO instead of using PathRemoveFileExtension, use Path::RemoveExtension.
    inline std::string PathRemoveFileExtension(std::string_view path_sv) { return Path::RemoveExtension(path_sv); }
    inline std::wstring PathRemoveFileExtension(wstring_view path_sv) { return UTF8_TODO::GetWide(PathRemoveFileExtension(UTF8_TODO::GetUtf8(path_sv))); }
    inline CString PathRemoveFileExtensionCS(wstring_view path_sv) { return UTF8_TODO::GetCString(PathRemoveFileExtension(UTF8_TODO::GetUtf8(path_sv))); }

    // Strips the extension from the path and then appends the new extension.
    // The extension can be provided with or without a dot.
    // PATH_TODO instead of using PathReplaceFileExtension, use Path::ReplaceExtension.
    inline std::string PathReplaceFileExtension(std::string_view path_sv, std::string_view extension_sv) { return Path::ReplaceExtension(path_sv, extension_sv); }
    inline std::wstring PathReplaceFileExtension(wstring_view path_sv, wstring_view extension_sv) { return UTF8_TODO::GetWide(PathReplaceFileExtension(UTF8_TODO::GetUtf8(path_sv), UTF8_TODO::GetUtf8(extension_sv))); }

    // Strips the filename from the path and then appends the new filename.
    CLASS_DECL_ZTOOLSO std::string PathReplaceFilename(std::string_view path_sv, std::string_view filename_sv);

    // Returns the extension, extracted from the path.
    // PATH_TODO instead of using PathGetFileExtension, use Path::GetExtension.
    inline std::string PathGetFileExtension(std::string_view path_sv, bool include_dot = false) { return Path::GetExtension(path_sv, include_dot); }
    inline std::wstring PathGetFileExtension(wstring_view path_sv, bool include_dot = false) { return UTF8_TODO::GetWide(PathGetFileExtension(UTF8_TODO::GetUtf8(path_sv), include_dot)); }

    // Adds the extension to a file path.
    // The extension can be provided with or without a dot.
    // PATH_TODO instead of using MakePathAppendFileExtension / PathAppendFileExtension, use Path::MakeAppendExtension / Path::AppendExtension.
    inline std::string& MakePathAppendFileExtension(std::string& file_path, std::string_view extension_sv) { return Path::MakeAppendExtension(file_path, extension_sv); }
    inline std::string PathAppendFileExtension(std::string file_path, std::string_view extension_sv) { return Path::AppendExtension(file_path, extension_sv); }
    template<typename T>
    CLASS_DECL_ZTOOLSO T PathAppendFileExtension(T filename, wstring_view extension_sv);

    // Ensures that the file path has the supplied extension.
    // The extension can be provided with or without a dot.
    CLASS_DECL_ZTOOLSO std::string PathEnsureFileExtension(std::string file_path, std::string_view extension_sv);

    // Creates a file path from a directory, a filename, and an optional extension.
    // The extension can be provided with or without a dot.
    CLASS_DECL_ZTOOLSO std::string CreateFilePath(std::string directory_path, std::string_view filename_sv, std::string_view extension_sv = std::string_view());

    // Convert all / and \ in path to native slashes (\ on Windows, / on Android).
    // PATH_TODO instead of using MakePathToNativeSlash / PathToNativeSlash, use Path::MakeToNativeSlash / Path::ToNativeSlash.
    inline std::string& MakePathToNativeSlash(std::string& path) { return Path::MakeToNativeSlash(path); }
    inline std::string PathToNativeSlash(std::string path) { return Path::ToNativeSlash(path); }
    inline std::string PathToNativeSlash(const char* path) { return Path::ToNativeSlash(std::string(path)); } // UTF8_TODO remove when the wide versions are gone
    CLASS_DECL_ZTOOLSO std::wstring PathToNativeSlash(std::wstring path);
    CLASS_DECL_ZTOOLSO CString PathToNativeSlash(CString path);

    // Converts all backward slashes to forward slashes: \ -> /.
    // PATH_TODO instead of using MakePathToForwardSlash / PathToForwardSlash, use Path::MakeToForwardSlash / Path::ToForwardSlash.
    inline std::string& MakePathToForwardSlash(std::string& path) { return Path::MakeToForwardSlash(path); }
    inline std::string PathToForwardSlash(std::string path)       { return Path::ToForwardSlash(std::move(path)); }

    inline std::wstring PathToForwardSlash(std::wstring path) { return SO::Replace(path, '\\', '/'); } // UTF8_TODO remove when the wide versions are gone
    inline CString PathToForwardSlash(CString path)           { path.Replace('\\', '/'); return path; }

    // Converts all forward slashes to backslashes: / -> \.
    inline std::string PathToBackwardSlash(std::string path) { return SO::Replace(path, '/', '\\'); }

    // PATH_TODO instead of using PathAppendToPath, use Path::Combine.
    template<typename T>
    CLASS_DECL_ZTOOLSO T PathAppendToPath(T path, wstring_view append_text_sv, TCHAR separator = PATH_CHAR);

    // Appends text to a path using a forward slash, making sure to avoid duplicate slashes.
    // PATH_TODO instead of using PathAppendForwardSlashToPath, use Path::CombineForwardSlash.
    inline std::string PathAppendForwardSlashToPath(std::string path, std::string_view append_text_sv) { return Path::Combine<'/'>(std::move(path), append_text_sv); }

    template<typename T>
    T PathAppendForwardSlashToPath(T path, wstring_view append_text_sv) { return PathAppendToPath<T>(std::move(path), append_text_sv, '/'); }

    // Removes a trailing slash from the path if there is one. Removes either / or \.
    CLASS_DECL_ZTOOLSO std::string PathRemoveTrailingSlash(std::string path);

    template<typename T = std::wstring>
    CLASS_DECL_ZTOOLSO T PathRemoveTrailingSlash(wstring_view path_sv);

    // Adds a trailing slash to the path if there isn't already one.
    template<typename T>
    CLASS_DECL_ZTOOLSO T PathEnsureTrailingSlash(T path, char separator = Path::NativeSlashChar);

    // Adds a trailing forward slash to the path if there isn't already one.
    template<typename T>
    auto PathEnsureTrailingForwardSlash(T&& path) { return PathEnsureTrailingSlash(std::forward<T>(path), '/'); }

    constexpr size_t MinLengthRFC3339DateTimeString = 20;

    // Convert time in RFC 3339 format (YYYY-MM-DDTHH:MM:SSZ) to timestamp (seconds since 1/1/1970).
    CLASS_DECL_ZTOOLSO int64_t ParseRFC3339DateTime(std::string date_time);

    ///<summary>Convert time in YYYYMMDDhhmmss format to timestamp (seconds since 1/1/1970)</summary>
    CLASS_DECL_ZTOOLSO int64_t ParseYYYYMMDDhhmmssDateTime(std::string date_time);
}
