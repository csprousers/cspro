#pragma once

#include <zToolsO/zToolsO.h>
#include <stdint.h>


namespace PortableFunctions
{
    CLASS_DECL_ZTOOLSO FILE* FileOpen(NullTerminatedString filename, NullTerminatedString mode);
    CLASS_DECL_ZTOOLSO FILE* FileOpen(NullTerminatedString filename, NullTerminatedString mode, int iShareFlag);

    CLASS_DECL_ZTOOLSO FILE* FileReopen(NullTerminatedString filename, NullTerminatedString mode, FILE* pExistingStream);

    ///<summary>Rename file on disk</summary>
    ///Returns true on success.
    CLASS_DECL_ZTOOLSO bool FileRename(NullTerminatedString old_filename, NullTerminatedString new_filename);

    ///<summary>Rename directory on disk</summary>
    ///Returns true on success.
    CLASS_DECL_ZTOOLSO bool DirectoryRename(NullTerminatedString old_directory, NullTerminatedString new_directory);

    ///<summary>Copy file on disk</summary>
    ///Returns true on success.
    CLASS_DECL_ZTOOLSO bool FileCopy(NullTerminatedString old_filename, NullTerminatedString new_filename, bool fail_if_exists);

    enum class FileCopyType { FailIfExists, CopyIfDifferent, AlwaysCopy };

    ///<summary>Copy file on disk, throwing exceptions on error. "CopyIfDifferent" checks the file size and modified date.</summary>
    CLASS_DECL_ZTOOLSO void FileCopyWithExceptions(NullTerminatedString old_filename, NullTerminatedString new_filename, FileCopyType file_copy_type,
                                                   std::tuple<int64_t, time_t>* out_file_size_and_modified_time = nullptr);

    ///<summary>Delete file from disk</summary>
    ///Returns true on success.
    CLASS_DECL_ZTOOLSO bool FileDelete(NullTerminatedString filename);

    ///<summary>
    ///Deletes a directory from the disk, with an option to delete all files
    ///and subdirectories within the directory.
    ///</summary>
    ///Returns true on success.
    CLASS_DECL_ZTOOLSO bool DirectoryDelete(NullTerminatedString directory, bool delete_all_paths_within_directory = false);

    ///<summary>Truncate file on disk</summary>
    ///Returns true on success.
    CLASS_DECL_ZTOOLSO bool FileTruncate(FILE* pFile, int64_t lFileSize);

    // Returns the size of the file in bytes.
    // By default, this returns int64_t, and -1 if unable to read the file.
    // It can also return std::optional<uint64_t>, and std::nullopt if unable to read the file.
    template<typename T = int64_t>
    CLASS_DECL_ZTOOLSO T FileSize(NullTerminatedString file_path);

    ///<summary>Returns a text representation of the file size, or a blank string if unable to read the file.</summary>
    CLASS_DECL_ZTOOLSO std::wstring FileSizeString(int64_t file_size);
    inline std::wstring FileSizeString(const std::wstring& filename) { return FileSizeString(FileSize(filename)); }

    ///<summary>Returns true if file exists (as a file or directory).</summary>
    CLASS_DECL_ZTOOLSO bool FileExists(NullTerminatedString filename);

    ///<summary>Returns true if file exists and is not a directory.</summary>
    CLASS_DECL_ZTOOLSO bool FileIsRegular(NullTerminatedString filename);

    ///<summary>Returns true if file exists and is a directory.</summary>
    CLASS_DECL_ZTOOLSO bool FileIsDirectory(NullTerminatedString filename);

    ///<summary>Returns last modified date/time of file.</summary>
    CLASS_DECL_ZTOOLSO time_t FileModifiedTime(NullTerminatedString filename);

    ///<summary>Returns size in bytes and last modified date/time of file. The size will be -1 if unable to read file.</summary>
    CLASS_DECL_ZTOOLSO std::tuple<int64_t, time_t> FileSizeAndModifiedTime(NullTerminatedString filename);

    ///<summary>Touches the file with the current time.</summary>
    CLASS_DECL_ZTOOLSO bool FileTouch(NullTerminatedString filename);

    ///<summary>Returns full path of file with unique name in given directory</summary>
    /// Depending on the platform this may create an empty file
    /// on disk (to prevent another process from using the same name)
    /// or may just generate a unique file name.
    CLASS_DECL_ZTOOLSO CString FileTempName(NullTerminatedString directory);

    ///<summary>Returns the full path of a file with a unique name in the given directory and with the specified extension.
    /// Unlike the above function, this does not create an empty file on the disk, but all calls to the function
    /// will return a filename that was not previously returned by this function.
    /// If a callback is specified, the function should return true if the name is unique.</summary>
    CLASS_DECL_ZTOOLSO std::wstring GetUniqueFilenameInDirectory(wstring_view directory_sv, wstring_view extension_sv, const TCHAR* filename_prefix = nullptr,
                                                                 std::function<bool(const std::wstring&)> uniqueness_check_callback = { });
     
    ///<summary>Returns MD5 Message-Digest (RFC 1321) of a file.</summary>
    CLASS_DECL_ZTOOLSO std::wstring FileMd5(NullTerminatedString filename);

    ///<summary>Returns MD5 Message-Digest (RFC 1321) of a block of memory.</summary>
    CLASS_DECL_ZTOOLSO std::wstring BinaryMd5(const std::byte* contents, size_t size);

    ///<summary>Returns MD5 Message-Digest (RFC 1321) of a block of memory.</summary>
    CLASS_DECL_ZTOOLSO std::wstring BinaryMd5(const std::vector<std::byte>& contents);

    ///<summary>Returns MD5 Message-Digest (RFC 1321) of a string.</summary>
    CLASS_DECL_ZTOOLSO std::wstring StringMd5(const std::string& s);

    ///<summary>Get current file position as offset from file start in bytes. 64 bit version of ftell.</summary>
    CLASS_DECL_ZTOOLSO int64_t ftelli64(FILE* stream);

    ///<summary>Set file position as offset in bytes from origin (SEEK_CUR, SEEK_END, SEEK_SET). 64 bit version of fseek.</summary>
    CLASS_DECL_ZTOOLSO int64_t fseeki64(FILE* stream, int64_t offset, int origin);

    ///<summary>Create a directory. Parent directories must exist.</summary>
    CLASS_DECL_ZTOOLSO bool PathMakeDirectory(NullTerminatedString path);

    ///<summary>Recursively make directories to ensure that path exists</summary>
    CLASS_DECL_ZTOOLSO bool PathMakeDirectories(NullTerminatedString path);

    ///<summary>Extract filename from path (remove directory). A trailing slash is ignored.</summary>
    /// Works with both / and \ (unlike Windows PathRemoveFileSpec).
    CLASS_DECL_ZTOOLSO const TCHAR* PathGetFilename(NullTerminatedString path);

    ///<summary>Extract filename from path (remove directory) and remove extension. A trailing slash is ignored.</summary>
    template<typename T = std::wstring>
    CLASS_DECL_ZTOOLSO T PathGetFilenameWithoutExtension(NullTerminatedString path);

    ///<summary>Extract directory from path (remove filename). The trailing slash is retained.</summary>
    /// Works with both / and \ (unlike Windows PathStripPath).
    template<typename T = std::wstring>
    CLASS_DECL_ZTOOLSO T PathGetDirectory(wstring_view path_sv);

    // Strips the extension from the path.
    template<typename T = std::wstring>
    CLASS_DECL_ZTOOLSO T PathRemoveFileExtension(wstring_view path_sv);

    // Strips the extension from the path and then appends the new extension.
    CLASS_DECL_ZTOOLSO std::wstring PathReplaceFileExtension(wstring_view path_sv, wstring_view extension_sv);

    ///<summary>Extract extension from path.</summary>
    template<typename T = std::wstring>
    CLASS_DECL_ZTOOLSO T PathGetFileExtension(wstring_view path_sv, bool include_dot = false);

    ///<summary>Adds an extension to a filename. The extension can be provided with or without a dot.</summary>
    template<typename T>
    CLASS_DECL_ZTOOLSO T PathAppendFileExtension(T filename, wstring_view extension_sv);

    ///<summary>Ensures that the filename has the supplied extension. The extension can be provided with or without a dot.</summary>
    CLASS_DECL_ZTOOLSO std::wstring PathEnsureFileExtension(std::wstring filename, wstring_view extension_sv);

    constexpr wstring_view PathSlashChars = _T("/\\");

    ///<summary>Returns whether a character is a path character (either / or \).</summary>
    inline bool IsPathCharacter(TCHAR ch)
    {
        return ( ch == '/' || ch == '\\' );
    }

    ///<summary>Convert all / and \ in path to native slashes (\ on Windows, / on Android).</summary>
    template<typename T>
    CLASS_DECL_ZTOOLSO T PathToNativeSlash(T path);

    ///<summary>Convert all / and \ in path to /.</summary>
    template<typename T>
    CLASS_DECL_ZTOOLSO T PathToForwardSlash(T path);

    ///<summary>Convert all \ and / in path to /.</summary>
    template<typename T>
    CLASS_DECL_ZTOOLSO T PathToBackwardSlash(T path);

    ///<summary>Append to path, making sure to avoid duplicate slashes.</summary>
    template<typename T>
    CLASS_DECL_ZTOOLSO T PathAppendToPath(T path, wstring_view append_text_sv, TCHAR separator = PATH_CHAR);

    ///<summary>Append forward slash to path, making sure to avoid duplicate slashes.</summary>
    template<typename T>
    T PathAppendForwardSlashToPath(T path, wstring_view append_text_sv) { return PathAppendToPath<T>(std::move(path), append_text_sv, '/'); }

    ///<summary>Remove trailing slash from path if there is one. Removes either / or\.</summary>
    template<typename T = std::wstring>
    CLASS_DECL_ZTOOLSO T PathRemoveTrailingSlash(wstring_view path_sv);

    // Adds a trailing slash to the path if there isn't already one.
    template<typename T>
    CLASS_DECL_ZTOOLSO T PathEnsureTrailingSlash(T path, TCHAR separator = PATH_CHAR);

    // Adds a trailing forward slash to the path if there isn't already one.
    template<typename T>
    auto PathEnsureTrailingForwardSlash(T&& path) { return PathEnsureTrailingSlash(std::forward<T>(path), '/'); }

    // Return shared common start path of two paths or empty string if the two paths are on different volumes.
    CLASS_DECL_ZTOOLSO std::wstring PathGetCommonRoot(wstring_view path1_sv, wstring_view path2_sv, TCHAR separator = PATH_CHAR);

    ///<summary>Format time value as YYYY-MM-DD HH:MM:SS.</summary>
    CLASS_DECL_ZTOOLSO std::string TimeToString(time_t t);

    ///<summary>Format time value in RFC 3339 format (YYYY-MM-DDTHH:MM:SSZ).</summary>
    CLASS_DECL_ZTOOLSO CString TimeToRFC3339String(time_t t);

    constexpr size_t MinLengthRFC3339DateTimeString = 20;

    ///<summary>Convert time in RFC 3339 format (YYYY-MM-DDTHH:MM:SSZ) to timestamp (seconds since 1/1/1970)</summary>
    CLASS_DECL_ZTOOLSO time_t ParseRFC3339DateTime(std::wstring date_time);

    ///<summary>Convert time in YYYYMMDDhhmmss format to timestamp (seconds since 1/1/1970)</summary>
    CLASS_DECL_ZTOOLSO time_t ParseYYYYMMDDhhmmssDateTime(std::wstring date_time);
}


#ifndef WIN32
    #define _SH_DENYRW -1
    #define _SH_DENYNO -1
#endif

#ifdef WIN32
// From sys/stat.h
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#endif
