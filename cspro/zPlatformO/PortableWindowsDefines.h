#pragma once

/*
 * PortableWindowsDefines.h
 * Definitions/types that are built-in on Windows defined for other platforms (Windows.h for non-Windows platforms)
 *  Created on: Jan 4, 2013
 *      Author: SAVY
 */

#ifdef WIN_DESKTOP

#error You should not include this file when compiling for Windows

#else

class CFont;

#ifndef WIN32

#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <wctype.h>

typedef unsigned char byte;
typedef int LONG;
typedef const char*     PCSTR;
typedef char*           PSTR;
typedef const wchar_t*  PCWSTR;
typedef wchar_t*        PWSTR;
#ifdef UNICODE
    typedef wchar_t     TCHAR;
    typedef wchar_t     _TUCHAR;
#else
    typedef char        TCHAR;
#endif
typedef wchar_t         OLECHAR;
#define FAR
#define NEAR
typedef wchar_t WCHAR;
typedef wchar_t* LPWSTR;
typedef wchar_t* LPTSTR;
typedef const wchar_t* LPCTSTR;
#define _W64 //TODO: fix this for cygwin

#define _close      close
#define _fileno     fileno
#define _lseek      lseek
#define _read       read
#define _tell       tell
#define _write      write

#define _stat64     stat64
#define _timezone   timezone

#define _MAX_PATH   260 /* max. length of full pathname */

#if defined(_UNICODE) || defined(UNICODE)
    #define _T(x) L ##x
#else
    #define _T(x) x
#endif

typedef TCHAR csprochar;
typedef struct stat filestat;


#ifdef UNICODE

/* Unformatted i/o */
#define _fgettc         fgetwc
#define _fgettc_nolock  _fgetwc_nolock
#define _fgettchar      _fgetwchar
#define _fgetts         fgetws
#define _fputtc         fputwc
#define _fputtc_nolock  _fputwc_nolock
#define _fputtchar      _fputwchar
#define _fputts         fputws
#define _cputts         _cputws
#define _cgetts         _cgetws
#define _cgetts_s       _cgetws_s
#define _gettc          getwc
#define _gettc_nolock   _getwc_nolock
#define _gettch         _getwch
#define _gettch_nolock  _getwch_nolock
#define _gettche        _getwche
#define _gettche_nolock _getwche_nolock
#define _gettchar       getwchar
#define _gettchar_nolock _getwchar_nolock
#define _getts          _getws
#define _getts_s        _getws_s
#define _puttc          putwc
#define _puttc_nolock   _putwc_nolock
#define _puttchar       putwchar
#define _puttchar_nolock _putwchar_nolock
#define _puttch         _putwch
#define _puttch_nolock  _putwch_nolock
#define _putts          _putws
#define _ungettc        ungetwc
#define _ungettc_nolock _ungetwc_nolock
#define _ungettch       _ungetwch
#define _ungettch_nolock _ungetwch_nolock

/* Stdio functions */

#define _tfdopen    _wfdopen
#define _tfsopen    _wfsopen
#define _tfopen     _wfopen
#define _tfopen_s   _wfopen_s
#define _tfreopen   _wfreopen
#define _tfreopen_s _wfreopen_s
#define _tperror    _wperror
#define _tpopen     _wpopen
#define _ttempnam   _wtempnam
#define _ttmpnam    _wtmpnam
#define _ttmpnam_s  _wtmpnam_s


#define _tmemset    wmemset
#define _tmemchr    wmemchr
#define _tmemcmp    wmemcmp
#define _tmemcpy    wmemcpy
//#define _tCF_TEXT CF_UNICODETEXT
#define _tgetenv        _wgetenv

#define _itot_s     _itow_s
#define _ltot_s     _ltow_s
#define _ultot_s    _ultow_s
#define _itot       _itow
#define _ltot       _ltow
#define _ultot      _ultow
#define _ttoi       _wtoi
#define _ttol       _wtol
#define _tstof      _wtof

#define _tcscat         wcscat
#define _tcscat_s       wcscat_s
#define _tcschr         wcschr
#define _tcscpy         wcscpy
#define _tcscpy_s       wcscpy_s
#define _tcscspn        wcscspn
#define _tcslen         wcslen
#define _tcsnlen        wcsnlen
#define _tcsncat        wcsncat
#define _tcsncat_s      wcsncat_s
#define _tcsncat_l      _wcsncat_l
#define _tcsncat_s_l    _wcsncat_s_l
#define _tcsncpy        wcsncpy
#define _tcsncpy_s      wcsncpy_s
#define _tcsncpy_l      _wcsncpy_l
#define _tcsncpy_s_l    _wcsncpy_s_l
#define _tcspbrk        wcspbrk
#define _tcsrchr        wcsrchr
#define _tcsspn         wcsspn
#define _tcsstr         wcsstr
#define _tcstok         wcstok
#define _tcstok_s       wcstok_s
#define _tcstok_l       _wcstok_l
#define _tcstok_s_l     _wcstok_s_l
#define _tcstoul        wcstoul
#define _tcserror       _wcserror
#define _tcserror_s     _wcserror_s
#define __tcserror      __wcserror
#define __tcserror_s    __wcserror_s

#define _tcsdup         _wcsdup
#define _tcsnset        _wcsnset
#define _tcsnset_s      _wcsnset_s
#define _tcsnset_l      _wcsnset_l
#define _tcsnset_s_l    _wcsnset_s_l
#define _tcsrev         _wcsrev
#define _tcsset         _wcsset
#define _tcsset_s       _wcsset_s
#define _tcsset_l       _wcsset_l
#define _tcsset_s_l     _wcsset_s_l

#define _tcscmp         wcscmp
#define _tcsicmp        wcscasecmp
#define _tcsicmp_l      _wcsicmp_l
#define _tcsnccmp       wcsncmp
#define _tcsncmp        wcsncmp
#define _tcsncicmp      wcsncasecmp
#define _tcsncicmp_l    _wcsnicmp_l
#define _tcsnicmp       wcsncasecmp
#define _tcsnicmp_l     _wcsnicmp_l

#define _tcscoll        wcscoll
#define _tcscoll_l      _wcscoll_l
#define _tcsicoll       _wcsicoll
#define _tcsicoll_l     _wcsicoll_l
#define _tcsnccoll      _wcsncoll
#define _tcsnccoll_l    _wcsncoll_l
#define _tcsncoll       _wcsncoll
#define _tcsncoll_l     _wcsncoll_l
#define _tcsncicoll     _wcsnicoll
#define _tcsncicoll_l   _wcsnicoll_l
#define _tcsnicoll      _wcsnicoll
#define _tcsnicoll_l    _wcsnicoll_l

#define _stscanf        wscanf
#define _tcstol         wcstol

/* ctype functions */

#define _istalnum   iswalnum
#define _istalnum_l   _iswalnum_l
#define _istalpha   iswalpha
#define _istalpha_l   _iswalpha_l
#define _istascii   iswascii
#define _istcntrl   iswcntrl
#define _istcntrl_l   _iswcntrl_l
#define _istdigit   iswdigit
#define _istdigit_l   _iswdigit_l
#define _istgraph   iswgraph
#define _istgraph_l   _iswgraph_l
#define _istlower   iswlower
#define _istlower_l   _iswlower_l
#define _istprint   iswprint
#define _istprint_l   _iswprint_l
#define _istpunct   iswpunct
#define _istpunct_l   _iswpunct_l
#define _istspace   iswspace
#define _istspace_l   _iswspace_l
#define _istupper   iswupper
#define _istupper_l   _iswupper_l
#define _istxdigit  iswxdigit
#define _istxdigit_l  _iswxdigit_l

// 20131207 the crystax version of swprintf doesn't work with unicode characters (there
// must be a problem with the wide to multibyte routines, so replacing it with a different one)
#include <zPlatformO/util_snprintf.h>
#define _stprintf(dest,fmt,...) ap_snprintf(dest,50000,fmt,__VA_ARGS__)
#define _sntprintf(dest,size,fmt,...) ap_snprintf(dest,size,fmt,__VA_ARGS__)


#define _totupper       towupper
#define _totupper_l     _towupper_l
#define _totlower       towlower
#define _totlower_l     _towlower_l

#define _vftprintf      vfwprintf
#define _ftprintf       fwprintf

/* Redundant "logical-character" mappings */

#define _tcsclen        wcslen
#define _tcscnlen       wcsnlen
#define _tcsclen_l(_String, _Locale) wcslen(_String)
#define _tcscnlen_l(_String, _Max_count, _Locale) wcsnlen((_String), (_Max_count))
#define _tcsnccat       wcsncat
#define _tcsnccat_s     wcsncat_s
#define _tcsnccat_l     _wcsncat_l
#define _tcsnccat_s_l   _wcsncat_s_l
#define _tcsnccpy       wcsncpy
#define _tcsnccpy_s     wcsncpy_s
#define _tcsnccpy_l     _wcsncpy_l
#define _tcsnccpy_s_l   _wcsncpy_s_l
#define _tcsncset       _wcsnset
#define _tcsncset_s     _wcsnset_s
#define _tcsncset_l     _wcsnset_l
#define _tcsncset_s_l   _wcsnset_s_l

#define _tcsdec     _wcsdec
#define _tcsinc     _wcsinc
#define _tcsnbcnt   _wcsncnt
#define _tcsnccnt   _wcsncnt
#define _tcsnextc   _wcsnextc
#define _tcsninc    _wcsninc
#define _tcsspnp    _wcsspnp

#define _tcslwr     _wcslwr
#define _tcslwr_l   _wcslwr_l
#define _tcslwr_s   _wcslwr_s
#define _tcslwr_s_l _wcslwr_s_l
#define _tcsxfrm    wcsxfrm
#define _tcsxfrm_l  _wcsxfrm_l
#else
#define _tmemset    memset
#define _tmemchr    memchr
#define _tmemcmp    memcmp
#define _tmemcpy    memcpy

#endif

// the following are from WinDef.h

typedef unsigned int ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef char *PSZ;

#define far
#define near
#define CONST               const

typedef unsigned int        DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef float               FLOAT;
typedef FLOAT               *PFLOAT;
typedef BOOL near           *PBOOL;
typedef BOOL far            *LPBOOL;
typedef BYTE near           *PBYTE;
typedef BYTE far            *LPBYTE;
typedef int near            *PINT;
typedef int far             *LPINT;
typedef WORD near           *PWORD;
typedef WORD far            *LPWORD;
typedef long far            *LPLONG;
typedef DWORD near          *PDWORD;
typedef DWORD far           *LPDWORD;
typedef void far            *LPVOID;
typedef CONST void far      *LPCVOID;
typedef DWORD               COLORREF;

typedef int                 INT;
typedef unsigned int        UINT;
typedef unsigned int        *PUINT;

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

#ifdef TRUE
#undef TRUE
#endif
#define TRUE  1

// the following are from BaseTsd.h
//TODO: Savy fix the int INT_PTR define and _W64 stuff ... X64_TODO review

#define __int64     int64_t

#if INTPTR_MAX == INT64_MAX
    typedef __int64 INT_PTR, *PINT_PTR;
    typedef __uint64_t UINT_PTR, *PUINT_PTR;
    typedef __int64 LONG_PTR, *PLONG_PTR;
    typedef __uint64_t ULONG_PTR, *PULONG_PTR;
#else
    typedef _W64 int INT_PTR, *PINT_PTR;
    typedef _W64 unsigned int UINT_PTR, *PUINT_PTR;
    typedef _W64 long LONG_PTR, *PLONG_PTR;
    typedef _W64 unsigned long ULONG_PTR, *PULONG_PTR;
#endif

typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;
typedef LONG_PTR            LRESULT;

#ifndef SIZE_MAX
    #ifdef _WIN64
        #define SIZE_MAX _UI64_MAX
    #else
        #define SIZE_MAX UINT_MAX
    #endif
#endif

#ifndef MAX_PATH
#define MAX_PATH        260
#endif


#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

typedef long            LONGLONG;
typedef unsigned long   ULONGLONG;

#define MAXLONG     0x7fffffff
#define WM_APP      0x8000

using HKL = void*;

typedef struct _GUID {
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID;

#define _countof(x) (sizeof(x)/sizeof(x[0]))

inline void Sleep(DWORD dwMilliseconds) { usleep(dwMilliseconds * 1000); }

#define TEXT                _T

#define PATH_CHAR           _T('/')
#define PATH_STRING         _T("/")

#define FILE_ATTRIBUTE_NORMAL 0x00000080

typedef UINT_PTR            WPARAM;
typedef LONG_PTR            LPARAM;

#include <zPlatformO/PortableWindowsGUIDefines.h>
#include <zPlatformO/PortableFStream.h>

#endif // !WIN32

#endif // !WIN_DESKTOP
