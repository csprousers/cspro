#include "StdAfx.h"
#include "Tools.h"
#include "Special.h"
#include "TextConverter.h"
#include <zPlatformO/PlatformInterface.h>

#ifdef WIN32
#include "WinRegistry.h"
#include <io.h>
#include <lmcons.h>
#include <VersionHelpers.h>
#endif


Encoding GetEncodingFromBOM(int iFileHandle)
{
    Encoding retEncoding = Encoding::Invalid;

    BYTE byte1;
    int bytesRead;
    _lseek (iFileHandle, 0, SEEK_SET); //Go to the begining of the file
    bytesRead = _read (iFileHandle, &byte1, 1);


    // No encoding specified
    if (bytesRead == 0)
        return Encoding::Ansi;

    if ((int)byte1 == EOF)                           // empty file (encoding == 0)
    {
        retEncoding = Encoding::Ansi;
    }

    if (byte1 == 0xFF)                             // UTF16LE?
    {
        BYTE byte2;
        bytesRead = _read (iFileHandle, &byte2, 1);
        if (bytesRead == 0){
            _lseek (iFileHandle, 0, SEEK_SET);
            return Encoding::Invalid;
        }
        if (byte2 == 0xFE)
        {
            retEncoding = Encoding::Utf16LE;
        }
        else
        {
           retEncoding = Encoding::Ansi;
        }
    }

    if (byte1 == 0xFE)                             // UTF16BE?
    {
        BYTE byte2;
        bytesRead = _read (iFileHandle, &byte2, 1);
        if (bytesRead == 0){
            _lseek (iFileHandle, 0, SEEK_SET);
            return Encoding::Invalid;
        }
        if (byte2 == 0xFF)
        {
            retEncoding = Encoding::Utf16BE;
        }
    }

    if (byte1 == 0xEF)                             // UTF8?
    {
        BYTE byte2;
        bytesRead=  _read (iFileHandle, &byte2, 1);
        if (bytesRead == 0){
            _lseek (iFileHandle, 0, SEEK_SET);
            return Encoding::Invalid;
        }
        if (byte2 == 0xBB)
        {
            BYTE byte3;
            bytesRead=  _read (iFileHandle, &byte3, 1);
            if (bytesRead == 0){
                _lseek (iFileHandle, 0, SEEK_SET);
                return Encoding::Invalid;
            }
            if (byte3 == 0xBF)
            {
                retEncoding = Encoding::Utf8;
            }
        }
    }

    if(retEncoding == Encoding::Invalid){ //No BOM so just set it to ANSI??
        retEncoding  = Encoding::Ansi;
    }
    _lseek (iFileHandle, 0, SEEK_SET);

    return retEncoding;
}

Encoding GetEncodingFromBOM(FILE* file)
{
    return GetEncodingFromBOM(_fileno(file));
}


// 20111213 this function opens a file, gets the BOM, and closes the file; returns false if the file can't be opened
bool GetFileBOM(NullTerminatedString filename, Encoding& encoding)
{
    FILE* tempFile = PortableFunctions::FileOpen(filename, _T("rb"), _SH_DENYNO);

    if( tempFile == nullptr )
        return false;

    encoding = GetEncodingFromBOM(tempFile);

    fclose(tempFile);

    return true;
}


const TCHAR* ToString(Encoding encoding)
{
    const TCHAR* const EncodingStrings[] = { _T("Invalid"), _T("ANSI"), _T("UTF-16LE"), _T("UTF-16BE"), _T("UTF-8") };
    return EncodingStrings[static_cast<size_t>(encoding)];
}


// Read a line until \n.
// 20140326 for variable length strings (used by fileread)
bool ReadLine(CFile& cFile, CString* pStr, Encoding encoding)
{
    bool    bRet=true;

    try {
    UINT    nBytes, nTotalBytes=0;
#define BUF256                  256
        char    lpBuffer[BUF256];
#ifdef WIN32
        TCHAR   wBuffer[BUF256];
#else
        std::wstring wBuffer;
#endif
        UINT    wBytes = 0;

        ULONGLONG    lCurrentPos=cFile.GetPosition();

        int             iLen=0;
        int             iPosNewLine=-1;

        while( iPosNewLine == -1 && (nBytes=cFile.Read( lpBuffer, BUF256 )) > 0 ) {

            if( encoding == Encoding::Utf8 )
            {
                if( ( BUF256 - nBytes ) < 4 ) // don't let the buffer end in the middle of a character sequence
                {
                    int goBackChars = 0;

                    while( lpBuffer[nBytes + goBackChars - 1] >> 6 == 2 ) // we're in the middle of a sequence
                        goBackChars--;

                    if( lpBuffer[nBytes + goBackChars - 1] & 0xC0 ) // the beginning of a sequence
                        goBackChars--;

                    if( goBackChars )
                    {
                        cFile.Seek(goBackChars,CFile::current);
                        nBytes += goBackChars;
                    }
                }
#ifdef WIN32
                wBytes = MultiByteToWideChar(CP_UTF8,0,lpBuffer,nBytes,wBuffer,BUF256);
#else
                wBuffer = UTF8Convert::UTF8ToWide(lpBuffer,nBytes);
                wBytes = wBuffer.length();
#endif
            }

            else if( encoding == Encoding::Ansi )
            {
#ifdef WIN32
                wBytes = MultiByteToWideChar(CP_ACP,0,lpBuffer,nBytes,wBuffer,BUF256);
#else
                wBuffer = TextConverter::WindowsAnsiToWide(lpBuffer,nBytes);
                wBytes = wBuffer.length();
#endif
            }

            else
            {
                ASSERT(0); // no other encoding supported
            }

            nTotalBytes += nBytes;

            TCHAR * pBuff = pStr->GetBuffer(iLen + wBytes);

            // first fill pBuff
            for( UINT i = 0; i < wBytes && wBuffer[i] != _T('\n'); i++ )
            {
                if( wBuffer[i] != _T('\r') )
                    pBuff[iLen++] = wBuffer[i];
            }

            // now search for the endline in the ANSI/UTF8 string
            for( UINT i = 0; iPosNewLine == -1 && i < nBytes ; i++ )
            {
                if( lpBuffer[i] == '\n' )
                    iPosNewLine = nTotalBytes - nBytes + i;
            }

        }

        pStr->ReleaseBuffer(iLen);

        if( nTotalBytes == 0 )
            bRet = false;

        // Some newline was found
        if( iPosNewLine != -1 ) {
            cFile.Seek( lCurrentPos+iPosNewLine+1, CFile::begin );
        }
    }
    catch(...) {
        bRet = false;
    }

    return bRet;
}


// Return errorlevel when bWait is used
// Return 1/0 when bWait is false. 1 indicates the program was executed.
bool RunProgram(std::wstring command, int* iRetCode, int iShowWindow, bool bFocus, bool bWait)
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
        if( !CreateProcess( NULL,   // No module name (use command line).
            command.data(),
            NULL,             // Process handle not inheritable.
            NULL,             // Thread handle not inheritable.
            FALSE,            // Set handle inheritance to FALSE.
            0,                // No creation flags.
            NULL,             // Use parent's environment block.
            NULL,             // Use parent's starting directory.
            &si,              // Pointer to STARTUPINFO structure.
            &pi )             // Pointer to PROCESS_INFORMATION structure.
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

    int result_length = std::min(result.GetLength(), MAX_PATH - 1);
    _tcsncpy(pszPath, result, result_length);
    pszPath[result_length] = 0;
}

#endif


bool RecycleFile(NullTerminatedString filename)
{
#ifdef _MFC_VER
    LPCTSTR specs = filename.c_str();

    SHFILEOPSTRUCT info = {NULL};
    TCHAR complete_specs[MAX_PATH+2] = {_T('\0')};
    TCHAR *dummy;
    if (GetFullPathName(specs, _countof(complete_specs), complete_specs, &dummy) != 0)
    {
        info.wFunc = FO_DELETE;
        info.pFrom = complete_specs;
        info.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION| FOF_FILESONLY;
        return SHFileOperation(&info) == 0;
    }
#endif
    return false;
}


std::wstring GetWorkingFolder(wstring_view base_filename)
{
    std::wstring working_folder = PortableFunctions::PathGetDirectory(base_filename);

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
    int length = GetCurrentDirectory(_MAX_PATH, working_folder.data());
    working_folder.resize(length);
#else
    std::wstring working_folder = PlatformInterface::GetInstance()->GetWorkingDirectory();
#endif

    ASSERT(working_folder.length() == ( PortableFunctions::PathEnsureTrailingSlash(working_folder).length() - 1 ));

    return working_folder;
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


template<typename T>
void NormalizePathSlash(T& path)
{
    constexpr TCHAR incorrect_path_slash = OnAndroidOrWasm() ? '\\' : '/';

    if constexpr(std::is_same_v<T, std::wstring>)
    {
        SO::Replace(path, incorrect_path_slash, PATH_CHAR);
    }

    else
    {
        path.Replace(incorrect_path_slash, PATH_CHAR);
    }
}

template CLASS_DECL_ZTOOLSO void NormalizePathSlash(std::wstring& path);
template CLASS_DECL_ZTOOLSO void NormalizePathSlash(CString& path);


// Function name    : MakeFullPath
// Description      : //Makes the full path by Canonicalizing the PathRelativeTo+ Relativepath
//      ex: "d:\code\cspro20\test" + "..\..\test.fmf" = d:\code\test.fmf
// Return type      : CString
// Argument         : CString sRelativeToFName
// Argument         : CString sFileName
std::wstring MakeFullPath(wstring_view relative_to_directory, std::wstring filename)
{
    if( filename.empty() )
        return filename;

    NormalizePathSlash(filename);

    if( PathIsRelative(filename.c_str()) )
        filename.insert(0, std::wstring(SO::TrimRight(relative_to_directory, PATH_CHAR)) + PATH_STRING);

    std::wstring full_path(MAX_PATH, '\0');
    PathCanonicalize(full_path.data(), filename.c_str());
    full_path.resize(_tcslen(full_path.data()));

    return full_path;
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



template<typename T/* = std::wstring*/>
T GetRelativeFNameForDisplay(NullTerminatedString sRelativeToFName, NullTerminatedString sFileName)
{
    constexpr wstring_view StartingInSameDirectoryPrefix(_T(".") PATH_STRING);
    constexpr wstring_view RelativeToPreviousDirectoryPrefix(_T("..") PATH_STRING);

    T relative_filename = GetRelativeFName<T>(sRelativeToFName, sFileName);

    // remove the initial relative path information (if starting in the same directory as the relative filename)
    if( SO::StartsWith(relative_filename, StartingInSameDirectoryPrefix) )
    {
        if constexpr(std::is_same_v<T, std::wstring>)
        {
            return relative_filename.substr(StartingInSameDirectoryPrefix.length());
        }

        else
        {
            return relative_filename.Mid(StartingInSameDirectoryPrefix.length());
        }
    }

    // if sFileName is the directory where sRelativeToFName resides, the relative filename
    // will be something like: ../dir_name, so in that case return ./ instead
    if( SO::StartsWith(relative_filename, RelativeToPreviousDirectoryPrefix) &&
        SO::EqualsNoCase(PortableFunctions::PathGetDirectory<T>(sRelativeToFName), PortableFunctions::PathEnsureTrailingSlash<T>(sFileName)) )
    {
        return StartingInSameDirectoryPrefix;
    }

    return relative_filename;
}

template CLASS_DECL_ZTOOLSO std::wstring GetRelativeFNameForDisplay(NullTerminatedString sRelativeToFName, NullTerminatedString sFileName);
template CLASS_DECL_ZTOOLSO CString GetRelativeFNameForDisplay(NullTerminatedString sRelativeToFName, NullTerminatedString sFileName);


std::wstring EscapeCommandLineArgument(std::wstring argument)
{
    if( argument.find(' ') != std::wstring::npos )
    {
        if( argument.size() < 2 || argument.front() != '"' || argument.back() != '"' )
            return _T("\"") + argument + _T("\"");
    }

    return argument;
}


std::wstring UnescapeCommandLineArgument(std::wstring argument)
{
    if( argument.length() >= 2 && argument.front() == '"' && argument.back() == '"' )
        return argument.substr(1, argument.length() - 2);

    return argument;
}


#ifdef WIN_DESKTOP

std::wstring GetWindowsSpecialFolder(WindowsSpecialFolder folder)
{
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

        if( registry.Open(HKEY_LOCAL_MACHINE, _T("Software\\Microsoft\\Windows\\CurrentVersion")) )
        {
            std::optional<std::wstring> path = registry.ReadOptionalString(_T("ProgramW6432Dir"));

            if( path.has_value() )
                return PortableFunctions::PathEnsureTrailingSlash(std::move(*path));
        }

        // default to the 32-bit version if there is no registry entry
        folder_value = CSIDL_PROGRAM_FILES;
    }

    std::wstring path(MAX_PATH, '\0');
    SHGetSpecialFolderPath(nullptr, path.data(), folder_value, FALSE);
    path.resize(_tcslen(path.data()));

    return PortableFunctions::PathEnsureTrailingSlash(std::move(path));
}


std::vector<std::wstring> GetLogicalDrivesVector()
{
    DWORD drives_text_length = GetLogicalDriveStrings(0, nullptr);
    auto drives_text = std::make_unique<TCHAR[]>(drives_text_length);

    // GetLogicalDriveStrings will return something like C:\[null]G:\[null][null]
    GetLogicalDriveStrings(drives_text_length, drives_text.get());

    std::vector<std::wstring> drives;

    const TCHAR* drives_text_itr = drives_text.get();
    const TCHAR* drives_text_end = drives_text_itr + drives_text_length;

    while( drives_text_itr < drives_text_end && *drives_text_itr != '\0' )
    {
        const std::wstring& drive = drives.emplace_back(drives_text_itr);
        drives_text_itr += drive.size() + 1;
    }

    return drives;
}

#endif // WIN_DESKTOP


const std::wstring& GetDownloadsFolder()
{
    static std::wstring downloads_folder;

    if( downloads_folder.empty() )
    {
#ifdef WIN_DESKTOP
        PWSTR path;

        if( SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Downloads, 0, nullptr, &path)) )
        {
            downloads_folder = path;
            CoTaskMemFree(path);
        }

#else
        downloads_folder = PlatformInterface::GetInstance()->GetDownloadsDirectory();

#endif

        downloads_folder = PortableFunctions::PathEnsureTrailingSlash(downloads_folder);
    }

    return downloads_folder;
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


#ifdef WIN_DESKTOP
#undef PIF_INDEX
#include "Iphlpapi.h"
#pragma comment(lib,"iphlpapi.lib")
#endif

CString GetDeviceId()
{
    // Cache this on first call since this will never change
    static CString csAddress;
    if (!csAddress.IsEmpty())
        return csAddress;

#ifdef WIN_DESKTOP
    DWORD size;

    if (GetAdaptersAddresses(AF_INET, 0, NULL, NULL, &size) == ERROR_BUFFER_OVERFLOW) {
        PIP_ADAPTER_ADDRESSES adapter_addresses = (PIP_ADAPTER_ADDRESSES) malloc(size);

        if (GetAdaptersAddresses(AF_INET, 0, NULL, adapter_addresses, &size) == ERROR_SUCCESS) {
            PIP_ADAPTER_ADDRESSES pAdapterItr = adapter_addresses;

            while (pAdapterItr) {
                if (pAdapterItr->PhysicalAddressLength != 0) {
                    csAddress.Format(_T("%02x%02x%02x%02x%02x%02x"),
                        pAdapterItr->PhysicalAddress[0], pAdapterItr->PhysicalAddress[1],
                        pAdapterItr->PhysicalAddress[2], pAdapterItr->PhysicalAddress[3],
                        pAdapterItr->PhysicalAddress[4], pAdapterItr->PhysicalAddress[5]);
                    break; // if we eventually return more than one, remove this and keep processing the addresses
                }

                pAdapterItr = pAdapterItr->Next;
            }
        }

        free(adapter_addresses);
    }

#else
    csAddress = PlatformInterface::GetInstance()->GetApplicationInterface()->GetDeviceId();
#endif

    return csAddress;
}


CString GetDeviceUserName()
{
#if defined(WIN_DESKTOP) || defined(_CONSOLE)
    TCHAR username[UNLEN + 1];
    DWORD bufferSize = UNLEN + 1;
    GetUserName(username,&bufferSize);
    return CString(username);

#else // 20131209 we'll return the account user name on android devices
    return PlatformInterface::GetInstance()->GetApplicationInterface()->GetUsername();
#endif
}


CString GetLocaleLanguage()
{
#ifdef WIN32
    CString language_name;
    GetUserDefaultLocaleName(language_name.GetBuffer(LOCALE_NAME_MAX_LENGTH), LOCALE_NAME_MAX_LENGTH);
    language_name.ReleaseBuffer();

    // make the string match what Java returns below
    language_name.Replace('-', '_');

    return language_name;

#else
    return PlatformInterface::GetInstance()->GetApplicationInterface()->GetLocaleLanguage();
#endif
}


const OperatingSystemDetails& GetOperatingSystemDetails()
{
    static OperatingSystemDetails details;

    if( details.operating_system.empty() )
    {
        details.operating_system = OnWindows() ? _T("Windows") : _T("Android");

#if defined(WIN_DESKTOP) || defined(_CONSOLE)
        WinRegistry registry;

        if( registry.Open(HKEY_LOCAL_MACHINE,_T("Software\\Microsoft\\Windows NT\\CurrentVersion")) )
        {
            DWORD dwMajorVersion;
            DWORD dwMinorVersion;

            // this should work on Windows 10 and above
            if( registry.ReadDWord(_T("CurrentMajorVersionNumber"), &dwMajorVersion) &&
                registry.ReadDWord(_T("CurrentMinorVersionNumber"), &dwMinorVersion) )
            {
                details.version_number = FormatTextCS2WS(_T("%lu.%lu"), dwMajorVersion, dwMinorVersion);
            }

            else
            {
                registry.ReadString(_T("CurrentVersion"), details.version_number);
            }

            // read the build number (which will help differentiate Windows 11 from 10)
            details.build_number = registry.ReadOptionalString(_T("CurrentBuildNumber"));
        }

        if( details.version_number.empty() )
        {
            WORD wMajorVersion = 4;
            WORD wMinorVersion = 0;

            while( IsWindowsVersionOrGreater(wMajorVersion + 1, 0, 0) )
                ++wMajorVersion;

            while( IsWindowsVersionOrGreater(wMajorVersion, wMinorVersion + 1, 0) )
                ++wMinorVersion;

            details.version_number = FormatTextCS2WS(_T("%d.%d"), wMajorVersion, wMinorVersion);
        }

#elif defined(ANDROID)
        details.version_number = PlatformInterface::GetInstance()->GetVersionNumber();
#endif
    }

    return details;
}


#ifdef WIN32
std::wstring PathGetVolume(NullTerminatedString path)
{
    std::wstring volume_name(MAX_PATH, '\0');
    GetVolumePathName(path.c_str(), volume_name.data(), MAX_PATH);
    volume_name.resize(_tcslen(volume_name.data()));
    return volume_name;
}
#endif

CString ReplaceInvalidFileChars(CString filename, TCHAR replaceWith)
{
    const CString illegalChars = _T("\\/:?\"<>|*");
    for (int i = 0; i < filename.GetLength(); ++i)
    {
        if (illegalChars.Find(filename[i]) != -1)
            filename.SetAt(i, replaceWith);
    }
    return filename;
}

TCHAR GetUnusedCharacter(LPCTSTR lpszText, TCHAR chStartingCharacter/* = '0'*/)
{
    while( _tcschr(lpszText,chStartingCharacter) != nullptr )
        chStartingCharacter++;

    return chStartingCharacter;
}
