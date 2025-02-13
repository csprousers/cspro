#pragma once

// Standard system wide includes: Windows.h, MFC, a few std lib classes...
// This should be in every pre-compiled header file.

#ifdef WIN_DESKTOP

// Windows includes
#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif

// Modify the following defines if you have to target a platform prior to the ones specified below.
// Refer to MSDN for the latest info on corresponding values for different platforms.
#ifndef WINVER              // Allow use of features specific to Vista or later.
#define WINVER _WIN32_WINNT_VISTA       // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_WINNT        // Allow use of features specific to Vista or later.
#define _WIN32_WINNT _WIN32_WINNT_VISTA // Change this to the appropriate value to target other versions of Windows.
#endif

#ifndef _WIN32_IE           // Allow use of features specific to IE 6.0 or later.
#define _WIN32_IE 0x0600    // Change this to the appropriate value to target other versions of IE.
#endif
//END -SAVY - For VS2010 upgrade - dropping support for windows 2000 and below

// On Windows Desktop include MFC

#ifndef _SECURE_ATL
#define _SECURE_ATL 1
#endif

//SAVY for now until we get the hotfix
#pragma push_macro("_CHAR_UNSIGNED")
#undef _CHAR_UNSIGNED
#include <afxwin.h>
#pragma pop_macro("_CHAR_UNSIGNED")
//END Savy
//SAVY commented this for CString conversions from wide char stuff .MSDN article "Breaking Changes in ATL 7.0 and MFC 7.0 since Visual C++ 6.0"
//#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS    // some CString constructors will be explicit

// turns off MFC's hiding of some common and often safely ignored warning messages
#define _AFX_ALL_WARNINGS

#include <afxext.h>         // MFC extensions
#include <afxdtctl.h>           // MFC support for Internet Explorer 4 Common Controls
#include <afxdialogex.h>
#include <afxshellmanager.h>

#ifndef _AFX_NO_OLE_SUPPORT
#include <afxole.h>         // MFC OLE classes
#include <afxodlgs.h>       // MFC OLE dialog classes
#include <afxdisp.h>        // MFC OLE automation classes
#endif // _AFX_NO_OLE_SUPPORT

#ifndef _AFX_NO_DB_SUPPORT
#include <afxdb.h>                      // MFC ODBC database classes
#endif // _AFX_NO_DB_SUPPORT

#ifndef _AFX_NO_DAO_SUPPORT
#include <afxdao.h>                     // MFC DAO database classes
#endif // _AFX_NO_DAO_SUPPORT

#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>                     // MFC support for Windows Common Controls
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxtempl.h>

#pragma warning(push)
// temporarily disable warning "declaration of 'identifier' hides class member"
// that is triggered by gdiplus.h
#pragma warning(disable:4458)
#include <gdiplus.h>
#pragma warning(pop)

#endif // WIN_DESKTOP (MFC includes)

#if defined(WIN32) && !defined(WIN_DESKTOP)
// Not on Win Desktop - just regular Windows

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN        // Exclude rarely-used stuff from Windows headers
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
#endif

#include <SDKDDKVer.h>

#endif

#ifdef WIN32

// Get rid of min/max macros that come from MFC/Windows.h to avoid name collisions
// Note that this needs to be done AFTER including gdiplus.h which uses those macros.
#undef min
#undef max

#endif

#ifndef WIN_DESKTOP
// MFC collection classes for non MFC platforms
#include <zPlatformO/PortableMFC.h>
#endif

// Cross platform includes
#include <math.h>
#include <cmath>

#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#ifdef USE_BINARY
#ifdef GENERATE_BINARY
#undef GENERATE_BINARY
#endif // GENERATE_BINARY
#endif // USE_BINARY

// Character set support
#define csprochar TCHAR

#define _tmemset    wmemset
#define _tmemchr    wmemchr
#define _tmemcmp    wmemcmp
#define _tmemcpy    wmemcpy
#define _tCF_TEXT   CF_UNICODETEXT

#define _tofstream std::wofstream
#define _tostream std::wostream

#ifdef WIN32
#define PATH_CHAR   _T('\\')
#define PATH_STRING _T("\\")
#endif

#include <zPlatformO/MobileStringConversion.h>

#ifndef WIN_DESKTOP
// some implementations of classes and functions to reduce the need to conditionally exclude lots of desktop code
#include <zPlatformO/DummyWinDesktopImplementions.h>
#endif


// assert definitions that can be disabled in future versions
#define ASSERT80(f) ASSERT(f)


// for more serious warning checking, make some warnings errors
#ifdef WIN32
#pragma warning(error:4005) // macro redefinition
#pragma warning(error:4150) // deletion of pointer to incomplete type 'type'; no destructor called
#pragma warning(error:4840) // non-portable use of class 'type' as an argument to a variadic function
#endif


// turn off some warnings for the console application
#ifdef _CONSOLE
#pragma warning(disable:4251) // 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
#pragma warning(disable:4275) // non - DLL-interface class 'class_1' used as base for DLL-interface class 'class_2'
#endif


// harmonize the DEBUG and _DEBUG preprocessor definitions
#if defined(DEBUG) && !defined(_DEBUG)
#define _DEBUG
#elif defined(_DEBUG) && !defined(DEBUG)
#define DEBUG
#endif


constexpr bool DebugMode()
{
#ifdef _DEBUG
    return true;
#else
    return false;
#endif
}


#include <zToolsO/assert_cast.h>
#include <zToolsO/CSProException.h>
#include <zToolsO/ErrorMessageDisplayer.h>
#include <zToolsO/OperatingSystem.h>
#include <zToolsO/StandardTemplates.h>
#include <zToolsO/StringOperations.h>
