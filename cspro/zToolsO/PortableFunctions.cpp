#include "StdAfx.h"
#include "PortableFunctions.h"
#include "DirectoryLister.h"
#include "FileIO.h"
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <filesystem>
#include <system_error>

extern "C"
{
#include "md5.h"
}

#ifdef WIN32
#include <corecrt_io.h>
#include <sys/utime.h>
#else
#include <utime.h>
#endif


FILE* PortableFunctions::FileOpen(NullTerminatedString filename, NullTerminatedString mode)
{
    FILE* pFile = NULL;

#ifdef WIN32
    bool bSuccess = (_tfopen_s(&pFile, filename.c_str(), mode.c_str()) == 0 && pFile != NULL);

    if (!bSuccess)
    {
        ASSERT(pFile == NULL);
        pFile = NULL;
    }
#else
    pFile = fopen(UTF8Convert::WideToUTF8(filename).c_str(), UTF8Convert::WideToUTF8(mode).c_str());
#endif

    return pFile;
}


FILE* PortableFunctions::FileOpen(NullTerminatedString filename, NullTerminatedString mode, int iShareFlag)
{
#ifdef WIN32
    return _tfsopen(filename.c_str(), mode.c_str(), iShareFlag);
#else
    return FileOpen(filename, mode);
#endif
}


FILE * PortableFunctions::FileReopen(NullTerminatedString filename, NullTerminatedString mode, FILE * pExistingStream)
{
#ifdef WIN32
    FILE* pFile = NULL;
    if (_wfreopen_s(&pFile, filename.c_str(), mode.c_str(), pExistingStream)) {
        return NULL;
    }
    return pFile;
#else
    return freopen(UTF8Convert::WideToUTF8(filename).c_str(), UTF8Convert::WideToUTF8(mode).c_str(), pExistingStream);
#endif
}


bool PortableFunctions::FileRename(NullTerminatedString old_filename, NullTerminatedString new_filename)
{
#ifdef WIN_DESKTOP
    return ::MoveFileEx(old_filename.c_str(), new_filename.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0;

#elif defined(WIN32)
    return _trename(old_filename.c_str(), new_filename.c_str()) == 0;

#else
    bool success = ( std::rename(UTF8Convert::WideToUTF8(old_filename).c_str(), UTF8Convert::WideToUTF8(new_filename).c_str()) == 0 );

    // on Android std::rename failed when moving files out of the cache directory, so try to copy instead
    if( !success && FileCopy(old_filename, new_filename, false) )
        success = PortableFunctions::FileDelete(old_filename);

    return success;

#endif
}


bool PortableFunctions::DirectoryRename(NullTerminatedString old_directory, NullTerminatedString new_directory)
{
#ifdef WIN_DESKTOP
    try
    {
        std::filesystem::rename(old_directory.c_str(), new_directory.c_str());
    }

    catch( const std::filesystem::filesystem_error& )
    {
        return false;
    }

    return true;

#else
    return FileRename(old_directory.c_str(), new_directory.c_str());

#endif
}


bool PortableFunctions::FileCopy(NullTerminatedString old_filename, NullTerminatedString new_filename, bool fail_if_exists)
{
#ifdef WIN_DESKTOP
    return ( CopyFile(old_filename.c_str(), new_filename.c_str(), fail_if_exists) != 0 );

#else
    if (fail_if_exists && PortableFunctions::FileExists(new_filename))
        return false;

    FILE* in = PortableFunctions::FileOpen(old_filename, _T("rb"));

    if (!in)
        return false;

    FILE* out = PortableFunctions::FileOpen(new_filename, _T("wb"));

    if (!out) {
        fclose(in);
        return false;
    }

    constexpr size_t MaxBufferSize = 1024 * 1024;
    size_t buffer_size = std::min(MaxBufferSize, static_cast<size_t>(FileSize(old_filename)));

    auto buffer = std::make_unique<char[]>(buffer_size);

    for( size_t bytes_read; ( bytes_read = fread(buffer.get(), 1, buffer_size, in) ) > 0; )
        fwrite(buffer.get(), 1, bytes_read, out);

    fclose(in);
    fclose(out);

    return true;
#endif
}


void PortableFunctions::FileCopyWithExceptions(NullTerminatedString old_filename, NullTerminatedString new_filename, FileCopyType file_copy_type,
                                               std::tuple<int64_t, time_t>* out_file_size_and_modified_time/* = nullptr*/)
{
    const bool fail_if_exists = ( file_copy_type == FileCopyType::FailIfExists );

    if( PortableFunctions::FileExists(new_filename) )
    {
        if( fail_if_exists )
            throw FileIO::Exception::FileCopyFailDestinationExists(old_filename, new_filename);

        if( file_copy_type == FileCopyType::CopyIfDifferent )
        {
            std::tuple<int64_t, time_t> old_size_and_modified_time = FileSizeAndModifiedTime(old_filename);

            if( std::get<0>(old_size_and_modified_time) != -1 && old_size_and_modified_time == FileSizeAndModifiedTime(new_filename) )
            {
                if( out_file_size_and_modified_time != nullptr )
                    *out_file_size_and_modified_time = old_size_and_modified_time;

                return;
            }
        }
    }

    if( !FileCopy(old_filename, new_filename, fail_if_exists) )
        throw FileIO::Exception::FileCopyFail(old_filename, new_filename);

    if( out_file_size_and_modified_time != nullptr )
        *out_file_size_and_modified_time = FileSizeAndModifiedTime(new_filename);
}


bool PortableFunctions::FileDelete(NullTerminatedString filename)
{
#ifdef WIN32
    return ( DeleteFile(filename.c_str()) != 0 );

#else
    if( FileIsDirectory(filename) )
    {
        // ALW 20180525 Android implementation of std::remove deletes files and directories. Ignore directories.
        return false;
    }

    else
    {
        return ( std::remove(UTF8Convert::WideToUTF8(filename).c_str()) == 0 );
    }

#endif
}

bool PortableFunctions::DirectoryDelete(NullTerminatedString directory, bool delete_all_paths_within_directory/* = false*/)
{
    unsigned errors = 0;

    // delete any files and subdirectories with the directory
    if( delete_all_paths_within_directory )
    {
        for( const auto& path : DirectoryLister(false, true, true).GetPaths(directory) )
        {
            if( PortableFunctions::FileIsDirectory(path) )
            {
                if( !DirectoryDelete(path, true) )
                    ++errors;
            }

            else if( !FileDelete(path) )
            {
                ++errors;
            }
        }
    }

    // delete the directory
#ifdef WIN32
    if( ::RemoveDirectory(directory.c_str()) == 0 )
        ++errors;
#else
    if( rmdir(UTF8Convert::WideToUTF8(directory).c_str()) != 0 )
        ++errors;
#endif

    return ( errors == 0 );
}

bool PortableFunctions::FileTruncate(FILE* pFile,int64_t lFileSize)
{
#ifdef WIN32
    if( fseeki64(pFile,lFileSize,SEEK_SET) == 0 )
    {
        HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(pFile));

        if( hFile != NULL )
            return SetEndOfFile(hFile) != 0 ? true : false;
    }

    return false;

#else
    return ( ftruncate(fileno(pFile),lFileSize) == 0 );

#endif
}

template<typename T/* = int64_t*/>
T PortableFunctions::FileSize(const NullTerminatedString file_path)
{
#ifdef WIN32
    struct _stat64 attrib;
    const int ret = _wstat64(file_path.c_str(), &attrib);
#else
    struct stat attrib;
    const int ret = stat(UTF8Convert::WideToUTF8(file_path).c_str(), &attrib);
#endif

    if( ret == 0 && S_ISREG(attrib.st_mode) )
        return attrib.st_size;

    if constexpr(std::is_same_v<T, int64_t>)
    {
        return -1;
    }

    else
    {
        return std::nullopt;
    }
}

template CLASS_DECL_ZTOOLSO int64_t PortableFunctions::FileSize(NullTerminatedString file_path);
template CLASS_DECL_ZTOOLSO std::optional<uint64_t> PortableFunctions::FileSize(NullTerminatedString file_path);


std::wstring PortableFunctions::FileSizeString(const int64_t file_size)
{
    if( file_size >= 0 )
    {
#ifdef WIN32
        constexpr int SizeLength = 64;
        std::wstring file_size_string;
        file_size_string.resize(SizeLength);

        if( StrFormatByteSize(file_size, file_size_string.data(), SizeLength) != nullptr )
        {
            file_size_string.resize(_tcslen(file_size_string.data()));
            return file_size_string;
        }

#else
        // unimplemented
        return ReturnProgrammingError(CS2WS(IntToString(file_size)));
#endif
    }

    return std::wstring();
}


bool PortableFunctions::FileExists(NullTerminatedString filename)
{
    if( filename.empty() )
        return false;

#ifdef WIN32
    struct _stat64 fstatus;
    return _wstat64(filename.c_str(), &fstatus) == 0;

#else
    filestat fstatus;
    return stat(UTF8Convert::WideToUTF8(filename).c_str(), &fstatus) == 0;

#endif
}


bool PortableFunctions::FileIsRegular(NullTerminatedString filename)
{
    if( filename.empty() )
        return false;

#ifdef WIN32
    struct _stat64 fstatus;
    auto stat_ret = _wstat64(filename.c_str(), &fstatus);

#else
    filestat fstatus;
    auto stat_ret = stat(UTF8Convert::WideToUTF8(filename).c_str(), &fstatus);

#endif

    return ( stat_ret == 0 && S_ISREG(fstatus.st_mode) );
}


bool PortableFunctions::FileIsDirectory(NullTerminatedString filename)
{
    if( filename.empty() )
        return false;

#ifdef WIN32
    // _wstat64 fails on drive letters without trailing / so add
    // trailing / e.g. "C:" => "C:/"
    if( filename.back() == ':' )
        return FileIsDirectory(std::wstring(filename) + _T("/"));

    struct _stat64 st;
    return ( _wstat64(filename.c_str(), &st) == 0 && S_ISDIR(st.st_mode) );

#else
    struct stat st;
    return ( stat(UTF8Convert::WideToUTF8(filename).c_str(), &st) == 0 && S_ISDIR(st.st_mode) );

#endif
}


time_t PortableFunctions::FileModifiedTime(NullTerminatedString filename)
{
#ifdef WIN32
    struct _stat64 attrib;
    _wstat64(filename.c_str(), &attrib);
#else
    struct stat attrib;
    stat(UTF8Convert::WideToUTF8(filename).c_str(), &attrib);
#endif

    return attrib.st_mtime;
}


std::tuple<int64_t, time_t> PortableFunctions::FileSizeAndModifiedTime(NullTerminatedString filename)
{
#ifdef WIN32
    struct _stat64 attrib;
    int ret = _wstat64(filename.c_str(), &attrib);
#else
    struct stat attrib;
    int ret = stat(UTF8Convert::WideToUTF8(filename).c_str(), &attrib);
#endif

    return ( ret == 0 && S_ISREG(attrib.st_mode) ) ? std::tuple<int64_t, time_t>(attrib.st_size, attrib.st_mtime) :
                                                     std::tuple<int64_t, time_t>(-1, 0);
}


bool PortableFunctions::FileTouch(NullTerminatedString filename)
{
#ifdef WIN32
    return ( _wutime(filename.c_str(), NULL) == 0 );
#else
    return ( utime(UTF8Convert::WideToUTF8(filename).c_str(),NULL) == 0 );
#endif
}


#ifdef ANDROID
// With Android 8.0 on Google Pixel mkstemp is crashing with SIGSYS so instead
// we use a custom version of mkstemp adapted from libzip source
// https://github.com/aseprite/libzip/blob/master/lib/mkstemp.c which in
// turn was adapted from NetBSD

#ifndef O_BINARY
#define O_BINARY 0
#endif

static int android_mkstemp(char *path)
{
    int fd;
    char *start, *trv;
    struct stat sbuf;
    pid_t pid;

    /* To guarantee multiple calls generate unique names even if
       the file is not created. 676 different possibilities with 7
       or more X's, 26 with 6 or less. */
    static char xtra[3] = "aa";
    int xcnt = 0;

    pid = getpid();

    /* Move to end of path and count trailing X's. */
    for (trv = path; *trv; ++trv)
        if (*trv == 'X')
            xcnt++;
        else
            xcnt = 0;

    /* Use at least one from xtra.  Use 2 if more than 6 X's. */
    if (*(trv - 1) == 'X')
        *--trv = xtra[0];
    if (xcnt > 6 && *(trv - 1) == 'X')
        *--trv = xtra[1];

    /* Set remaining X's to pid digits with 0's to the left. */
    while (*--trv == 'X') {
        *trv = (pid % 10) + '0';
        pid /= 10;
    }

    /* update xtra for next call. */
    if (xtra[0] != 'z')
        xtra[0]++;
    else {
        xtra[0] = 'a';
        if (xtra[1] != 'z')
            xtra[1]++;
        else
            xtra[1] = 'a';
    }

    /*
     * check the target directory; if you have six X's and it
     * doesn't exist this runs for a *very* long time.
     */
    for (start = trv + 1;; --trv) {
        if (trv <= path)
            break;
        if (*trv == '/') {
            *trv = '\0';
            if (stat(path, &sbuf))
                return (0);
            if (!S_ISDIR(sbuf.st_mode)) {
                errno = ENOTDIR;
                return (0);
            }
            *trv = '/';
            break;
        }
    }

    for (;;) {
        if ((fd=open(path, O_CREAT|O_EXCL|O_RDWR|O_BINARY, 0600)) >= 0)
            return (fd);
        if (errno != EEXIST)
            return (0);

        /* tricky little algorithm for backward compatibility */
        for (trv = start;;) {
            if (!*trv)
                return (0);
            if (*trv == 'z')
                *trv++ = 'a';
            else {
                if (isdigit((unsigned char)*trv))
                    *trv = 'a';
                else
                    ++*trv;
                break;
            }
        }
    }
    /*NOTREACHED*/
}

#endif


CString PortableFunctions::FileTempName(NullTerminatedString directory)
{
    CString tempFilePath;
#ifdef WIN32
    ::GetTempFileName(directory.c_str(), _T("CSPTMP"), 0, tempFilePath.GetBuffer(MAX_PATH));
    tempFilePath.ReleaseBuffer();
#else
    CString pathTemplate = CString(directory) + _T("CSPTMPXXXXXX");
    char *tmpPath = strdup(UTF8Convert::WideToUTF8(pathTemplate).c_str());
#ifdef ANDROID
    android_mkstemp(tmpPath);
#else
    mkstemp(tmpPath);
#endif
    tempFilePath = UTF8Convert::UTF8ToWide(tmpPath);
    free(tmpPath);
#endif // WIN32
    return tempFilePath;
}


std::wstring PortableFunctions::GetUniqueFilenameInDirectory(wstring_view directory_sv, wstring_view extension_sv, const TCHAR* filename_prefix/* = nullptr*/,
                                                             std::function<bool(const std::wstring&)> extra_check_callback/* = { }*/)
{
    static int i = -1;

    if( filename_prefix == nullptr )
        filename_prefix = _T(".CS");

    while( true )
    {
        ++i;

        std::wstring filename = PathAppendToPath<std::wstring>(directory_sv,
                                                               PathAppendFileExtension(SO::Concatenate(filename_prefix, IntToString(i)), extension_sv));

        if( !PortableFunctions::FileExists(filename) )
        {
            if( !extra_check_callback || extra_check_callback(filename) )
                return filename;
        }
    }
}


namespace
{
    std::wstring GenerateMd5(const std::function<bool(MD5_CTX&)>& md5_update_callback)
    {
        MD5_CTX ctx;
        MD5_Init(&ctx);

        do { } while( md5_update_callback(ctx) );

        constexpr size_t HexSequences = 16;

        unsigned char result[HexSequences];
        MD5_Final(result, &ctx);

        std::wstring md5_string(HexSequences * 2, '\0');
        TCHAR* md5_string_buffer = md5_string.data();

        for( size_t i = 0; i < HexSequences; ++i, md5_string_buffer += 2 )
            _sntprintf(md5_string_buffer, 3, _T("%02x"), static_cast<unsigned int>(result[i]));

        return md5_string;
    }
}

std::wstring PortableFunctions::FileMd5(NullTerminatedString filename)
{
    FILE* file = PortableFunctions::FileOpen(filename, _T("rb"));

    if( file == nullptr )
        return std::wstring();

    constexpr size_t BufferSize = 64 * 1024;
    auto buffer = std::make_unique<char[]>(BufferSize);

    std::wstring md5_string;
    class Md5Error { };

    try
    {
        md5_string = GenerateMd5([&](MD5_CTX& ctx) -> bool
        {
            size_t bytes_read = fread(buffer.get(), 1, BufferSize, file);
            MD5_Update(&ctx, buffer.get(), bytes_read);

            if( ferror(file) )
                throw Md5Error();

            return !feof(file);
        });
    }

    catch( const Md5Error& ) { }

    fclose(file);

    return md5_string;
}

std::wstring PortableFunctions::BinaryMd5(const std::byte* contents, size_t size)
{
    return GenerateMd5([&](MD5_CTX& ctx) -> bool
    {
        MD5_Update(&ctx, contents, size);
        return false;
    });
}

std::wstring PortableFunctions::BinaryMd5(const std::vector<std::byte>& contents)
{
    return BinaryMd5(contents.data(), contents.size());
}

std::wstring PortableFunctions::StringMd5(const std::string& s)
{
    return BinaryMd5(reinterpret_cast<const std::byte*>(s.data()), s.length());
}


int64_t PortableFunctions::ftelli64(FILE* stream)
{
#ifdef WIN32
    return _ftelli64(stream);
#else
    return ftello(stream);
#endif
}

int64_t PortableFunctions::fseeki64(FILE* stream, int64_t offset, int origin)
{
#ifdef WIN32
    return _fseeki64(stream, offset, origin);
#else
    return fseeko(stream, offset, origin);
#endif
}


bool PortableFunctions::PathMakeDirectory(NullTerminatedString path)
{
#ifdef WIN32
    return ( CreateDirectory(path.c_str(), NULL) != 0 );
#else
    return ( mkdir(UTF8Convert::WideToUTF8(path).c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0 );
#endif
}


bool PortableFunctions::PathMakeDirectories(NullTerminatedString path)
{
    if( PortableFunctions::FileIsDirectory(path) )
        return true;

    wstring_view path_sv = SO::TrimRight(path, PATH_CHAR);

    size_t pos = path_sv.find(PATH_CHAR, 1);

    while( pos != wstring_view::npos )
    {
        std::wstring directory = path_sv.substr(0, pos + 1);

        if( !PortableFunctions::FileIsDirectory(directory) )
        {
            // ignore result here since in some scenarios
            // we don't have permission to access earlier directories in path
            PortableFunctions::PathMakeDirectory(directory);
        }

        pos = path_sv.find(PATH_CHAR, pos + 1);
    }

    return PortableFunctions::PathMakeDirectory(path);
}


const TCHAR* PortableFunctions::PathGetFilename(NullTerminatedString path)
{
    const TCHAR* path_start = path.c_str();
    const TCHAR* last_path_character = path_start + path.length() - 1;
    const TCHAR* path_itr = last_path_character;

    for( ; path_itr >= path_start; --path_itr )
    {
        if( IsPathCharacter(*path_itr) )
        {
            // ignore the trailing /
            if( path_itr != last_path_character )
                break;
        }
    }

    return path_itr + 1;
}


template<typename T/* = std::wstring*/>
T PortableFunctions::PathGetFilenameWithoutExtension(NullTerminatedString path)
{
    return PortableFunctions::PathRemoveFileExtension<T>(PortableFunctions::PathGetFilename(path));
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathGetFilenameWithoutExtension(NullTerminatedString path);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathGetFilenameWithoutExtension(NullTerminatedString path);


template<typename T/* = std::wstring*/>
T PortableFunctions::PathGetDirectory(const wstring_view path_sv)
{
    size_t path_length = path_sv.length();

    if( path_length != 0 )
    {
        const TCHAR* path_itr = path_sv.data() + path_length - 1;

        do
        {
            if( IsPathCharacter(*path_itr) )
                return T(path_sv.data(), path_length);

             --path_length;
             --path_itr;

        } while( path_length > 0 );
    }

    return T();
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathGetDirectory(wstring_view path_sv);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathGetDirectory(wstring_view path_sv);


namespace
{
    // returns the position of the '.' starting a file extension
    inline const TCHAR* FindFileExtensionStart(const wstring_view path_sv)
    {
        if( !path_sv.empty() )
        {
            const TCHAR* path_itr = path_sv.data() + path_sv.length() - 1;

            while( true )
            {
                if( *path_itr == '.' )
                    return path_itr;

                if( PortableFunctions::IsPathCharacter(*path_itr) || path_itr == path_sv.data() )
                    break;

                --path_itr;
            }
        }

        return nullptr;
    }
}


template<typename T/* = std::wstring*/>
T PortableFunctions::PathRemoveFileExtension(const wstring_view path_sv)
{
    const TCHAR* extension_pos = FindFileExtensionStart(path_sv);

    if( extension_pos == nullptr )
        return path_sv;

    return T(path_sv.data(), extension_pos - path_sv.data());
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathRemoveFileExtension(wstring_view path_sv);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathRemoveFileExtension(wstring_view path_sv);


std::wstring PortableFunctions::PathReplaceFileExtension(const wstring_view path_sv, const wstring_view extension_sv)
{
    return PathAppendFileExtension(PathRemoveFileExtension(path_sv), extension_sv);
}


template<typename T/* = std::wstring*/>
T PortableFunctions::PathGetFileExtension(wstring_view path_sv, const bool include_dot/* = false*/)
{
    const TCHAR* extension_pos = FindFileExtensionStart(path_sv);

    if( extension_pos == nullptr )
        return T();

    if( !include_dot )
        ++extension_pos;

    return T(extension_pos, path_sv.data() + path_sv.length() - extension_pos);
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathGetFileExtension(wstring_view path_sv, bool include_dot/* = false*/);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathGetFileExtension(wstring_view path_sv, bool include_dot/* = false*/);


template<typename T>
T PortableFunctions::PathAppendFileExtension(T filename, const wstring_view extension_sv)
{
    if( !extension_sv.empty() )
    {
        if constexpr(std::is_same_v<T, std::wstring>)
        {
            if( extension_sv.front() != '.' )
                filename.push_back('.');

            filename.append(extension_sv.data(), extension_sv.length());
        }

        else
        {
            if( extension_sv.front() != '.' )
                filename.AppendChar('.');

            filename.Append(extension_sv.data(), extension_sv.length());
        }
    }

    return filename;
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathAppendFileExtension(std::wstring filename, wstring_view extension_sv);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathAppendFileExtension(CString filename, wstring_view extension_sv);


std::wstring PortableFunctions::PathEnsureFileExtension(std::wstring filename, const wstring_view extension_sv)
{
    const bool include_dot = !extension_sv.empty() && extension_sv.front() == '.';
    const std::wstring this_extension = PathGetFileExtension(filename, include_dot);

    return SO::EqualsNoCase(extension_sv, this_extension) ? filename :
                                                            PathAppendFileExtension(std::move(filename), extension_sv);
}


template<typename T>
T PortableFunctions::PathToNativeSlash(T path)
{
    constexpr TCHAR NativeSlash = PATH_CHAR;
    constexpr TCHAR NonNativeSlash = ( NativeSlash == '/' ) ? '\\' : '/';

    if constexpr(std::is_same_v<T, std::wstring>)
    {
        return SO::Replace(path, NonNativeSlash, NativeSlash);
    }

    else
    {
        path.Replace(NonNativeSlash, NativeSlash);
        return path;
    }
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathToNativeSlash(std::wstring path);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathToNativeSlash(CString path);


template<typename T>
T PortableFunctions::PathToForwardSlash(T path)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        return SO::Replace(path, '\\', '/');
    }

    else
    {
        path.Replace('\\', '/');
        return path;
    }
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathToForwardSlash(std::wstring path);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathToForwardSlash(CString path);


template<typename T>
T PortableFunctions::PathToBackwardSlash(T path)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        return SO::Replace(path, '/', '\\');
    }

    else
    {
        path.Replace('/', '\\');
        return path;
    }
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathToBackwardSlash(std::wstring path);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathToBackwardSlash(CString path);


template<typename T>
T PortableFunctions::PathAppendToPath(T path, wstring_view append_text_sv, const TCHAR separator/* = PATH_CHAR*/)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        if( path.empty() )
            return append_text_sv;

        const int separators_used = ( IsPathCharacter(path.back()) ? 1 : 0 ) +
                                    ( ( !append_text_sv.empty() && IsPathCharacter(append_text_sv.front()) ) ? 1 : 0 );

        // eliminate an extra separator
        if( separators_used == 2 )
        {
            append_text_sv = append_text_sv.substr(1);
        }

        // or add a missing separator
        else if( separators_used == 0 )
        {
            path.push_back(separator);
        }

        path.append(append_text_sv);

        return path;
    }

    else
    {
        if( path.IsEmpty() )
            return append_text_sv;

        const int separators_used = ( IsPathCharacter(path[path.GetLength() - 1]) ? 1 : 0 ) +
                                    ( ( !append_text_sv.empty() && IsPathCharacter(append_text_sv.front()) ) ? 1 : 0 );

        // eliminate an extra separator
        if( separators_used == 2 )
        {
            append_text_sv = append_text_sv.substr(1);
        }

        // or add a missing separator
        else if( separators_used == 0 )
        {
            path.AppendChar(separator);
        }

        path.Append(append_text_sv.data(), append_text_sv.length());

        return path;
    }
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathAppendToPath(std::wstring path, wstring_view append_text_sv, TCHAR separator);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathAppendToPath(CString path, wstring_view append_text_sv, TCHAR separator);


template<typename T/* = std::wstring*/>
T PortableFunctions::PathRemoveTrailingSlash(const wstring_view path_sv)
{
    return SO::TrimRight(path_sv, PathSlashChars);
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathRemoveTrailingSlash(wstring_view path_sv);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathRemoveTrailingSlash(wstring_view path_sv);


template<typename T>
T PortableFunctions::PathEnsureTrailingSlash(T path, const TCHAR separator/* = PATH_CHAR*/)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        if( !path.empty() && path.back() != separator )
            path.push_back(separator);
    }

    else
    {
        if( !path.IsEmpty() && path[path.GetLength() - 1] != separator )
            path.AppendChar(separator);
    }

    return path;
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathEnsureTrailingSlash(std::wstring path, TCHAR separator);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathEnsureTrailingSlash(CString path, TCHAR separator);


std::wstring PortableFunctions::PathGetCommonRoot(const wstring_view path1_sv, const wstring_view path2_sv, const TCHAR separator/* = PATH_CHAR*/)
{
    static_assert(( std::wstring_view::npos + 1 ) == 0);

    size_t last_separator;
    size_t next_separator = std::wstring_view::npos;

    while( true )
    {
        last_separator = next_separator;
        next_separator = path1_sv.find(separator, last_separator + 1);

        if( next_separator >= path2_sv.length() || path2_sv[next_separator] != separator )
            break;

        const size_t this_component_start_pos = last_separator + 1;
        const size_t this_component_length = next_separator - this_component_start_pos;

        if( path1_sv.substr(this_component_start_pos, this_component_length) != path2_sv.substr(this_component_start_pos, this_component_length) )
            break;
    }

    if( last_separator != std::wstring_view::npos )
        return path1_sv.substr(0, last_separator + 1);

    return std::wstring();
}


std::string PortableFunctions::TimeToString(const time_t t)
{
    char buff[20];
    strftime(buff, 20, "%Y-%m-%d %H:%M:%S", localtime(&t));
    return std::string(buff);
}


CString PortableFunctions::TimeToRFC3339String(const time_t t)
{
    const std::tm* ptm = gmtime(&t);

    return FormatText(_T("%04d-%02d-%02dT%02d:%02d:%02dZ"),
                      ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
                      ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
}


time_t PortableFunctions::ParseRFC3339DateTime(std::wstring date_time)
{
    if( date_time.length() < MinLengthRFC3339DateTimeString )
        return 0;

    // Figure out the timezone offset which will either
    // be +HH:MM, -HH:MM, Z or z (z/Z is for UTC).
    // Where it starts depends on whether or not the time
    // includes fractional seconds (.sss) before the timezone
    // offset.
    int hoursOffset = 0;
    int minsOffset = 0;
    wchar_t* buffer = date_time.data();
    wchar_t* pZoneStart = buffer + 19;
    while (*pZoneStart != '\0' &&
           *pZoneStart != '+' &&
           *pZoneStart != '-' &&
           *pZoneStart != 'Z' &&
           *pZoneStart != 'z')
    {
        ++pZoneStart;
    }

    if (*pZoneStart != '\0' && (*pZoneStart == '+' || *pZoneStart == '-')) {
        if (buffer - pZoneStart >= 4) {
            pZoneStart[3] = '\0';
            hoursOffset = _wtoi(pZoneStart + 1);
            minsOffset = _wtoi(pZoneStart + 4);
            if (*pZoneStart == '-') {
                hoursOffset *= -1;
                minsOffset *= -1;
            }
        }
    }

    // Put in nulls at separators so each component can be
    // treated as a separate string
    buffer[4] = '\0';
    buffer[7] = '\0';
    buffer[10] = '\0';
    buffer[13] = '\0';
    buffer[16] = '\0';
    buffer[19] = '\0';

    tm timeStruct;

    timeStruct.tm_year = _wtoi(buffer) - 1900;
    timeStruct.tm_mon = _wtoi(buffer + 5) - 1;
    timeStruct.tm_mday = _wtoi(buffer + 8);
    timeStruct.tm_hour = _wtoi(buffer + 11);
    timeStruct.tm_min = _wtoi(buffer + 14);
    timeStruct.tm_sec = _wtoi(buffer + 17);
    timeStruct.tm_isdst = 0;
    timeStruct.tm_wday = 0;
    timeStruct.tm_yday = 0;

    time_t unixTime = _mkgmtime(&timeStruct);

    unixTime += minsOffset * 60 + hoursOffset * 60 * 60;

    return unixTime;
}


time_t PortableFunctions::ParseYYYYMMDDhhmmssDateTime(std::wstring date_time)
{
    if( date_time.size() < 14 )
        return 0;

    // Make a copy so we can modify string
    wchar_t* buffer = date_time.data();
    tm timeStruct;

    timeStruct.tm_sec = _wtoi(buffer + 12);
    buffer[12] = 0;
    timeStruct.tm_min = _wtoi(buffer + 10);
    buffer[10] = 0;
    timeStruct.tm_hour = _wtoi(buffer + 8);
    buffer[8] = 0;
    timeStruct.tm_mday = _wtoi(buffer + 6);
    buffer[6] = 0;
    timeStruct.tm_mon = _wtoi(buffer + 4) - 1;
    buffer[4] = 0;
    timeStruct.tm_year = _wtoi(buffer) - 1900;
    timeStruct.tm_isdst = 0;
    timeStruct.tm_wday = 0;
    timeStruct.tm_yday = 0;

    return _mkgmtime(&timeStruct);
}
