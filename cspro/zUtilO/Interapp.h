#pragma once

//***************************************************************************
//  File name: INTERAPP.H
//
//  Description:
//       Stuff for inter-application communication.
//
//  History:    Date       Author   Comment
//              ---------------------------
//              03 Jul 96   csc     created
//              18 Jul 96   csc     standardized messages, etc
//              24 Jul 96   GSF     standardized responses, file names
//              25 Jul 96   CSC     added IMSAGetAppName
//              07 Oct 96   csc     added WM_IMSA40_LOGOPEN
//              09 Oct 96   csc     added WM_IMSA40_FILEOPENDONE
//              08 Feb 97   bmd     added WM_IMSA40_processDONEs
//              17 Feb 97   gsf     added IMSA_???_INFILE
//              09 May 97   gsf     added #define IMSA_EXT_MAPDATA
//              14 May 97   gsf     added IMSA_MODULE_MAPVIEW
//              25 Jun 97   gsf     added shared spec file strings
//              18 Sep 97   gsf     added IMSA_EXT_MAPTEXT and IMSA_EXT_MAPCOMP
//              22 Sep 97   gsf     added many spec file defines
//              29 Oct 97   gsf     added IMSA_LANGUAGE & default ini file strings
//              31 Jan 98   bmd     added IMSA_MODULE_ENTRYDEV and IMSA_MODULE_ENTRYRUN
//              05 Feb 98   bmd     added IsIMSAVersion method to return version number
//              17 Mar 99   gsf     implemented IMSA message numbering convention
//              30 Mar 2000 rhf     added WM_IMSA_WRITECASE
//              08 Jun 2000 RHF     including SetMsg.h
//              01 Dec 00   CSC     added DescribeFont
//              08 Dec 00   bmd     modifed GetExtention and StripExtention
//              08 Dec 00   bmd     added GetFileName and GetFilePath
//              22 Nov 04   csc     added SAFE_DELETE macro
//
//***************************************************************************

#include <zUtilO/zUtilO.h>
#include <zUtilO/FileExtensions.h>
#include <zUtilO/imsaStr.h>
#include <zUtilO/Versioning.h>
#include <zToolsO/Tools.h>

class CSpecFile;


//--- starting message number for between applications (.EXEs) ---//
#define WM_IMSA_INTERAPP            WM_APP+100

//--- inter-process communication messages  ---//
// RESOURCE_TODO ... use UWM scheme
#define WM_IMSA_FILEOPEN                            WM_IMSA_INTERAPP+1
#define WM_IMSA_FILECLOSE                           WM_IMSA_INTERAPP+2
#define WM_IMSA_SETFOCUS                            WM_IMSA_INTERAPP+3
#define WM_IMSA_SET_STATUSBAR_PANE                  WM_IMSA_INTERAPP+5
#define WM_IMSA_WRITECASE                           WM_IMSA_INTERAPP+6
#define WM_IMSA_UPDATE_SYMBOLTBL                    WM_IMSA_INTERAPP+8  //used by zDictF,zOrderF,zFormF to communicate with the aplication
#define WM_IMSA_ENGINEABORT                         WM_IMSA_INTERAPP+9
#define WM_IMSA_ENGINEMSG                           WM_IMSA_INTERAPP+10 // RHF Jan 03, 2001
#define WM_IMSA_TABCONVERT                          WM_IMSA_INTERAPP+12
#define WM_IMSA_EXPORTDONE                          WM_IMSA_INTERAPP+13
#define WM_IMSA_REFRESHFORM                         WM_IMSA_INTERAPP+14 // RHF Mar 05, 2002
#define WM_IMSA_REFRESHPROTECTED                    WM_IMSA_INTERAPP+15 // RHF Mar 05, 2002
#define WM_IMSA_REFRESHCAPIGROUPS                   WM_IMSA_INTERAPP+16 // RHF Nov 20, 2002
#define WM_IMSA_FIELD_BEHAVIOR                      WM_IMSA_INTERAPP+17 // RHF Nov 20, 2002
#define WM_IMSA_FIELD_VISIBILITY                    WM_IMSA_INTERAPP+18 // RHF Nov 20, 2002
#define WM_IMSA_ENTRY_TREEVIEW                      WM_IMSA_INTERAPP+19 // smg jun 12, 2003
#define WM_IMSA_SETSEQUENTIAL                       WM_IMSA_INTERAPP+20
#define WM_IMSA_PARTIAL_SAVE                        WM_IMSA_INTERAPP+21 // RHF Oct 15, 2003
#define WM_IMSA_SETCAPITEXT                         WM_IMSA_INTERAPP+22
#define WM_IMSA_DROPITEM                            WM_IMSA_INTERAPP+23
#define WM_IMSA_CHANGE_INPUT_REPOSITORY             WM_IMSA_INTERAPP+24
#define WM_IMSA_KEY_CHANGED                         WM_IMSA_INTERAPP+25
#define WM_IMSA_WINDOW_TITLE_QUERY                  WM_IMSA_INTERAPP+26
#define WM_IMSA_SYMBOLS_ADDED                       WM_IMSA_INTERAPP+27

#define WM_IMSA_USERBAR_UPDATE                      WM_IMSA_INTERAPP+31 // 20100415
#define WM_IMSA_SET_MESSAGE_OVERRIDES               WM_IMSA_INTERAPP+32 // 20100518
#define WM_IMSA_GET_USER_FONTS                      WM_IMSA_INTERAPP+33 // 20100621
#define WM_IMSA_GPS_DIALOG                          WM_IMSA_INTERAPP+34 // 20110524

#define WM_IMSA_RECONCILE_QSF_FIELD_NAME            WM_IMSA_INTERAPP+35 // 20120710

#define WM_IMSA_GROUP_OCCS_CHANGE                   WM_IMSA_INTERAPP+36 // 20141014

#define WM_IMSA_CONTROL_PARADATA_KEYING_INSTANCE    WM_IMSA_INTERAPP+37

#define WM_IMSA_PORTABLE_ENGINEUI                   WM_IMSA_INTERAPP+38

#define WM_IMSA_PROGRESS_DIALOG_SHOW                WM_IMSA_INTERAPP+39
#define WM_IMSA_PROGRESS_DIALOG_UPDATE              WM_IMSA_INTERAPP+40
#define WM_IMSA_PROGRESS_DIALOG_HIDE                WM_IMSA_INTERAPP+41

#define WM_IMSA_START_ENGINE                        WM_IMSA_INTERAPP+42
#define WM_IMSA_GET_PROCESS_SUMMARY_REPORTER        WM_IMSA_INTERAPP+43

#define WM_IMSA_RECONCILE_QSF_DICT_NAME             WM_IMSA_INTERAPP+44

#define WM_IMSA_CSENTRY_REFRESH_DATA                WM_IMSA_INTERAPP+45


//--- shared memory file name ---//
#define IMSA_SHARED_MEMFILE     _T("IMSA10")

//--- registered windows class names (see CMainFrame::Create)  ---//
#define IMSA_WNDCLASS_CSPRO      _T("IMSACSPro")
#define IMSA_WNDCLASS_CSDOCUMENT _T("IMSACSDocument")
#define IMSA_WNDCLASS_TABLEFRM   _T("IMSATableFrame")
#define IMSA_WNDCLASS_TABLEVIEW  _T("IMSATableView")
#define IMSA_WNDCLASS_TEXTVIEW   _T("IMSATextView")

//--- IMSA ini stuff  ---//
#define IMSA_INI                    _T("MEASURE.INI")
#define IMSA_PROJ_HEADING           _T("Project")
#define IMSA_PROJ_DIR               _T("Folder")
#define IMSA_DATA_DIR               _T("DataFolder")
#define IMSA_DEFAULT_PROJ_DIR       _T("\\MEASURE\\PROJECTS")

/*--- other global file names  ---*/
#define IMSA_FORM_CLIPFILE          _T("$form$.fmf")
#define IMSA_TABLE_CLIPFILE         _T("$table$.xts")


/////////////////////////////////////////////////////////////////////////////
//  Spec File Strings
/////////////////////////////////////////////////////////////////////////////

//--- all spec files  ---//
#define CMD_VERSION                             _T("Version")

//----CSPro Stuff -------//
#define CSPRO_DICTS                             _T("[Dictionaries]")

#define CSPRO_ARG_YES                           _T("Yes")
#define CSPRO_ARG_NO                            _T("No")
#define CSPRO_ARG_CONFIRM                       _T("Confirm")
#define CSPRO_ARG_NOCONFIRM                     _T("NoConfirm")

#define CSPRO_CMD_LABEL                         _T("Label")
#define CSPRO_CMD_NAME                          _T("Name")
#define CSPRO_CMD_RTLROSTERS                    _T("RTLRosters")

#define CSPRO_WNDCLASS_ENTRYFRM                 _T("CSProDEFrame")
#define CSPRO_WNDCLASS_BATCHWND                 _T("CSProBatchWnd")


//---- safe delete macro --------//
template<typename T>
void SAFE_DELETE(T*& p) { delete p; p = nullptr; }


//---- colors -------//
constexpr COLORREF rgbBlack   = RGB(  0,  0,  0);
constexpr COLORREF rgbWhite   = RGB(255,255,255);
constexpr COLORREF rgbVLtGray = RGB(224,224,224);
constexpr COLORREF rgbGray    = RGB(160,160,160);
constexpr COLORREF rgbBlue    = RGB(  0,  0,255);


//---- common measuring stuff -------//
const int TWIPS_PER_INCH = 1440;
const float CM_PER_INCH = 2.54f;
const int TWIPS_PER_POINT = 20;

enum APP_MODE { NO_MODE = -1, ADD_MODE = 0, MODIFY_MODE = 1, VERIFY_MODE };


/////////////////////////////////////////////////////////////////////////////
//
//                             CIMSACommandLineInfo
//
//  This class derives from CCommandLineInfo.  It provides functionality
//  to recognize the  /c  command-line switch, which indicates that the
//  program will run in a "child dependent mode", as called from another
//  IMSA application:
//  - Esc key can simulate ID_APP_EXIT
//  - the toolbar shows an ID_QUICK_QUIT button
//
/////////////////////////////////////////////////////////////////////////////
#ifdef WIN_DESKTOP

class CLASS_DECL_ZUTILO CIMSACommandLineInfo : public CCommandLineInfo
{
public:
    int             m_bChildApp;
    CStringArray    m_acsFileName;

public:
    CIMSACommandLineInfo() : m_bChildApp(FALSE), CCommandLineInfo() { }
    void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast) override;
};

CLASS_DECL_ZUTILO std::vector<std::wstring> GetFilenamesFromCommandLine();


// non-class prototypes
CLASS_DECL_ZUTILO int IMSASpawnApp(const std::wstring& exe_path, CString csWindow, CString csFileName, BOOL bChild);
CLASS_DECL_ZUTILO BOOL IMSASendMessage(const CString& csWindow, UINT uMsg, wstring_view param_sv);
CLASS_DECL_ZUTILO BOOL IMSASendMessage(const CString& csWindow, UINT uMsg, UINT uParam=0);
CLASS_DECL_ZUTILO BOOL IMSAOpenSharedFile(CString& csContents);
CLASS_DECL_ZUTILO CString IMSAGetProjectDir(BOOL bSet=TRUE);
CLASS_DECL_ZUTILO void IMSASetProjectDir(const CString& csPath);
CLASS_DECL_ZUTILO CString IMSAGetDataDir(BOOL bSet=TRUE);
CLASS_DECL_ZUTILO void IMSASetDataDir(const CString& csPath);
CLASS_DECL_ZUTILO void IMSASetDataDir(const CString& csPath);
CLASS_DECL_ZUTILO HWND GetThreadMainWindow(DWORD threadId);

CLASS_DECL_ZUTILO void CloseFileInTextViewer(NullTerminatedString filename, bool delete_file);
CLASS_DECL_ZUTILO void ViewFileInTextViewer(NullTerminatedString filename);

#endif

CLASS_DECL_ZUTILO void SetupEnvironmentToCreateFile(NullTerminatedString filename);


// returns the temporary directory (with a trailing slash)
CLASS_DECL_ZUTILO const std::wstring& GetTempDirectory();

CLASS_DECL_ZUTILO std::wstring GetUniqueTempFilename(NullTerminatedString base_filename, bool overwrite_hour_old_files = false);

CLASS_DECL_ZUTILO const std::wstring& GetAppDataPath();

CLASS_DECL_ZUTILO CString GetFilePath(CString csFileName);
CLASS_DECL_ZUTILO CString GetFileName(CString csFileName);

#ifdef WIN_DESKTOP
CLASS_DECL_ZUTILO CString ValFromHeader(const CSpecFile& specFile, const CString& csAttribute);
#endif

CLASS_DECL_ZUTILO std::vector<std::wstring> GetFileNameArrayFromSpecFile(CSpecFile& specFile, wstring_view section_name);


namespace Html
{
    enum class Subdirectory { Charting, CSS, Dialogs, Document, HtmlEditor, Images, Mustache, QuestionnaireView, Templates, Utilities };
    CLASS_DECL_ZUTILO const std::wstring& GetDirectory();
    CLASS_DECL_ZUTILO std::wstring GetDirectory(Subdirectory html_subdirectory);

    enum class CSS { CaseView, Common };
    CLASS_DECL_ZUTILO const std::wstring& GetCSS(CSS css);

    constexpr const TCHAR* CSProUsersForumUrl = _T("https://www.csprousers.org/forum");
}


// extract version number of CSPro version string (i.e. the 3.3 of "CSPro 3.3")
// returns -1 if unable to extract a valid number or string doesn't start w. CSPro
CLASS_DECL_ZUTILO double GetCSProVersionNumeric(wstring_view version_text);

// return true if version string is valid CSPro version.
// Must be well formed, i.e. "CSPro X.X" and the version number must be
// greater than or equal to minVersion and less than or equal to the current version
// as defined by CSPRO_VERSION.
CLASS_DECL_ZUTILO bool IsValidCSProVersion(wstring_view version_text, double min_version = 2.0);


#ifdef WIN_DESKTOP
#include <zUtilO/WindowsInterapp.h>
#endif
