#include "StdAfx.h"
#include "Tools.h"
#include "TextEncoding.h"

#ifdef WIN32
#include "WinRegistry.h"
#include <io.h>
#include <lmcons.h>
#include <VersionHelpers.h>
#endif

#ifndef WIN_DESKTOP
#include <zPlatformO/PlatformInterface.h>
#endif


template<>
TextEncoding GetEncodingFromBOM(const int iFileHandle)
{
    // files without a BOM will be treated as UTF-8
    TextEncoding text_encoding = TextEncoding::Type::Utf8;

    // go to the begining of the file
    _lseek(iFileHandle, 0, SEEK_SET);

    // all BOMs handled by CSPro are three characters or fewer
    char bom_data[3];
    const int bytes_read = _read(iFileHandle, bom_data, sizeof(bom_data));

    if( bytes_read != 0 )
    {
        text_encoding = TextEncoding(bom_data, bytes_read, TextEncoding::Type::Utf8);

        // go back to the beginning of the file
        _lseek(iFileHandle, 0, SEEK_SET);
    }

    return text_encoding;
}

template CLASS_DECL_ZTOOLSO TextEncoding GetEncodingFromBOM(int iFileHandle);


template<>
Encoding GetEncodingFromBOM(const int iFileHandle)
{
    const TextEncoding text_encoding = GetEncodingFromBOM<TextEncoding>(iFileHandle);

    switch( text_encoding.GetType() )
    {
        case TextEncoding::Type::Ansi:
        case TextEncoding::Type::Utf8:
            return Encoding::Ansi;

        case TextEncoding::Type::Utf8Bom:
            return Encoding::Utf8;

        case TextEncoding::Type::Utf16LE:
            return Encoding::Utf16LE;

        case TextEncoding::Type::Utf16BE:
            return Encoding::Utf16BE;

        default:
            return ReturnProgrammingError(Encoding::Invalid);
    }
}

template CLASS_DECL_ZTOOLSO Encoding GetEncodingFromBOM(int iFileHandle);



Encoding GetEncodingFromBOM(FILE* const file)
{
    return GetEncodingFromBOM(_fileno(file));
}


// 20111213 this function opens a file, gets the BOM, and closes the file; returns false if the file can't be opened
bool GetFileBOM(const InterfaceString file_path, Encoding& encoding)
{
#ifndef WIN32
#define _SH_DENYNO -1
#endif
    FILE* tempFile = PortableFunctions::FileOpen(file_path, "rb", _SH_DENYNO);

    if( tempFile == nullptr )
        return false;

    encoding = GetEncodingFromBOM(tempFile);

    fclose(tempFile);

    return true;
}


const char* ToString(const Encoding encoding)
{
    constexpr const char* EncodingStrings[] = { "Invalid", "ANSI", "UTF-16LE", "UTF-16BE", "UTF-8" };
    return EncodingStrings[static_cast<size_t>(encoding)];
}


// Return errorlevel when bWait is used
// Return 1/0 when bWait is false. 1 indicates the program was executed.
bool RunProgram(std::wstring command, int* iRetCode, int iShowWindow, bool bFocus, bool bWait,
                const wchar_t* const directory/* = nullptr*/)
{
    bool    bRet=true;
    *iRetCode = 0; // RHF May 22, 2006

#ifdef WIN_DESKTOP
    ASSERT( iShowWindow == SW_MAXIMIZE ||
        iShowWindow == SW_MINIMIZE ||
        iShowWindow == SW_SHOWNA );

    if( bFocus ) {
        if( iShowWindow == SW_MAXIMIZE ) {
            iShowWindow = SW_SHOWMAXIMIZED;
        }
        else if( iShowWindow == SW_MINIMIZE ) {
            iShowWindow = SW_SHOWMINIMIZED;
        }
        else if( iShowWindow == SW_SHOWNA ) {
            iShowWindow = SW_SHOWNORMAL;
        }
    }
    else {
        if( iShowWindow == SW_MINIMIZE )
            iShowWindow = SW_SHOWMINNOACTIVE;
    }

    // if( bWait ) {
    // 20110922 switching from WinExec to CreateProcess (for no wait calls)
    if( true ) {
        STARTUPINFO si;
        PROCESS_INFORMATION pi;

        ZeroMemory( &si, sizeof(si) );
        si.cb = sizeof(si);
        ZeroMemory( &pi, sizeof(pi) );

        //if( iShowWindow == SW_MAXIMIZE || iShowWindow == SW_MINIMIZE || iShowWindow == SW_SHOWNORMAL ) {
        si.dwFlags = STARTF_USESHOWWINDOW;
        si.wShowWindow=(WORD)iShowWindow;
        //}

        // Start the child process.
        if( !CreateProcess(nullptr, // No module name (use command line).
            command.data(),
            nullptr,                // Process handle not inheritable.
            nullptr,                // Thread handle not inheritable.
            FALSE,                  // Set handle inheritance to FALSE.
            0,                      // No creation flags.
            nullptr,                // Use parent's environment block.
            directory,              // Use parent's starting directory.
            &si,                    // Pointer to STARTUPINFO structure.
            &pi )                   // Pointer to PROCESS_INFORMATION structure.
            )
        {
            return false;
        }

        // 20110922
        if( !bWait ) {
            *iRetCode = 1;
            //return true;
        }
        else
        {
            *iRetCode=0;

            // Wait until child process exits.
            WaitForSingleObject( pi.hProcess, INFINITE );

            DWORD   uiRetCode=0;
            GetExitCodeProcess( pi.hProcess, &uiRetCode );

            *iRetCode = uiRetCode;
        }

        // Close process and thread handles.
        CloseHandle( pi.hProcess );
        CloseHandle( pi.hThread );
    }
    /*else {
        UINT uRet = ::WinExec((const csprochar*) csCmd, iShowWindow );

        if( uRet > 31 ) // If the function succeeds, the return value is greater than 31.
            *iRetCode = 1;
        else {
            *iRetCode = 0;
            bRet = false;
        }
    }*/
#else
    ASSERT(false);
#endif

    return bRet;
}

//FABN Jan 23, 2006 -> moved from CCapiEUtils (old code)
CString clearString(CString ss, bool bNum) {

    ss.Trim();

    if(bNum && !ss.IsEmpty()){
        CString zeros(_T('0'), ss.GetLength());
        if( ss.Compare(zeros)==0 ){
            ss = _T("0");
        } else {
            ss.TrimLeft('0');
        }
    }
    return ss;
}


#ifndef WIN32
BOOL PathRemoveFileSpec( csprochar* pszPath ) {
    csprochar*  p=pszPath + _tcslen(pszPath)-1;
    bool        bFound=false;

    while( p >= pszPath ) {
        while( ( *p == _T('\\') || *p == '/') && p >= pszPath ) {
            *p = 0;
            p--;
            bFound = true;
        }
        if( bFound ) break;
        p--;
    }

    return bFound ? TRUE: FALSE;

/*
    CString csStringU;

    csStringU = pszPath;

    PathRemoveFileSpecW( csStringU.GetBuffer(MAX_PATH) );
    csStringU.ReleaseBuffer();

    CString csStringX;

    csStringX = csStringU;

    memcpy( pszPath, csStringX, csStringX.GetLength() );
    pszPath[csStringX.GetLength()] = 0;

    ASSERT(0);
    return false;
*/
}

void PathStripPath( csprochar* pszPath )
{
    csprochar* const pend = pszPath + _tcslen(pszPath);
    csprochar*  lastSlash=pend-1;

    // ignore slash as last char in path
    if ( lastSlash >= pszPath && (*lastSlash == _T('\\') || *lastSlash == '/')) {
        lastSlash--;
    }

    // walk back from end of path chars until you find /
    while( lastSlash >= pszPath && *lastSlash != _T('\\') && *lastSlash != '/') {
            lastSlash--;
    }

    if (lastSlash >= pszPath) {
        // copy from after slash to start of path (reusing same string)
        for (csprochar* p = lastSlash + 1; p <= pend; ++p) {
            *(pszPath++) = *p;
        }
    }
}

#endif

#ifndef WIN32

bool PathIsRelative(const TCHAR* lpszPath)
{
    return ( *lpszPath == _T('.') || !( *lpszPath == _T('/') || *lpszPath == '\\' ) );
}

BOOL PathCanonicalize( csprochar* lpszDst, const csprochar* lpszSrc ){
    int destPos = 0;
    int strLen = _tcslen(lpszSrc);
    BOOL bRet = TRUE; // are there errors in the path?

    for( int i = 0; i < strLen; i++ )
    {
        if( lpszSrc[i] == '.' )
        {
            if( ( i + 2 ) <= strLen && lpszSrc[i + 1] == '.' && (lpszSrc[i + 2] == PATH_CHAR ||  lpszSrc[i + 2] == _T('\0'))) // going back one folder (../)
            {
                if( destPos == 0 ) // something is wrong with the relative pathing; we'll still process though
                    bRet = FALSE;

                bool nonSlashFound = false;

                for( destPos--; destPos >= 0; destPos-- ) // find the last folder
                {
                    if( lpszDst[destPos] == PATH_CHAR && nonSlashFound )
                        break;

                    nonSlashFound = true; // this is so we get rid of the second slash in a case like this: /myfolder/../myfolder
                }

                destPos++;

                i += 2; // skip past the ../
                continue;
            }

            else if( ( i + 1 ) <= strLen && (lpszSrc[i + 1] == PATH_CHAR || lpszSrc[i + 1] == _T('\0'))) // relative to this folder (./)
            {
                i += 1; // we don't have to process this at all
                continue;
            }
        }

        lpszDst[destPos++] = lpszSrc[i];
    }

    lpszDst[destPos] = 0;

    return bRet;
}

void PathRelativePathTo(LPTSTR pszPath, LPCTSTR pszFrom, DWORD dwAttrFrom, LPCTSTR pszTo, DWORD dwAttrTo)
{
    // this is a simple, probably not very comprehensive, implementation of this function
    ASSERT(dwAttrFrom == FILE_ATTRIBUTE_NORMAL && dwAttrTo == FILE_ATTRIBUTE_NORMAL);
    ASSERT(!PathIsRelative(pszFrom) && !PathIsRelative(pszTo));

    size_t number_characters_matching = 0;

    while( pszFrom[number_characters_matching] != 0 && toupper(pszFrom[number_characters_matching]) == toupper(pszTo[number_characters_matching]) )
        ++number_characters_matching;

    CString result;

    // remove any matching characters that are within the same directory
    while( number_characters_matching > 0 && pszFrom[number_characters_matching - 1] != PATH_CHAR )
        --number_characters_matching;

    // if no characters match, don't make the path relative
    if( number_characters_matching == 0 )
    {
        result = pszTo;
    }

    else
    {
        // count the number of paths to go back
        for( size_t i = number_characters_matching; pszFrom[i] != 0; ++i )
        {
            if( pszFrom[i] == PATH_CHAR )
                result.AppendFormat(_T("..%c"), PATH_CHAR);
        }

        if( result.IsEmpty() )
            result.AppendFormat(_T(".%c"), PATH_CHAR);

        result.Append((LPCTSTR)pszTo + number_characters_matching);
    }

    SO::CopyToFixedBuffer(pszPath, CS2WS(result), MAX_PATH);
}

#endif


template<bool ThrowExceptionOnError/* = false*/>
std::conditional_t<ThrowExceptionOnError, void, bool> RecycleFile(const InterfaceString file_path)
{
    bool success = false;

#ifdef WIN_DESKTOP
    SHFILEOPSTRUCT info = { nullptr };
    auto complete_file_path = std::make_unique_for_overwrite<wchar_t[]>(MAX_PATH);

    if( GetFullPathName(file_path.c_str(), MAX_PATH, complete_file_path.get(), nullptr) != 0 )
    {
        info.wFunc = FO_DELETE;
        info.pFrom = complete_file_path.get();
        info.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_FILESONLY;
        success = ( SHFileOperation(&info) == 0 );
    }
#endif

    if constexpr(ThrowExceptionOnError)
    {
        if( !success )
            throw CSProException("The file could not be recycled: %s", file_path.c_str_utf8());
    }

    else
    {
        return success;
    }
}

template CLASS_DECL_ZTOOLSO void RecycleFile<true>(InterfaceString file_path);
template CLASS_DECL_ZTOOLSO bool RecycleFile<false>(InterfaceString file_path);


std::wstring GetWorkingFolder(const wstring_view base_filename_sv)
{
    std::wstring working_folder = PortableFunctions::PathGetDirectory(base_filename_sv);

    if( !working_folder.empty() )
    {
        ASSERT(working_folder == PortableFunctions::PathEnsureTrailingSlash(working_folder));
        working_folder.pop_back();
        return working_folder;
    }

    return GetWorkingFolder();
}


std::wstring GetWorkingFolder()
{
#ifdef WIN_DESKTOP
    std::wstring working_folder(MAX_PATH, '\0');
    int length = GetCurrentDirectory(MAX_PATH, working_folder.data());
    working_folder.resize(length);
#else
    std::wstring working_folder = UTF8_TODO::GetWide(PlatformInterface::GetInstance()->GetWorkingDirectory());
#endif

    ASSERT(working_folder.length() == ( PortableFunctions::PathEnsureTrailingSlash(working_folder).length() - 1 ));

    return working_folder;
}


std::string GetWorkingDirectory(const std::string_view base_filename_sv)
{
    std::string working_folder = PortableFunctions::PathGetDirectory(base_filename_sv);

    if( !working_folder.empty() )
    {
        ASSERT(working_folder == PortableFunctions::PathEnsureTrailingSlash(working_folder));
        working_folder.pop_back();
        return working_folder;
    }

    return GetWorkingDirectory();
}


std::string GetWorkingDirectory()
{
    return UTF8_TODO::GetUtf8(GetWorkingFolder());
}


#ifndef WIN32

LPCTSTR PathFindExtension(LPCTSTR lpszPath) // 20131121
{
    int strLen = wcslen(lpszPath);
    LPCTSTR pEndString = lpszPath + strLen;

    for( int i = strLen - 1; i >= 0; i-- )
    {
        if( lpszPath[i] == '.' )
        {
            return lpszPath + i;
        }

        // we've reached a folder, so no extension
        else if( lpszPath[i] == '/' || lpszPath[i] == '\\' )
        {
            return pEndString;
        }
    }

    return pEndString;
}

void PathRemoveExtension(LPTSTR lpszPath) // 20131121
{
    LPTSTR pExt = const_cast<LPTSTR>(PathFindExtension(lpszPath));
    *pExt = 0;
}

#endif


// Function name    : MakeFullPath
// Description      : //Makes the full path by Canonicalizing the PathRelativeTo+ Relativepath
//      ex: "d:\code\cspro20\test" + "..\..\test.fmf" = d:\code\test.fmf
// Return type      : CString
// Argument         : CString sRelativeToFName
// Argument         : CString sFileName
std::wstring MakeFullPath(const wstring_view relative_to_directory_sv, std::wstring filename)
{
    if( filename.empty() )
        return filename;

    filename = PortableFunctions::PathToNativeSlash(filename);

    if( PathIsRelative(filename.c_str()) )
        filename.insert(0, std::wstring(SO::TrimRight(relative_to_directory_sv, PATH_CHAR)) + PATH_STRING);

    std::wstring full_path(MAX_PATH, '\0');
    PathCanonicalize(full_path.data(), filename.c_str());
    full_path.resize(_tcslen(full_path.data()));

    return full_path;
}


std::string MakeFullPath(const std::string_view relative_to_directory_sv, std::string filename)
{
    if( filename.empty() )
        return filename;

    PortableFunctions::MakePathToNativeSlash(filename);

    if( Path::IsRelative(filename) )
        filename.insert(0, std::string(SO::TrimRight(relative_to_directory_sv, Path::NativeSlashChar)) + Path::NativeSlashString);

    std::wstring full_path(MAX_PATH, '\0');
    PathCanonicalize(full_path.data(), UTF8_TODO::GetWide(filename).c_str());
    full_path.resize(wcslen(full_path.data()));

    return UTF8_TODO::GetUtf8(full_path);
}


/////////////////////////////////////////////////////////////////////////////////
//
//      CString GetRelativeFName(const TCHAR* sRelativeToFName, const TCHAR* sFileName)
//
/////////////////////////////////////////////////////////////////////////////////
template<typename T/* = std::wstring*/>
T GetRelativeFName(NullTerminatedString sRelativeToFName, NullTerminatedString sFileName)
{
    if constexpr(std::is_same_v<T, std::wstring>)
    {
        T relative_filename(MAX_PATH, '\0');
        PathRelativePathTo(relative_filename.data(), sRelativeToFName.c_str(), FILE_ATTRIBUTE_NORMAL, sFileName.c_str(), FILE_ATTRIBUTE_NORMAL);
        relative_filename.resize(_tcslen(relative_filename.data()));

        // if PathRelativePathTo Fails because of PathRelativePathTo limitations
        if( relative_filename.empty() )
        {
            TCHAR first_ch = sFileName[0];

            if( first_ch == '.' || first_ch == '\\' || first_ch == 0 || sFileName[1] == ':' )
            {
                return sFileName;
            }

            else
            {
                return T(_T(".\\")) + sFileName.c_str();
            }
        }

        return relative_filename;
    }

    else
    {
        CString sRelFName;
        PathRelativePathTo(sRelFName.GetBuffer(MAX_PATH), sRelativeToFName.c_str(),
                           FILE_ATTRIBUTE_NORMAL, sFileName.c_str(), FILE_ATTRIBUTE_NORMAL);

        sRelFName.ReleaseBuffer();

        // if PathRelativePathTo Fails because of PathRelativePathTo limitations
        if( sRelFName.IsEmpty() )
        {
            TCHAR first_ch = sFileName[0];

            if( first_ch == '.' || first_ch == '\\' || first_ch == 0 || sFileName[1] == ':' )
            {
                return sFileName;
            }

            else
            {
                return CString(_T(".\\")) + sFileName;
            }
        }

        return sRelFName;
    }
}

template CLASS_DECL_ZTOOLSO std::wstring GetRelativeFName(NullTerminatedString sRelativeToFName, NullTerminatedString sFileName);
template CLASS_DECL_ZTOOLSO CString GetRelativeFName(NullTerminatedString sRelativeToFName, NullTerminatedString sFileName);


std::string GetRelativePath(const std::string_view relative_to_file_path_sv, const std::string_view filename_sv)
{
    return UTF8_TODO::GetUtf8(GetRelativeFName<std::wstring>(UTF8_TODO::GetWide(relative_to_file_path_sv), UTF8_TODO::GetWide(filename_sv)));
}


std::string GetRelativePathForDisplay(const cs::string_view_sz relative_to_file_path, const cs::string_sz filename)
{
    const static std::string StartingInSameDirectoryPrefix = "." + std::string(Path::NativeSlashString);
    const static std::string RelativeToPreviousDirectoryPrefix = ".." + std::string(Path::NativeSlashString);

    std::string relative_path = UTF8_TODO::GetUtf8(GetRelativeFName<std::wstring>(UTF8_TODO::GetWide(relative_to_file_path), UTF8_TODO::GetWide(filename)));

    // remove the initial relative path information (if starting in the same directory as the relative filename)
    if( SO::StartsWith(relative_path, StartingInSameDirectoryPrefix) )
        return relative_path.substr(StartingInSameDirectoryPrefix.length());

    // if filename is the directory where relative_to_file_path resides, the relative filename
    // will be something like: ../dir_name, so in that case return ./ instead
    if( SO::StartsWith(relative_path, RelativeToPreviousDirectoryPrefix) &&
        SO::EqualsNoCase(PortableFunctions::PathGetDirectory(relative_to_file_path), PortableFunctions::PathEnsureTrailingSlash(relative_path)) )
    {
        return StartingInSameDirectoryPrefix;
    }

    return relative_path;
}


std::string EscapeCommandLineArgument(std::string argument)
{
    if( argument.find(' ') != std::string::npos )
    {
        if( argument.size() < 2 || argument.front() != '"' || argument.back() != '"' )
            return "\"" + argument + "\"";
    }

    return argument;
}


std::wstring EscapeCommandLineArgument(std::wstring argument)
{
    if( argument.find(' ') != std::wstring::npos )
    {
        if( argument.size() < 2 || argument.front() != '"' || argument.back() != '"' )
            return _T("\"") + argument + _T("\"");
    }

    return argument;
}


std::string UnescapeCommandLineArgument(std::string argument)
{
    if( argument.length() >= 2 && argument.front() == '"' && argument.back() == '"' )
        return argument.substr(1, argument.length() - 2);

    return argument;
}


std::string GetWindowsSpecialFolder(const WindowsSpecialFolder folder)
{
#ifndef WIN_DESKTOP
    return ReturnProgrammingError(std::string());
#else
    int folder_value;

    if( folder == WindowsSpecialFolder::Desktop )
    {
        folder_value = CSIDL_DESKTOP;
    }

    else if( folder == WindowsSpecialFolder::Windows )
    {
        folder_value = CSIDL_WINDOWS;
    }

    else if( folder == WindowsSpecialFolder::Documents )
    {
        folder_value = CSIDL_PERSONAL;
    }

    else if( folder == WindowsSpecialFolder::ProgramFiles32 )
    {
        folder_value = CSIDL_PROGRAM_FILES;
    }

    else /*if( folder == WindowsSpecialFolder::ProgramFiles64 )*/
    {
        // 20111102 the 64-bit request didn't actually work, so using the values from the registry instead
        WinRegistry registry;

        if( registry.Open(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion") )
        {
            std::optional<std::string> path = registry.ReadOptionalString("ProgramW6432Dir");

            if( path.has_value() )
                return PortableFunctions::PathEnsureTrailingSlash(std::move(*path));
        }

        // default to the 32-bit version if there is no registry entry
        folder_value = CSIDL_PROGRAM_FILES;
    }

    auto path = std::make_unique_for_overwrite<wchar_t[]>(MAX_PATH);
    SHGetSpecialFolderPath(nullptr, path.get(), folder_value, FALSE);
    return PortableFunctions::PathEnsureTrailingSlash(TC::ToUtf8(path.get()));
#endif
}


#ifdef WIN_DESKTOP

std::vector<std::string> GetLogicalDrivesVector()
{
    const DWORD drives_text_length = GetLogicalDriveStrings(0, nullptr);
    auto drives_text = std::make_unique_for_overwrite<wchar_t[]>(drives_text_length);

    // GetLogicalDriveStrings will return something like C:\[null]G:\[null][null]
    GetLogicalDriveStrings(drives_text_length, drives_text.get());

    std::vector<std::string> drives;

    const wchar_t* drives_text_itr = drives_text.get();
    const wchar_t* const drives_text_end = drives_text_itr + drives_text_length;

    while( drives_text_itr < drives_text_end && *drives_text_itr != '\0' )
    {
        const size_t drive_length = wcslen(drives_text_itr);
        drives.emplace_back(TC::ToUtf8(drives_text_itr, drive_length));
        drives_text_itr += drive_length + 1;
    }

    return drives;
}

#endif // WIN_DESKTOP


const std::string& GetDownloadsDirectory()
{
    static const std::string downloads_directory =
        [&]()
        {
            std::string directory;

#ifdef WIN_DESKTOP
            PWSTR path;

            if( SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &path)) )
            {
                directory = TC::ToUtf8(path);
                CoTaskMemFree(path);
            }

#else
            directory = PlatformInterface::GetInstance()->GetDownloadsDirectory();

#endif

            return PortableFunctions::PathEnsureTrailingSlash(std::move(directory));
        }();

    return downloads_directory;
}


namespace
{
    bool UseCRLF(const TCHAR* crlf_override)
    {
        ASSERT(crlf_override == nullptr || CString(crlf_override) == _T("\n") || CString(crlf_override) == _T("\r\n"));

        if( crlf_override != nullptr )
            return ( *crlf_override == 'r' );

#ifdef WIN32
        return true;
#else
        return false;
#endif
    }
}


CString DelimitCRLF(CString csText, const TCHAR* crlf_override/* = nullptr*/)
{
    bool use_crlf = UseCRLF(crlf_override);

    int p = 0;

    while( ( p = csText.Find(_T('\\'), p) ) >= 0 ) // 20120812 allow carriage returns as well as backslashes
    {
        csText = csText.Left(p + 1) + csText.Mid(p);
        p += 2;
    }

    p = 0;

    while( ( p = csText.Find(use_crlf ? _T("\r\n") : _T("\n"), p) ) >= 0 )
    {
        if( !use_crlf )
        {
            // add a space for the two characters: \\ + n (unlike on the desktop, which already has two characters for \r + \n
            csText = csText.Left(p + 1) + csText.Mid(p);
        }

        csText.SetAt(p++, '\\');
        csText.SetAt(p++, 'n');
    }

    return csText;
}


CString UndelimitCRLF(CString csText, const TCHAR* crlf_override/* = nullptr*/)
{
    bool use_crlf = UseCRLF(crlf_override);

    int p = 0; // 20120812 we need to backslash \ characters too so that a value like neither\none is valid

    while( ( p = csText.Find(_T('\\'), p) ) >= 0 && p < ( csText.GetLength() - 1 ) )
    {
        if( csText[p + 1] == _T('\\') )
        {
            csText = csText.Left(p + 1) + csText.Mid(p + 2);
        }

        else if( csText[p + 1] == _T('n') )
        {
            if( use_crlf )
            {
                csText.SetAt(p++, '\r');
                csText.SetAt(p, '\n');
            }

            else
            {
                csText.SetAt(p, '\n');
                csText = csText.Left(p + 1) + csText.Mid(p + 2);
            }
        }

        p++;
    }

    return csText;
}


#ifdef WIN32
#include "winsock2.h"
#undef PIF_INDEX
#include "Iphlpapi.h"
#pragma comment(lib, "iphlpapi.lib")
#endif

const std::string& GetDeviceId()
{
    // Cache this on first call since this will never change
    static const std::string device_id =
        []()
        {
#ifdef WIN32
            DWORD size;

            if( GetAdaptersAddresses(AF_INET, 0, nullptr, nullptr, &size ) == ERROR_BUFFER_OVERFLOW )
            {
                auto adapter_addresses_memory = std::make_unique_for_overwrite<std::byte[]>(size);
                PIP_ADAPTER_ADDRESSES adapter_addresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(adapter_addresses_memory.get());

                if( GetAdaptersAddresses(AF_INET, 0, nullptr, adapter_addresses, &size) == ERROR_SUCCESS )
                {
                    PIP_ADAPTER_ADDRESSES adapter_itr = adapter_addresses;

                    while( adapter_itr != nullptr )
                    {
                        if( adapter_itr->PhysicalAddressLength != 0 )
                        {
                            // if we eventually return more than one, remove return and keep processing the addresses
                            return FormatText("%02x%02x%02x%02x%02x%02x", static_cast<unsigned int>(adapter_itr->PhysicalAddress[0]),
                                                                          static_cast<unsigned int>(adapter_itr->PhysicalAddress[1]),
                                                                          static_cast<unsigned int>(adapter_itr->PhysicalAddress[2]),
                                                                          static_cast<unsigned int>(adapter_itr->PhysicalAddress[3]),
                                                                          static_cast<unsigned int>(adapter_itr->PhysicalAddress[4]),
                                                                          static_cast<unsigned int>(adapter_itr->PhysicalAddress[5]));
                        }

                        adapter_itr = adapter_itr->Next;
                    }
                }
            }

            return ReturnProgrammingError(std::string());

#else
            return PlatformInterface::GetInstance()->GetApplicationInterface()->GetDeviceId();
#endif
        }();

    return device_id;
}


std::string GetDeviceUserName()
{
#if WIN32
    constexpr DWORD MaxBufferSize = UNLEN + 1;
    wchar_t username[MaxBufferSize];
    DWORD buffer_size = MaxBufferSize;

    if( GetUserName(username, &buffer_size) == 0 )
        return ReturnProgrammingError(std::string());

    return TC::ToUtf8(username, buffer_size - 1);

#else // 20131209 we'll return the account user name on Android devices
    return PlatformInterface::GetInstance()->GetApplicationInterface()->GetUsername();
#endif
}


std::string GetLocaleLanguage(const bool separate_subtags_by_underscores/* = true*/)
{
    // Windows returns the locale with hyphens; Android with underscores

#ifdef WIN32
    auto wide_language_name = std::make_unique_for_overwrite<wchar_t[]>(LOCALE_NAME_MAX_LENGTH);
    const int length_with_null_character = GetUserDefaultLocaleName(wide_language_name.get(), LOCALE_NAME_MAX_LENGTH);

    std::string language_name = ( length_with_null_character != 0 ) ? TC::ToUtf8(wide_language_name.get(), length_with_null_character - 1) :
                                                                      ReturnProgrammingError(std::string());

    if( separate_subtags_by_underscores )
        SO::Replace(language_name, '-', '_');

#else
    std::string language_name = PlatformInterface::GetInstance()->GetApplicationInterface()->GetLocaleLanguage();

    if( !separate_subtags_by_underscores )
        SO::Replace(language_name, '_', '-');

#endif

    return language_name;
}


#ifdef WIN32

const std::string& GetLocaleInformation(const LCTYPE LCType)
{
    static std::map<LCTYPE, std::string> locale_info;
    const auto& lookup = locale_info.find(LCType);

    if( lookup != locale_info.cend() )
        return lookup->second;

    const int wide_length_with_null = GetLocaleInfo(LOCALE_USER_DEFAULT, LCType, nullptr, 0);
    auto wide_buffer = std::make_unique_for_overwrite<wchar_t[]>(wide_length_with_null);

    GetLocaleInfo(LOCALE_USER_DEFAULT, LCType, wide_buffer.get(), wide_length_with_null);

    return locale_info.emplace(LCType, TC::ToUtf8(wide_buffer.get(), wide_length_with_null - 1)).first->second;
}

#endif


const char* GetOperatingSystemName()
{
#ifdef WIN32
    return "Windows";
#elif defined(ANDROID)
    return "Android";
#elif defined(WASM)
    return "WASM";
#else
    static_assert(false);
#endif
}


const OperatingSystemDetails& GetOperatingSystemDetails()
{
    static OperatingSystemDetails details;

    if( details.operating_system.empty() )
    {
        details.operating_system = GetOperatingSystemName();

#ifdef WIN32
        WinRegistry registry;

        if( registry.Open(HKEY_LOCAL_MACHINE, "Software\\Microsoft\\Windows NT\\CurrentVersion") )
        {
            DWORD dwMajorVersion;
            DWORD dwMinorVersion;

            // this should work on Windows 10 and above
            if( registry.ReadDWord("CurrentMajorVersionNumber", &dwMajorVersion) &&
                registry.ReadDWord("CurrentMinorVersionNumber", &dwMinorVersion) )
            {
                details.version_number = FormatText("%lu.%lu", static_cast<unsigned long>(dwMajorVersion),
                                                               static_cast<unsigned long>(dwMinorVersion));
            }

            else
            {
                registry.ReadString("CurrentVersion", details.version_number);
            }

            // read the build number (which will help differentiate Windows 11 from 10)
            details.build_number = registry.ReadOptionalString("CurrentBuildNumber");
        }

        if( details.version_number.empty() )
        {
            WORD wMajorVersion = 4;
            WORD wMinorVersion = 0;

            while( IsWindowsVersionOrGreater(wMajorVersion + 1, 0, 0) )
                ++wMajorVersion;

            while( IsWindowsVersionOrGreater(wMajorVersion, wMinorVersion + 1, 0) )
                ++wMinorVersion;

            details.version_number = FormatText("%d.%d", static_cast<int>(wMajorVersion),
                                                         static_cast<int>(wMinorVersion));
        }

#elif defined(ANDROID)
        details.version_number = PlatformInterface::GetInstance()->GetVersionNumber();

#else
        static_assert_false();
#endif
    }

    return details;
}


#ifdef WIN32
std::string PathGetVolume(const std::string_view path_sv)
{
    wchar_t volume_name[MAX_PATH];
    GetVolumePathName(TC::ToWide(path_sv).c_str(), volume_name, MAX_PATH);
    return TC::ToUtf8(volume_name);
}
#endif


std::string ReplaceInvalidFileChars(std::string filename, const char replace_with_ch)
{
    constexpr const char* InvalidChars = "\\/:?\"<>|*";

    ASSERT(strchr(InvalidChars, replace_with_ch) == nullptr);

    for( char& ch : filename )
    {
        if( strchr(InvalidChars, ch) != nullptr )
            ch = replace_with_ch;
    }

    return filename;
}


char GetUnusedCharacter(const std::string_view text_sv, char starting_ch)
{
    while( text_sv.find(starting_ch) != std::string_view::npos )
        ++starting_ch;

    return starting_ch;
}
