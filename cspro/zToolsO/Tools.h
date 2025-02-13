#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/DateTime.h>
#include <zToolsO/PortableFunctions.h>
#include <zToolsO/NumberToString.h>
#include <zToolsO/TextFormatter.h>
#include <zToolsO/WindowsDesktopMessage.h>

#ifdef WIN32

#include "shlwapi.h"

#ifdef _CONSOLE
#pragma comment(lib,"shlwapi.lib")
#endif

#endif


CLASS_DECL_ZTOOLSO short trimright(TCHAR* in);
CLASS_DECL_ZTOOLSO short trimleft(TCHAR* in);
CLASS_DECL_ZTOOLSO short trimall(TCHAR* in);
CLASS_DECL_ZTOOLSO void memcpyright(csprochar *bufout, int lenout, csprochar *bufin, int lenin);
CLASS_DECL_ZTOOLSO void strcpymax(csprochar *pszOut, const csprochar *pszIn, int maxlen);
CLASS_DECL_ZTOOLSO int strsizereplace(csprochar *buf, csprochar *in, csprochar *out);
CLASS_DECL_ZTOOLSO short strreplace(csprochar *buf, csprochar *in, csprochar *out);

//FABN Jan 23, 2006
CLASS_DECL_ZTOOLSO CString clearString(CString ss, bool bNum);


enum class Encoding : int { Invalid, Ansi, Utf16LE, Utf16BE, Utf8 };

constexpr std::string_view Utf8BOM_sv = "\xEF\xBB\xBF";

inline bool HasUtf8BOM(const void* buffer, size_t buffer_size)
{
    return ( buffer_size >= Utf8BOM_sv.length() && memcmp(buffer, Utf8BOM_sv.data(), Utf8BOM_sv.length()) == 0 );
}

CLASS_DECL_ZTOOLSO Encoding GetEncodingFromBOM(int iFileHandle);
CLASS_DECL_ZTOOLSO Encoding GetEncodingFromBOM(FILE* file);
CLASS_DECL_ZTOOLSO bool GetFileBOM(NullTerminatedString filename, Encoding& encoding);

CLASS_DECL_ZTOOLSO const TCHAR* ToString(Encoding encoding);


CLASS_DECL_ZTOOLSO bool ReadLine( CFile& cFile, CString * pStr, Encoding encoding );

CLASS_DECL_ZTOOLSO CString DelimitCRLF(CString csText, const TCHAR* crlf_override = nullptr);
CLASS_DECL_ZTOOLSO CString UndelimitCRLF(CString csText, const TCHAR* crlf_override = nullptr);

CLASS_DECL_ZTOOLSO bool RunProgram(std::wstring command, int* iRetCode, int iShowWindow, bool bFocus, bool bWait);

//////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZTOOLSO BinaryGen
{
public:
#ifdef GENERATE_BINARY
    static bool m_bGeneratingBinary;
    static std::wstring m_sBinaryName;
    static const std::wstring& GetBinaryName();
#endif // GENERATE_BINARY
    static bool isGeneratingBinary();
};
//////////////////////////////////////////////////////////////////////////

CLASS_DECL_ZTOOLSO bool RecycleFile(NullTerminatedString filename);

CLASS_DECL_ZTOOLSO std::wstring GetWorkingFolder(wstring_view base_filename);
CLASS_DECL_ZTOOLSO std::wstring GetWorkingFolder();

template<typename T>
CLASS_DECL_ZTOOLSO void NormalizePathSlash(T& path);

CLASS_DECL_ZTOOLSO std::wstring MakeFullPath(wstring_view relative_to_directory, std::wstring filename);

template<typename T = std::wstring>
CLASS_DECL_ZTOOLSO T GetRelativeFName(NullTerminatedString sRelativeToFName, NullTerminatedString sFileName);

template<typename T = std::wstring>
CLASS_DECL_ZTOOLSO T GetRelativeFNameForDisplay(NullTerminatedString sRelativeToFName, NullTerminatedString sFileName);


// wraps the argument in double quotes if a space appears in the argument;
// if the argument comes wrapped in double quotes, the argument is not modified
CLASS_DECL_ZTOOLSO std::wstring EscapeCommandLineArgument(std::wstring argument);

// if the argument comes wrapped in double quotes, the quotes are removed
CLASS_DECL_ZTOOLSO std::wstring UnescapeCommandLineArgument(std::wstring argument);


#ifdef WIN_DESKTOP

enum class WindowsSpecialFolder { Desktop, Windows, Documents, ProgramFiles32, ProgramFiles64 };
CLASS_DECL_ZTOOLSO std::wstring GetWindowsSpecialFolder(WindowsSpecialFolder folder);

CLASS_DECL_ZTOOLSO std::vector<std::wstring> GetLogicalDrivesVector();

#endif

CLASS_DECL_ZTOOLSO const std::wstring& GetDownloadsFolder();

CLASS_DECL_ZTOOLSO std::wstring CreateUuid();

CLASS_DECL_ZTOOLSO CString GetDeviceId();
CLASS_DECL_ZTOOLSO CString GetDeviceUserName();
CLASS_DECL_ZTOOLSO CString GetLocaleLanguage();


template<typename PT = int, typename NT, typename DT>
constexpr PT CreatePercent(NT&& numerator, DT&& denominator)
{
    if( denominator == 0 )
        return 0;

    return static_cast<PT>(100 * ( static_cast<double>(std::forward<NT>(numerator)) /
                                   static_cast<double>(std::forward<DT>(denominator)) ));
}


struct OperatingSystemDetails
{
    std::wstring operating_system;
    std::wstring version_number;
    std::optional<std::wstring> build_number;
};

CLASS_DECL_ZTOOLSO const OperatingSystemDetails& GetOperatingSystemDetails();


inline void InitializeCSProEnvironment() { /* nothing done at the moment*/ }


#ifndef WIN32
CLASS_DECL_ZTOOLSO LPCTSTR PathFindExtension(LPCTSTR lpszPath);
CLASS_DECL_ZTOOLSO void PathRemoveExtension(LPTSTR lpszPath);
CLASS_DECL_ZTOOLSO BOOL PathRemoveFileSpec( csprochar* pszPath );
CLASS_DECL_ZTOOLSO void PathStripPath( csprochar* pszPath );
CLASS_DECL_ZTOOLSO bool PathIsRelative(const TCHAR* lpszPath);
CLASS_DECL_ZTOOLSO BOOL PathCanonicalize( csprochar* lpszDst, const csprochar* lpszSrc );
#endif

#ifdef WIN32
CLASS_DECL_ZTOOLSO std::wstring PathGetVolume(NullTerminatedString path);
#endif

CLASS_DECL_ZTOOLSO CString ReplaceInvalidFileChars(CString filename, TCHAR replaceWith);

CLASS_DECL_ZTOOLSO TCHAR GetUnusedCharacter(LPCTSTR lpszText, TCHAR chStartingCharacter = '0');

template<typename T>
constexpr const TCHAR* PluralizeWord(T count, const TCHAR* word_for_one = _T(""), const TCHAR* word_for_rest = _T("s"))
{
    return ( count == 1 ) ? word_for_one : word_for_rest;
}

template<typename T>
TCHAR SuperscriptDigit(T count) { ASSERT(count >= 0 && count <= 9); return _T("⁰¹²³⁴⁵⁶⁷⁸⁹")[(size_t)count]; }

constexpr bool is_digit(int ch)     { return ( ch >= '0' && ch <= '9' ); }
constexpr bool is_lower(int ch)     { return ( ch >= 'a' && ch <= 'z' ); }
constexpr bool is_upper(int ch)     { return ( ch >= 'A' && ch <= 'Z' ); }
constexpr bool is_alpha(int ch)     { return ( is_lower(ch) || is_upper(ch) ); }
constexpr bool is_alnum(int ch)     { return ( is_alpha(ch) || is_digit(ch) ); }
constexpr bool is_tokch(int ch)     { return ( is_alnum(ch) || ch == '_' ); }
constexpr bool is_quotemark(int ch) { return ( ch == '\'' || ch == '"' ); }
constexpr bool is_crlf(int ch)      { return ( ch == '\n' || ch == '\r' ); }
