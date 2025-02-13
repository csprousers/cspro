#include "StdAfx.h"
#include "Versioning.h"


#define CSPRO_RELEASE_DATE 20240319

#define BETA_DESIGNATION   _T("beta")

// to override the version that appears in the UI (but not serialized files), uncomment the next line
// #define OVERRIDE_CSPRO_VERSION_NUMBER_DETAILED_TEXT _T("x.x.x")


int Versioning::GetReleaseDate()
{
    return CSPRO_RELEASE_DATE;
}


static CString GetVersionString(CString version, bool include_cspro)
{
#ifdef OVERRIDE_VERSION_DETAILED_NUMBER
    version = CString(OVERRIDE_VERSION_DETAILED_NUMBER).Left(version.GetLength());
#endif

    if( include_cspro )
        version.Insert(0, _T("CSPro "));

    if constexpr(IsBetaBuild())
        version.AppendFormat(_T(" (%s)"), BETA_DESIGNATION);

    return version;
}

CString Versioning::GetVersionString(bool include_cspro/* = false*/)
{
    return ::GetVersionString(CSPRO_VERSION_NUMBER_TEXT, include_cspro);
}

CString Versioning::GetVersionDetailedString(bool include_cspro/* = false*/)
{
    return ::GetVersionString(CSPRO_VERSION_NUMBER_DETAILED_TEXT, include_cspro);
}


CString Versioning::GetReleaseDateString()
{
    CString csVersion;

#ifdef WIN_DESKTOP
    #define DATE_FORMATTER _T("%d %B %Y")
#else
    #define DATE_FORMATTER _T("%d %b %Y")
#endif

#if defined(_DEBUG) && defined(WIN_DESKTOP)
    CFileStatus status;
    _TCHAR acExe[_MAX_PATH];
    GetModuleFileName(NULL,acExe,_MAX_PATH);
    CFile::GetStatus(acExe,status);
    csVersion = status.m_mtime.Format(DATE_FORMATTER _T(" (debug build)"));
#else
    const int StringBufferSize = 25;
    std::tm tm;
    ReadableTimeToTm(&tm,GetReleaseDate(),0);
    wcsftime(csVersion.GetBufferSetLength(StringBufferSize),StringBufferSize,DATE_FORMATTER,&tm);
    csVersion.ReleaseBuffer();
#endif

#ifdef WIN_DESKTOP
    // if the shift key is pressed, display the build date/time instead
    if( ( GetKeyState(VK_SHIFT) & 0x8000 ) != 0 )
        csVersion.Format(_T("%s %s"), (LPCTSTR)CString(__DATE__), (LPCTSTR)CString(__TIME__));
#endif

    return csVersion;
}
