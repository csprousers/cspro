//***************************************************************************
//  File name: Interapp.cpp
//
//  Description:
//       Stuff for inter-application communication
//
//  History:    Date       Author     Comment
//              -----------------------------
//              03 Jul 96   CSC       created
//              10 Jul 96   GSF       added IMSASpawnApp
//              11 Jul 96   CSC       added IMSAOpenSharedFile
//              19 Jul 96   CSC/GSF   added DD <-> QT handshaking messages
//              25 Jul 96   CSC       added IMSAGetAppName
//              01 Aug 96   CSC       subsumed proj folder into InterApp
//              16 Sep 96   CSC       IMSASendMessage prototype changed to: uParam=0 default
//              09 Oct 96   GSF       added CIMSAGetXTabLogFileName
//              08 Feb 97   BMD       added CIMSAGet???FileName
//
//***************************************************************************

#include "StdAfx.h"
#include "Interapp.h"
#include "CSProExecutables.h"
#include "CustomUri.h"
#include <zPlatformO/PlatformInterface.h>

#ifdef _CONSOLE
#include <Shlobj.h>
#endif

#ifdef WIN_DESKTOP

/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSA40CommandLineInfo::ParseParam
//
/////////////////////////////////////////////////////////////////////////////
void CIMSACommandLineInfo::ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
{
    if (bFlag)  {
        if (lstrcmp(pszParam,_T("c")) == 0)  {
            m_bChildApp = TRUE;
        }
    }
    else {
        m_acsFileName.Add(pszParam);
    }
    CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
}


std::vector<std::wstring> GetPathsFromCommandLine()
{
    std::vector<std::wstring> paths;

    for( int i = 1; i < __argc; ++i )
    {
        const wchar_t* const path = __wargv[i];
        ASSERT(path != nullptr);

        // ignore flags
        if( *path == '-' || *path == '/' )
            continue;

        paths.emplace_back(MakeFullPath(GetWorkingFolder(), path));
    }

    return paths;
}


std::vector<std::string> GetArgumentsFromCommandLineWithCSProUriSupport()
{
    std::vector<std::string> arguments;

    for( int i = 1; i < __argc; ++i )
    {
        const wchar_t* const arg = __wargv[i];
        ASSERT(arg != nullptr);

        // ignore flags
        if( *arg == '-' || *arg == '/' )
            continue;

        std::string argument = TC::ToUtf8(arg);

        if( CustomUri::UsesCSProScheme(argument) )
        {
            arguments.emplace_back(std::move(argument));
        }

        else
        {
            arguments.emplace_back(MakeFullPath(GetWorkingDirectory(), std::move(argument)));
        }
    }

    return arguments;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           IMSASpawnApp
//
//          exe_path = full path name of app to spawn
//          csWindow = registered window name of app to spawn
//          csFileName = full path name of file for app to open
//          bChild = spawn as "child"? (" /c" option)
//
//          if app is already running, send message to open file
//          if app is not running, spawn it
//
//          return code >=32 if successful
//
/////////////////////////////////////////////////////////////////////////////

void IMSASpawnApp(const std::wstring& exe_path, CString csWindow, CString csFileName, BOOL bChild)
{
    ASSERT(!exe_path.empty());

    if (!PortableFunctions::FileExists(exe_path))  {
        CString csMsg;
        CString csLoad;
        csLoad.LoadString(IDS_CANTFINDAPP);
        csMsg.Format(csLoad, exe_path.c_str());
        AfxMessageBox(csMsg,MB_OK|MB_ICONSTOP);
        return;
    }

    CString csParm; // 20111213

    bool bCSProProgramCandidate = !csWindow.IsEmpty();

    CWnd* pTargetWnd = NULL;
    //if (!csWindow.IsEmpty())  {
    if( bCSProProgramCandidate ) {
        pTargetWnd = AfxGetMainWnd()->FindWindow(csWindow, NULL);
    }
    if (pTargetWnd != NULL)  {
        if (csFileName.GetLength() > 0)  {
            IMSASendMessage(csWindow, WM_IMSA_FILEOPEN, csFileName);
        }
        else  {
            pTargetWnd->SendMessage(WM_IMSA_SETFOCUS);
        }
        return;
    }
    else  {
        // App is *not* active, launch it!
        if (csFileName[0] == '\"') {             // BMD  17 Jul 2001
            //csApp += _T(" ") + csFileName;
            csParm = csFileName;
        }
        else {
            // csApp += _T(" \"")+ csFileName +_T("\"");
            csParm = _T(" \"") + csFileName + _T("\"");
        }
        if (bChild) {
            //csApp +=_T(" /c");
            csParm += _T(" /c");
        }

        //Savy replaced winexec with ShellExecute for unicode
        //return ::WinExec((char*) csApp, SW_SHOW);
        HINSTANCE retCode = ShellExecute(NULL, NULL, exe_path.c_str(), csParm, NULL, SW_SHOW);

        if( bCSProProgramCandidate )
        {
            // Savy added this code to wait for the ShellExecute delay in launching an App.
            // We wait a max of 200 milliseconds to check if the process window is launched.
            if( retCode > reinterpret_cast<HINSTANCE>(32) )
            {
                DWORD startTime = GetTickCount();
                const int MAX_WAIT_TIME = 1000; // 20120809 made a higher number // 200;

                while( !AfxGetMainWnd()->FindWindow(csWindow,NULL) )
                {
                    if( GetTickCount() - startTime > MAX_WAIT_TIME )
                        break;
                }
            }
        }
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//                           IMSA40SendMessage
//
//  Sends a message to another IMSA 4.0 application
//
//          csWindow = registered window name of the app to receive the message
//          uMsg = message to send
//          csParam = CString parameter to send
//          uParam = UINT parameter to send
//
//          return code indicates whether or not the message was sent (if the
//          app was not running, then the return value will be FALSE).
//
/////////////////////////////////////////////////////////////////////////////

BOOL IMSASendMessage(const CString& csWindow, const UINT uMsg, const wstring_view param_sv)
{
    CWnd* const pTargetWnd = AfxGetMainWnd()->FindWindow(csWindow, NULL);

    if( pTargetWnd == nullptr )
        return FALSE;

    // App is alive, send it a message to open the file ...
    // see MSJ "Windows Q&A" from 8/94 (on MSDN) for info about the following
    HANDLE hFileMap;
    SECURITY_ATTRIBUTES sa;
    LPTSTR szInitDataBuf;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = FALSE;

    // Create the file-mapping object backed by the system's
    // paging file. The size should match the amount of data
    // we want to transfer to the child process.
    hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, &sa, PAGE_READWRITE, 0,
                                 uint32_cast(( param_sv.length() + 1 ) * sizeof(TCHAR)), IMSA_SHARED_MEMFILE);
    ASSERT(hFileMap != NULL);

    // Map a view of the file-mapping object so that we can
    // initialize the data we wish to transfer.
    szInitDataBuf = (LPTSTR) MapViewOfFile(hFileMap, FILE_MAP_WRITE, 0, 0, 0);
    ASSERT(szInitDataBuf != NULL);

    // Copy the data we wish to transfer.
    memcpy(szInitDataBuf, param_sv.data(), param_sv.length() * sizeof(TCHAR));
    UnmapViewOfFile(szInitDataBuf);

    // Unmap the view of the file-mapping object so that
    // we free the address space region.
    pTargetWnd->SendMessage(uMsg);
    CloseHandle(hFileMap);

    return TRUE;
}

BOOL IMSASendMessage(const CString& csWindow, UINT uMsg, UINT uParam /*=0*/)  {
    CWnd* pTargetWnd = AfxGetMainWnd()->FindWindow(csWindow, NULL);
    if (pTargetWnd)  {
        pTargetWnd->SendMessage(uMsg, uParam);
        return TRUE;
    }
    return FALSE;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           IMSAOpenSharedFile
//
//  Opens a shared memory file, in response to a WM_IMSA* message.
//  Used for inter-app communication (file opens and closes, etc).
//
//          csContents - will contain the contents of the shared
//                       memory file (usually a file name)
//
//          return code indicates success or failure (nothing found)
//
/////////////////////////////////////////////////////////////////////////////

BOOL IMSAOpenSharedFile(CString& csContents)  {
    HANDLE hFileMap;
    TCHAR* pszFileName;

    // Map a view of the file-mapping object into our process's address space.
    hFileMap = OpenFileMapping(FILE_MAP_READ, FALSE, IMSA_SHARED_MEMFILE);
    ASSERT(hFileMap != NULL);
    pszFileName = (LPTSTR) MapViewOfFile(hFileMap, FILE_MAP_READ, 0, 0, 0);

    csContents.Empty();
    if (pszFileName[0] == NULL)  {
        TRACE(_T("IMSAOpenSharedFile warning:  inter-app message received with no file name specified\n"));
    }
    else  {
//      ASSERT(strlen(pszFileName) < 100);
        csContents = pszFileName;
        UnmapViewOfFile(pszFileName);
        CloseHandle(hFileMap);
    }
    return (!csContents.IsEmpty());
}


/////////////////////////////////////////////////////////////////////////////
//
//                           IMSAGetProjectDir
//                           IMSASetProjectDir
//
//  Returns or assigns the current project directory (from IMSA40.INI)
//
/////////////////////////////////////////////////////////////////////////////

CString IMSAGetProjectDir(BOOL bSet /*=TRUE*/)  {

    TCHAR pszIMSAPath[_MAX_PATH];
    GetPrivateProfileString(IMSA_PROJ_HEADING, IMSA_PROJ_DIR, IMSA_DEFAULT_PROJ_DIR, pszIMSAPath, _MAX_PATH, IMSA_INI);
    if (bSet)  {
        SetCurrentDirectory (pszIMSAPath);
    }
    CString csRetVal(pszIMSAPath);
    return csRetVal;
}

void IMSASetProjectDir(const CString& csPath)  {

    WritePrivateProfileString(IMSA_PROJ_HEADING, IMSA_PROJ_DIR, csPath, IMSA_INI);
}


/////////////////////////////////////////////////////////////////////////////
//
//                           IMSAGetDataDir
//                           IMSASetDataDir
//
//  Returns or assigns the current data directory (from IMSA40.INI)
//
/////////////////////////////////////////////////////////////////////////////

CString IMSAGetDataDir(BOOL bSet /*=TRUE*/)  {

    TCHAR pszIMSAPath[_MAX_PATH];
    GetPrivateProfileString(IMSA_PROJ_HEADING, IMSA_DATA_DIR, IMSA_DEFAULT_PROJ_DIR, pszIMSAPath, _MAX_PATH, IMSA_INI);
    if (bSet)  {
        SetCurrentDirectory (pszIMSAPath);
    }
    CString csRetVal(pszIMSAPath);
    return csRetVal;
}

void IMSASetDataDir(const CString& csPath)  {

    WritePrivateProfileString(IMSA_PROJ_HEADING, IMSA_DATA_DIR, csPath, IMSA_INI);
}

namespace {
    // Callback function for EnumThreadWindows used by GetThreadMainWindow
    BOOL CALLBACK GetThreadHwndEnumCallback(HWND hWnd, LPARAM lParam)
    {
        // The docs say that EnumThreadWindows should only get top-level
        // windows but with WinForms apps at least it returns various others.
        // To get the top level window it should not have an owner,
        // it should be visible and it should be the root
        HWND owner = ::GetWindow(hWnd, GW_OWNER);
        BOOL visible = IsWindowVisible(hWnd);
        HWND root = GetAncestor(hWnd, GA_ROOTOWNER);
        if (owner == 0 && visible && root == hWnd) {
            HWND* foundWindow = (HWND*)lParam;
            *foundWindow = hWnd;
            return FALSE; // Found, stop enumerating
        }
        else {
            return TRUE; // Keep enumerating
        }
    }
};

// Get handle of main window for a given thread
HWND GetThreadMainWindow(DWORD threadId)
{
    HWND result = nullptr;
    EnumThreadWindows(threadId, GetThreadHwndEnumCallback, (LPARAM)& result);
    return result;
}


void CloseFileInTextViewer(const InterfaceString file_path, bool delete_file)
{
    if( PortableFunctions::FileIsRegular(file_path) )
    {
        IMSASendMessage(IMSA_WNDCLASS_TEXTVIEW, WM_IMSA_FILECLOSE, file_path.GetStringView());

        if( delete_file )
            PortableFunctions::FileDelete(file_path);
    }
}


void ViewFileInTextViewer(InterfaceString file_path)
{
    if( !PortableFunctions::FileIsRegular(file_path) )
        return;

    const std::optional<std::string> textviewer_exe = CSProExecutables::GetExecutablePath(CSProExecutables::Program::TextView);

    if( textviewer_exe.has_value() )
        IMSASpawnApp(UTF8_TODO::GetWide(*textviewer_exe), IMSA_WNDCLASS_TEXTVIEW, WS2CS(file_path.Release()), TRUE);
}

#endif


void SetupEnvironmentToCreateFile(InterfaceString file_path)
{
    // make sure the directory for this file exists
    FileIO::CreateDirectoriesForFile(file_path);

#ifdef WIN_DESKTOP
    CloseFileInTextViewer(std::move(file_path), false);
#endif
}


const std::string& GetTempDirectory()
{
    static const std::string temp_directory =
        []()
        {
#ifdef WIN32
            wchar_t path[MAX_PATH];
            GetTempPath(MAX_PATH, path);
            return TC::ToUtf8(path);
#else
            return PlatformInterface::GetInstance()->GetTempDirectory();
#endif
        }();

    ASSERT(PortableFunctions::PathEnsureTrailingSlash(PortableFunctions::PathToNativeSlash(temp_directory)) == temp_directory);

    return temp_directory;
}


std::string GetUniqueTempFilePath(const std::string_view base_filename_sv, const bool overwrite_hour_old_files/* = false*/)
{
    ASSERT(!base_filename_sv.empty());
    ASSERT(PortableFunctions::PathGetDirectory(base_filename_sv).empty());

    const std::string& directory = GetTempDirectory();
    ASSERT(directory == PortableFunctions::PathEnsureTrailingSlash(directory));

    const std::string filename_without_extension = PortableFunctions::PathRemoveFileExtension(base_filename_sv);
    const std::string extension = PortableFunctions::PathGetFileExtension(base_filename_sv, true);

    // create a unique name
    for( int i = 0; ; ++i )
    {
        std::string test_file_path = SO::Concatenate(directory,
                                                     filename_without_extension,
                                                     ( i > 0 ) ? IntToString(i) : std::string(),
                                                     extension);

        if( !PortableFunctions::FileExists(test_file_path) ||
            ( overwrite_hour_old_files && ( GetTimestamp() - PortableFunctions::FileModifiedTime(test_file_path) ) > DateHelper::SecondsInHour<int64_t>() ) )
        {
            return test_file_path;
        }

        if( i > 20000 )
        {
            // this should never happen but is a safeguard to being in an infinite loop
            return ReturnProgrammingError(std::string());
        }
    }
}


const std::string& GetAppDataPath()
{
    static const std::string app_data_path =
        []()
        {
#ifdef WIN32
            wchar_t path[MAX_PATH];

            if( SUCCEEDED(SHGetFolderPath(nullptr, CSIDL_APPDATA | CSIDL_FLAG_CREATE, nullptr, 0, path)) )
            {
                std::string full_path = Path::Combine(TC::ToUtf8(path),
                                                      OnWindowsDesktop() ? "CSPro" : "CSEntryConsole");

                PortableFunctions::PathMakeDirectory(full_path);

                return full_path;
            }

            // in case the AppData folder can't be found, use the temp directory
            else
            {
                return GetTempDirectory();
            }
#else
            return PlatformInterface::GetInstance()->GetInternalStorageDirectory();
#endif
        }();

    ASSERT(!app_data_path.empty());

    return app_data_path;
}

/////////////////////////////////////////////////////////////////////////////
//
//                            GetFilePath
//
/////////////////////////////////////////////////////////////////////////////

CString GetFilePath(CString csFileName)
{
    // this function sometimes failed if it had relative paths in it
    CString sFullPath;
    PathCanonicalize(sFullPath.GetBuffer(MAX_PATH),csFileName);
    sFullPath.ReleaseBuffer();

    csFileName = sFullPath;

    CString csReturn;
    int i = csFileName.ReverseFind(PATH_CHAR);
    if (i < 0) {
        if (csFileName.GetLength() > 1) {
            if (csFileName[1] == COLON) {
                csReturn = csFileName.Left(2);
            }
            else {
                csReturn =_T("");
            }
        }
        else {
            csReturn =_T("");
        }
    }
    else {
        csReturn = csFileName.Left(i);
    }
    return csReturn;
}


/////////////////////////////////////////////////////////////////////////////
//
//                            GetFileName
//
/////////////////////////////////////////////////////////////////////////////

CString GetFileName(CString csFileName)
{
    //FABN Aug 22, 2003
    csFileName.Replace(_T('/'),'\\');

    CString csReturn;
    int i = csFileName.ReverseFind('\\');
    if (i < 0) {
        if (csFileName.GetLength() > 1) {
            if (csFileName[1] == COLON) {
                csReturn = csFileName.Mid(2);
            }
            else {
                csReturn = csFileName;
            }
        }
        else {
            csReturn = csFileName;
        }
    }
    else {
        csReturn = csFileName.Mid(i + 1);
    }
    return csReturn;
}



#ifdef _AFX

CString ValFromHeader(const CSpecFile& specFile, const CString& csAttribute) // JSON_TODO should not be needed when done
{
    CString sValue;
    CString csCmd;    // the string command  (left side of =)
    CString csArg;    // the string argument (right side of =)
    BOOL bFirst = TRUE;

    CSpecFile& spFile = const_cast<CSpecFile&>(specFile);
    spFile.SeekToBegin(); //Start from the begining of the file

    while (spFile.GetLine(csCmd, csArg) == SF_OK) {
        ASSERT(!csCmd.IsEmpty());

        if (csCmd[0] == '[')  {    //Extract only the header
            if(bFirst) {
                bFirst = FALSE;
            }
            else {
                break;
            }
        }

        if(csCmd == csAttribute) {
            sValue = csArg;
            break;
        }
    }

    return sValue;
}

#endif // _AFX


/////////////////////////////////////////////////////////////////////////////
//
//                             GetInputDictsFromSpecFile
//
//      This routine reads a spec file from the beginning
//      It stuffs the full path names of the input dictionaries into pArrDictName
//
//      You should open the spec file, call this, then close the spec file.
//              //if the path is relative then it gives the full path relative to the specfile
/////////////////////////////////////////////////////////////////////////////

std::vector<std::string> GetFileNameArrayFromSpecFile(CSpecFile& specFile, const std::wstring_view section_name_sv)
{
    std::vector<std::string> file_paths;
    bool found_section = false;

    CString csCmd;     // the string command  (left side of =)
    CIMSAString csArg; // the string argument (right side of =)

    specFile.SeekToBegin(); //Start from the beginning of the file

    while (specFile.GetLine(csCmd, csArg) == SF_OK) {
        ASSERT (!csCmd.IsEmpty());

        if (csCmd[0] == '[')  {
            // we are starting a new section
            if (SO::EqualsNoCase(csCmd, section_name_sv)) {
                found_section = true;
                break;
            }
            specFile.SkipSection();
        }
    }

    if( found_section ) {
        while (specFile.GetLine(csCmd, csArg) == SF_OK)  {
            ASSERT (!csCmd.IsEmpty());
            if (csCmd[0] == '[')  {     // we are starting a new section ...
                specFile.UngetLine();
                break;
            }
            else if (csCmd.CompareNoCase(_T("File")) == 0) {

                if(csArg.Find(_T(",")) >= 0)
                    csArg.GetToken(_T(",")); //ignore old file format that contains datetime

                CString sFileName = csArg;
                sFileName.Trim();
                sFileName = specFile.EvaluateRelativeFilename(sFileName);
                file_paths.emplace_back(UTF8_TODO::GetUtf8(sFileName));
            }
            else {
                ErrorMessage::Display(FormatText(L"Invalid line at %d\n%s", specFile.GetLineNumber(), csCmd.GetString()));
            }
        }
    }

    return file_paths;
}


// extract version number of CSPro version string (i.e. the 3.3 of "CSPro 3.3")
// returns -1 if unable to extract a valid number or string doesn't start w. CSPro
double GetCSProVersionNumeric(const std::string_view version_text_sv)
{
    constexpr std::string_view CSProPrefix_sv = "CSPro ";

    if( SO::StartsWithNoCase(version_text_sv, CSProPrefix_sv) )
        return CIMSAString::fVal(version_text_sv.substr(CSProPrefix_sv.length()));

    return -1;
}


// return true if version string is valid CSPro version.
// Must be well formed, i.e. "CSPro X.X" and the version number must be
// greater than or equal to minVersion and less than or equal to the current version
// as defined by Versioning::CSProVersionText.
bool IsValidCSProVersion(const std::string_view version_text_sv, const double min_version/* = 2.0*/)
{
    const double d = GetCSProVersionNumeric(version_text_sv);
    return ( d >= min_version && d <= Versioning::Number );
}


const std::string& Html::GetDirectory()
{
    static const std::string html_directory =
        []()
        {
#if defined(_DEBUG) && defined(WIN_DESKTOP)
            // the html directory is copied to the executables folder in a post build event,
            // but in case other DLLs are being worked on that depend on the contents of that folder,
            // use the direct folder while in debug mode
            return MakeFullPath(PortableFunctions::PathGetDirectory(__FILE__), "..\\html");
#else
            return Path::Combine(CSProExecutables::GetApplicationOrAssetsDirectory(), "html");
#endif
        }();

    return html_directory;
}


std::string Html::GetDirectory(const Subdirectory html_subdirectory)
{
    return Path::Combine(GetDirectory(),
        ( html_subdirectory == Subdirectory::Charting )          ? "charting" :
        ( html_subdirectory == Subdirectory::CSS )               ? "css" :
        ( html_subdirectory == Subdirectory::Dialogs )           ? "dialogs" :
        ( html_subdirectory == Subdirectory::Document )          ? "document" :
        ( html_subdirectory == Subdirectory::HtmlEditor )        ? "html-editor" :
        ( html_subdirectory == Subdirectory::Images )            ? "images" :
        ( html_subdirectory == Subdirectory::Mapping )           ? "mapping" :
        ( html_subdirectory == Subdirectory::Mustache )          ? PortableFunctions::PathToNativeSlash("external\\mustache").c_str() :
        ( html_subdirectory == Subdirectory::QuestionnaireView ) ? "questionnaire-view" :
        ( html_subdirectory == Subdirectory::Runtime )           ? "runtime" :
        ( html_subdirectory == Subdirectory::Templates  )        ? "templates" :
        ( html_subdirectory == Subdirectory::Utilities  )        ? "utilities" :
      /*( html_subdirectory == Subdirectory::Visualizations )*/    "visualizations");
}


std::string Html::GetCSSFilePath(const CSS css)
{
    return Path::Combine(GetDirectory(Subdirectory::CSS), ( css == CSS::CaseView ) ? "case-view.css" :
                                                          ( css == CSS::Common )   ? "common.css" :
                                                        /*( css == CSS::Markdown )*/ "markdown.css");
}


const std::string& Html::GetCSS(const CSS css)
{
    static std::map<CSS, std::string> css_contents;
    const auto& css_lookup = css_contents.find(css);

    if( css_lookup != css_contents.cend() )
        return css_lookup->second;

    // load the CSS if it hasn't already been loaded
    try
    {
        return css_contents.try_emplace(css, FileIO::ReadText(GetCSSFilePath(css))).first->second;
    }

    catch(...)
    {
        return ReturnProgrammingError(SO::Empty_string);
    }
}
