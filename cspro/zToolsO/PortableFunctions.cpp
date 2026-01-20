#include "StdAfx.h"
#include "PortableFunctions.h"
#include "DirectoryLister.h"
#include "File.h"
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
// From sys/stat.h
#define S_ISREG(m) (((m) & S_IFMT) == S_IFREG)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)

#else
#include <utime.h>
#endif


FILE* PortableFunctions::FileOpen(const InterfaceString file_path, const InterfaceString mode, const int share_flag/* = INT_MIN*/)
{
#ifdef WIN32
    if( share_flag == INT_MIN )
    {
        FILE* file;

        return ( _tfopen_s(&file, file_path.c_str(), mode.c_str()) == 0 ) ? file :
                                                                            nullptr;
    }

    else
    {
        return _tfsopen(file_path.c_str(), mode.c_str(), share_flag);
    }


#else
    return fopen(file_path.c_str(), mode.c_str());

#endif
}


bool PortableFunctions::FileRename(const InterfaceString old_file_path, const InterfaceString new_file_path)
{
#ifdef WIN_DESKTOP
    return ( MoveFileEx(old_file_path.c_str(), new_file_path.c_str(),
                        MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED) != 0 );

#elif defined(WIN32)
    return ( _trename(old_file_path.c_str(), new_file_path.c_str()) == 0 );

#else
    bool success = ( std::rename(old_file_path.c_str(), new_file_path.c_str()) == 0 );

    // on Android std::rename failed when moving files out of the cache directory, so try to copy instead
    if( !success && FileCopy(old_file_path, new_file_path, false) )
        success = PortableFunctions::FileDelete(old_file_path);

    return success;

#endif
}


void PortableFunctions::FileRenameWithExceptions(const InterfaceString old_file_path, const InterfaceString new_file_path)
{
    if( !FileRename(old_file_path, new_file_path) )
        throw FileIO::Exception::FileMoveFail(old_file_path, new_file_path);
}


bool PortableFunctions::DirectoryRename(const InterfaceString old_directory, const InterfaceString new_directory)
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


bool PortableFunctions::FileCopy(const InterfaceString old_file_path, const InterfaceString new_file_path, const bool fail_if_exists)
{
#ifdef WIN_DESKTOP
    return ( CopyFile(old_file_path.c_str(), new_file_path.c_str(), fail_if_exists) != 0 );

#else
    const int64_t in_file_size = FileSize(old_file_path);

    if( in_file_size < 0 )
        return false;

    if( fail_if_exists && PortableFunctions::FileExists(new_file_path) )
        return false;

    // copy the file
    try
    {
        constexpr size_t MaxBufferSize = 1024 * 1024;
        const size_t buffer_size = std::min(MaxBufferSize, static_cast<size_t>(in_file_size));

        FileIO::File out_file;
        out_file.SetDeleteFileOnError(true)
                .OpenForWritingCreate(new_file_path);

        if( in_file_size > 0 )
        {
            FileIO::File in_file;
            in_file.OpenForReading(old_file_path);

            auto buffer = std::make_unique_for_overwrite<char[]>(buffer_size);

            for( size_t bytes_read; ( bytes_read = in_file.Read(buffer.get(), buffer_size) ) != 0; )
                out_file.Write(buffer.get(), bytes_read);
        }

        out_file.Close();

        return true;
    }

    catch(...)
    {
        return false;
    }
#endif
}


bool PortableFunctions::FileCopyWithExceptions(const InterfaceString old_file_path, const InterfaceString new_file_path,
                                               const FileOverwriteFlag file_overwrite_flag,
                                               std::tuple<int64_t, int64_t>* const out_file_size_and_modified_time/* = nullptr*/)
{
    if( file_overwrite_flag != FileOverwriteFlag::Always &&
        PortableFunctions::FileIsRegular(new_file_path) )
    {
        if( file_overwrite_flag == FileOverwriteFlag::Never )
        {
            return false;
        }

        else if( file_overwrite_flag == FileOverwriteFlag::Fail )
        {
            throw FileIO::Exception::FileCopyFailDestinationExists(old_file_path, new_file_path);
        }

        else
        {
            ASSERT(file_overwrite_flag == FileOverwriteFlag::Different);

            std::tuple<int64_t, int64_t> old_size_and_modified_time = FileSizeAndModifiedTime(old_file_path);

            if( std::get<0>(old_size_and_modified_time) != -1 &&
                old_size_and_modified_time == FileSizeAndModifiedTime(new_file_path) )
            {
                if( out_file_size_and_modified_time != nullptr )
                    *out_file_size_and_modified_time = old_size_and_modified_time;

                return false;
            }
        }
    }

    if( !FileCopy(old_file_path, new_file_path, false) )
        throw FileIO::Exception::FileCopyFail(old_file_path, new_file_path);

    if( out_file_size_and_modified_time != nullptr )
        *out_file_size_and_modified_time = FileSizeAndModifiedTime(new_file_path);

    return true;
}


bool PortableFunctions::FileDelete(const InterfaceString file_path)
{
#ifdef WIN32
    return ( DeleteFile(file_path.c_str()) != 0 );

#else
    if( FileIsDirectory(file_path) )
    {
        // ALW 20180525 Android implementation of std::remove deletes files and directories. Ignore directories.
        return false;
    }

    else
    {
        return ( std::remove(file_path.c_str()) == 0 );
    }

#endif
}


void PortableFunctions::FileDeleteWithExceptions(const InterfaceString file_path)
{
    if( FileExists(file_path) && !FileDelete(file_path) )
        throw FileIO::Exception::FileDeleteFail(file_path);
}


bool PortableFunctions::DirectoryDelete(const InterfaceString directory, const bool delete_all_paths_within_directory/* = false*/)
{
    unsigned errors = 0;

    // delete any files and subdirectories with the directory
    if( delete_all_paths_within_directory )
    {
        DirectoryLister directory_lister(false, true, true);

        for( const std::wstring& path : directory_lister.GetPaths(UTF8_TODO::EnsureWide(directory)) )
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
    if( rmdir(directory.c_str()) != 0 )
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
T PortableFunctions::FileSize(const InterfaceString file_path)
{
#ifdef WIN32
    struct _stat64 attrib;
    const int ret = _wstat64(file_path.c_str(), &attrib);
#else
    struct stat attrib;
    const int ret = stat(file_path.c_str(), &attrib);
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

template CLASS_DECL_ZTOOLSO int64_t PortableFunctions::FileSize(InterfaceString file_path);
template CLASS_DECL_ZTOOLSO std::optional<uint64_t> PortableFunctions::FileSize(InterfaceString file_path);


std::string PortableFunctions::FileSizeString(const int64_t file_size)
{
    if( file_size >= 0 )
    {
#ifdef WIN32
        constexpr int SizeLength = 64;

        // the older StrFormatByteSizeA can only format DWORD arguments
        if( file_size <= std::numeric_limits<DWORD>::max() )
        {
            std::string file_size_string;
            file_size_string.resize(SizeLength);

            if( StrFormatByteSizeA(static_cast<DWORD>(file_size), file_size_string.data(), SizeLength) != nullptr )
            {
                file_size_string.resize(strlen(file_size_string.data()));
                return file_size_string;
            }
        }

        else
        {
            std::wstring file_size_string;
            file_size_string.resize(SizeLength);

            if( StrFormatByteSizeW(file_size, file_size_string.data(), SizeLength) != nullptr )
            {
                file_size_string.resize(wcslen(file_size_string.data()));
                return UTF8_TODO::GetUtf8(file_size_string);
            }
        }

#else
        // unimplemented
        return ReturnProgrammingError(IntToString(file_size));
#endif
    }

    return std::string();
}


bool PortableFunctions::FileExists(const InterfaceString path)
{
    if( path.empty() )
        return false;

#ifdef WIN32
    struct _stat64 fstatus;
    return ( _wstat64(path.c_str(), &fstatus) == 0 );

#else
    filestat fstatus;
    return ( stat(path.c_str(), &fstatus) == 0 );

#endif
}


bool PortableFunctions::FileIsRegular(const InterfaceString path)
{
    if( path.empty() )
        return false;

#ifdef WIN32
    struct _stat64 fstatus;
    const int stat_ret = _wstat64(path.c_str(), &fstatus);

#else
    filestat fstatus;
    const int stat_ret = stat(path.c_str(), &fstatus);

#endif

    return ( stat_ret == 0 && S_ISREG(fstatus.st_mode) );
}


bool PortableFunctions::FileIsDirectory(InterfaceString path)
{
    if( path.empty() )
        return false;

#ifdef WIN32
    // _wstat64 fails on drive letters without trailing / so add
    // trailing / e.g. "C:" => "C:/"
    if( path.back() == ':' )
        return FileIsDirectory(path.Release().append(L"/"));

    struct _stat64 st;
    return ( _wstat64(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode) );

#else
    struct stat st;
    return ( stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode) );

#endif
}


template<bool ThrowExceptionOnError/* = false*/>
int64_t PortableFunctions::FileModifiedTime(const InterfaceString file_path)
{
#ifdef WIN32
    struct _stat64 attrib;
    const int ret = _wstat64(file_path.c_str(), &attrib);
#else
    struct stat attrib;
    const int ret = stat(file_path.c_str(), &attrib);
#endif

    if( ret == 0 && S_ISREG(attrib.st_mode) )
        return attrib.st_mtime;

    if constexpr(ThrowExceptionOnError)
    {
        throw FileIO::Exception::FileNotFound(file_path);
    }

    else
    {
        return 0;
    }
}

template CLASS_DECL_ZTOOLSO int64_t PortableFunctions::FileModifiedTime<true>(InterfaceString file_path);
template CLASS_DECL_ZTOOLSO int64_t PortableFunctions::FileModifiedTime<false>(InterfaceString file_path);


std::tuple<int64_t, int64_t> PortableFunctions::FileSizeAndModifiedTime(const InterfaceString file_path)
{
#ifdef WIN32
    struct _stat64 attrib;
    int ret = _wstat64(file_path.c_str(), &attrib);
#else
    struct stat attrib;
    int ret = stat(file_path.c_str(), &attrib);
#endif

    return ( ret == 0 && S_ISREG(attrib.st_mode) ) ? std::tuple<int64_t, int64_t>(attrib.st_size, attrib.st_mtime) :
                                                     std::tuple<int64_t, int64_t>(-1, 0);
}


bool PortableFunctions::FileTouch(const InterfaceString file_path)
{
#ifdef WIN32
    return ( _wutime(file_path.c_str(), nullptr) == 0 );
#else
    return ( utime(file_path.c_str(), nullptr) == 0 );
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


std::string PortableFunctions::FileTempPath(const std::string& directory_path)
{
#ifdef WIN32
    wchar_t temp_file_name[MAX_PATH];
    ::GetTempFileName(TC::ToWide(directory_path).c_str(), L"CSPTMP", 0, temp_file_name);
    return TC::ToUtf8(temp_file_name);

#else
    std::string path_template = Path::Combine(directory_path, "CSPTMPXXXXXX");

#ifdef ANDROID
    android_mkstemp(path_template.data());
#else
    mkstemp(path_template.data());
#endif

    return path_template;
#endif
}


std::string PortableFunctions::GetUniqueFilePathInDirectory(const std::string_view directory_sv, const std::string_view extension_sv,
                                                            const char* filename_prefix/* = nullptr*/,
                                                            const std::function<bool(const std::string&)> extra_check_callback/* = { }*/)
{
    static int i = -1;

    if( filename_prefix == nullptr )
        filename_prefix = ".CS";

    while( true )
    {
        ++i;

        std::string file_path = Path::Combine(std::string(directory_sv),
                                              PathAppendFileExtension(SO::Concatenate(filename_prefix, IntToString(i)), extension_sv));

        if( !PortableFunctions::FileExists(file_path) )
        {
            if( !extra_check_callback || extra_check_callback(file_path) )
                return file_path;
        }
    }
}


std::string PortableFunctions::GetCommandLine()
{
#ifdef WIN_DESKTOP
    return TC::ToUtf8(::GetCommandLine());
#else
    return std::string();
#endif
}


namespace
{
    template<typename CF>
    std::string GenerateMd5(const CF& md5_update_callback)
    {
        MD5_CTX ctx;
        MD5_Init(&ctx);

        do { } while( md5_update_callback(ctx) );

        constexpr size_t HexSequences = 16;

        unsigned char result[HexSequences];
        MD5_Final(result, &ctx);

        std::string md5_string(HexSequences * 2, '\0');
        char* md5_string_buffer = md5_string.data();

        for( size_t i = 0; i < HexSequences; ++i, md5_string_buffer += 2 )
            std::snprintf(md5_string_buffer, 3, "%02x", static_cast<unsigned int>(result[i]));

        return md5_string;
    }
}


std::string PortableFunctions::FileMd5(const InterfaceString& file_path, const bool throw_exception_on_read_error/* = false*/)
{
    auto return_error = [&]()
    {
        if( throw_exception_on_read_error )
            throw CSProException("A MD5 could not be created for: %s", file_path.c_str_utf8());

        return std::string();
    };

    FILE* const file = !file_path.empty() ? PortableFunctions::FileOpen(file_path, "rb") :
                                            nullptr;

    if( file == nullptr )
        return return_error();

    constexpr size_t BufferSize = 64 * 1024;
    auto buffer = std::make_unique_for_overwrite<char[]>(BufferSize);

    std::string md5_string;
    class Md5Error { };

    try
    {
        md5_string = GenerateMd5([&](MD5_CTX& ctx) -> bool
        {
            const size_t bytes_read = fread(buffer.get(), 1, BufferSize, file);
            MD5_Update(&ctx, buffer.get(), uint32_cast(bytes_read));

            if( ferror(file) )
                throw Md5Error();

            return !feof(file);
        });
    }
    catch( const Md5Error& ) { }

    fclose(file);

    if( md5_string.empty() )
        return return_error();

    return md5_string;
}


std::string PortableFunctions::StreamMd5(std::istream& input_stream)
{
    if( input_stream )
    {
        class Md5Error { };

        try
        {
            constexpr size_t BufferSize = 64 * 1024;
            auto buffer = std::make_unique_for_overwrite<char[]>(BufferSize);

            return GenerateMd5(
                [&](MD5_CTX& ctx) -> bool
                {
                    input_stream.read(buffer.get(), BufferSize);

                    const std::streamsize bytes_read = input_stream.gcount();

                    if( bytes_read > 0 )
                    {
                        MD5_Update(&ctx, buffer.get(), uint32_cast(bytes_read));
                        return true;
                    }

                    else if( input_stream.eof() )
                    {
                        return false;
                    }

                    else
                    {
                        throw Md5Error();
                    }

                });
        }

        catch( const Md5Error& ) { }
    }

    throw CSProException("A MD5 could not be created for the input stream.");
}


std::string PortableFunctions::BinaryMd5(const std::byte* const contents, const size_t size)
{
    return GenerateMd5([&](MD5_CTX& ctx) -> bool
    {
        MD5_Update(&ctx, contents, uint32_cast(size));
        return false;
    });
}


std::string PortableFunctions::StringMd5(const std::string_view text_sv)
{
    return BinaryMd5(reinterpret_cast<const std::byte*>(text_sv.data()), text_sv.length());
}


int64_t PortableFunctions::ftelli64(FILE* stream)
{
#ifdef WIN32
    return _ftelli64(stream);
#else
    return ftello(stream);
#endif
}


int PortableFunctions::fseeki64(FILE* const stream, const int64_t offset, const int origin)
{
#ifdef WIN32
    return _fseeki64(stream, offset, origin);
#else
    return fseeko(stream, offset, origin);
#endif
}


bool PortableFunctions::PathMakeDirectory(const InterfaceString directory_path)
{
#ifdef WIN32
    return ( CreateDirectory(directory_path.c_str(), nullptr) != 0 );
#else
    return ( mkdir(directory_path.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0 );
#endif
}


bool PortableFunctions::PathMakeDirectories(InterfaceString directory_path)
{
    if( PortableFunctions::FileIsDirectory(directory_path) )
        return true;

    std::string_view directory_path_sv = directory_path.GetString<std::string>();
    directory_path_sv = SO::TrimRight(directory_path_sv, Path::NativeSlashChar);

    size_t pos = directory_path_sv.find(Path::NativeSlashChar, 1);

    while( pos != std::string_view::npos )
    {
        const std::string this_directory_path(directory_path_sv.substr(0, pos + 1));

        if( !PortableFunctions::FileIsDirectory(this_directory_path) )
        {
            // ignore result here since in some scenarios
            // we don't have permission to access earlier directories in path
            PortableFunctions::PathMakeDirectory(this_directory_path);
        }

        pos = directory_path_sv.find(Path::NativeSlashChar, pos + 1);
    }

    return PortableFunctions::PathMakeDirectory(std::move(directory_path));
}


const TCHAR* PortableFunctions::PathGetFilename(NullTerminatedString path)
{
    const TCHAR* path_start = path.c_str();
    const TCHAR* last_path_character = path_start + path.length() - 1;
    const TCHAR* path_itr = last_path_character;

    for( ; path_itr >= path_start; --path_itr )
    {
        if( Path::IsSlashChar(*path_itr) )
        {
            // ignore the trailing /
            if( path_itr != last_path_character )
                break;
        }
    }

    ASSERT(PortableFunctions::PathGetFilename(UTF8_TODO::GetUtf8(path)) == UTF8_TODO::GetUtf8(path_itr + 1));

    return path_itr + 1;
}


std::string PortableFunctions::PathGetDirectory(const std::string_view path_sv)
{
    size_t path_length = path_sv.length();

    if( path_length != 0 )
    {
        const char* path_itr = &path_sv.back();

        do
        {
            if( Path::IsSlashChar(*path_itr) )
                return std::string(path_sv.data(), path_length);

             --path_length;
             --path_itr;

        } while( path_length > 0 );
    }

    return std::string();
}


template<typename T/* = std::wstring*/>
T PortableFunctions::PathGetDirectory(const wstring_view path_sv)
{
    size_t path_length = path_sv.length();

    if( path_length != 0 )
    {
        const wchar_t* path_itr = path_sv.data() + path_length - 1;

        do
        {
            if( Path::IsSlashChar(*path_itr) )
                return T(path_sv.data(), string_len_cast<T>(path_length));

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
    template<typename CT>
    const CT* FindFileExtensionStart(const std::basic_string_view<CT> path_sv)
    {
        if( !path_sv.empty() )
        {
            const CT* const path_begin = path_sv.data();
            const CT* path_itr = path_begin + path_sv.length() - 1;

            while( true )
            {
                if( *path_itr == '.' )
                    return path_itr;

                if( Path::IsSlashChar(*path_itr) || path_itr == path_begin )
                    break;

                --path_itr;
            }
        }

        return nullptr;
    }
}


std::string PortableFunctions::PathReplaceFilename(const std::string_view path_sv, const std::string_view filename_sv)
{
    return Path::Combine(PathGetDirectory(path_sv), filename_sv);
}


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

            filename.Append(extension_sv.data(), int32_cast(extension_sv.length()));
        }
    }

    return filename;
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathAppendFileExtension(std::wstring filename, wstring_view extension_sv);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathAppendFileExtension(CString filename, wstring_view extension_sv);


std::string PortableFunctions::PathEnsureFileExtension(std::string file_path, const std::string_view extension_sv)
{
    const bool include_dot = !extension_sv.empty() && extension_sv.front() == '.';
    const std::string this_extension = PathGetFileExtension(file_path, include_dot);

    return SO::EqualsNoCase(extension_sv, this_extension) ? file_path :
                                                            PathAppendFileExtension(std::move(file_path), extension_sv);
}


std::string PortableFunctions::CreateFilePath(std::string directory_path, const std::string_view filename_sv, const std::string_view extension_sv/* = std::string_view()*/)
{
    return PortableFunctions::PathAppendFileExtension(Path::Combine(std::move(directory_path), filename_sv),
                                                      extension_sv);
}


std::wstring PortableFunctions::PathToNativeSlash(std::wstring path)
{
    constexpr TCHAR NativeSlash = PATH_CHAR;
    constexpr TCHAR NonNativeSlash = ( NativeSlash == '/' ) ? '\\' : '/';
    return SO::Replace(path, NonNativeSlash, NativeSlash);
}


CString PortableFunctions::PathToNativeSlash(CString path)
{
    constexpr TCHAR NativeSlash = PATH_CHAR;
    constexpr TCHAR NonNativeSlash = ( NativeSlash == '/' ) ? '\\' : '/';
    path.Replace(NonNativeSlash, NativeSlash);
    return path;
}


template<typename T>
T PortableFunctions::PathAppendToPath(T path, wstring_view append_text_sv, const TCHAR separator/* = PATH_CHAR*/)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        if( path.empty() )
            return append_text_sv;

        const int separators_used = ( Path::IsSlashChar(path.back()) ? 1 : 0 ) +
                                    ( ( !append_text_sv.empty() && Path::IsSlashChar(append_text_sv.front()) ) ? 1 : 0 );

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

        const int separators_used = ( Path::IsSlashChar(path[path.GetLength() - 1]) ? 1 : 0 ) +
                                    ( ( !append_text_sv.empty() && Path::IsSlashChar(append_text_sv.front()) ) ? 1 : 0 );

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

        path.Append(append_text_sv.data(), string_len_cast<T>(append_text_sv.length()));

        return path;
    }
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathAppendToPath(std::wstring path, wstring_view append_text_sv, TCHAR separator);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathAppendToPath(CString path, wstring_view append_text_sv, TCHAR separator);


std::string PortableFunctions::PathRemoveTrailingSlash(std::string path)
{
    return SO::MakeTrimRight(path, Path::SlashChars_sv);
}


template<typename T/* = std::wstring*/>
T PortableFunctions::PathRemoveTrailingSlash(const wstring_view path_sv)
{
    return SO::TrimRight(path_sv, L"/\\");
}

template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathRemoveTrailingSlash(wstring_view path_sv);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathRemoveTrailingSlash(wstring_view path_sv);


template<typename T>
T PortableFunctions::PathEnsureTrailingSlash(T path, const char separator/* = Path::NativeSlashChar*/)
{
    if constexpr(!std::is_same_v<T, CString>)
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

template CLASS_DECL_ZTOOLSO std::string PortableFunctions::PathEnsureTrailingSlash(std::string path, char separator);
template CLASS_DECL_ZTOOLSO std::wstring PortableFunctions::PathEnsureTrailingSlash(std::wstring path, char separator);
template CLASS_DECL_ZTOOLSO CString PortableFunctions::PathEnsureTrailingSlash(CString path, char separator);


int64_t PortableFunctions::ParseRFC3339DateTime(std::string date_time) // UTF8_TODO move to the DateTime class
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
    char* buffer = date_time.data();
    char* pZoneStart = buffer + 19;
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
            hoursOffset = atoi(pZoneStart + 1);
            minsOffset = atoi(pZoneStart + 4);
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

    timeStruct.tm_year = atoi(buffer) - 1900;
    timeStruct.tm_mon = atoi(buffer + 5) - 1;
    timeStruct.tm_mday = atoi(buffer + 8);
    timeStruct.tm_hour = atoi(buffer + 11);
    timeStruct.tm_min = atoi(buffer + 14);
    timeStruct.tm_sec = atoi(buffer + 17);
    timeStruct.tm_isdst = 0;
    timeStruct.tm_wday = 0;
    timeStruct.tm_yday = 0;

    int64_t unixTime = _mkgmtime(&timeStruct);

    unixTime += minsOffset * 60 + hoursOffset * 60 * 60;

    return unixTime;
}


int64_t PortableFunctions::ParseYYYYMMDDhhmmssDateTime(std::string date_time)
{
    if( date_time.size() < 14 )
        return 0;

    // Make a copy so we can modify string
    char* buffer = date_time.data();
    tm timeStruct;

    timeStruct.tm_sec = atoi(buffer + 12);
    buffer[12] = 0;
    timeStruct.tm_min = atoi(buffer + 10);
    buffer[10] = 0;
    timeStruct.tm_hour = atoi(buffer + 8);
    buffer[8] = 0;
    timeStruct.tm_mday = atoi(buffer + 6);
    buffer[6] = 0;
    timeStruct.tm_mon = atoi(buffer + 4) - 1;
    buffer[4] = 0;
    timeStruct.tm_year = atoi(buffer) - 1900;
    timeStruct.tm_isdst = 0;
    timeStruct.tm_wday = 0;
    timeStruct.tm_yday = 0;

    return _mkgmtime(&timeStruct);
}
