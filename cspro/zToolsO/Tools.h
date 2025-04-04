#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/DateTime.h>
#include <zToolsO/PortableFunctions.h>
#include <zToolsO/NumberToString.h>
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


// TEXT_ENCODING_TODO the below should move to TextEncoding.h
enum class Encoding : int { Invalid, Ansi, Utf16LE, Utf16BE, Utf8 };

CLASS_DECL_ZTOOLSO Encoding GetEncodingFromBOM(int iFileHandle);
CLASS_DECL_ZTOOLSO Encoding GetEncodingFromBOM(FILE* file);
CLASS_DECL_ZTOOLSO bool GetFileBOM(InterfaceString file_path, Encoding& encoding);

CLASS_DECL_ZTOOLSO const char* ToString(Encoding encoding);
// TEXT_ENCODING_TODO the above should move to TextEncoding.h


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

CLASS_DECL_ZTOOLSO bool RecycleFile(InterfaceString file_path);

CLASS_DECL_ZTOOLSO std::wstring GetWorkingFolder(wstring_view base_filename_sv);
CLASS_DECL_ZTOOLSO std::string GetWorkingDirectory(std::string_view base_filename_sv);
CLASS_DECL_ZTOOLSO std::wstring GetWorkingFolder();
CLASS_DECL_ZTOOLSO std::string GetWorkingDirectory();

CLASS_DECL_ZTOOLSO std::wstring MakeFullPath(wstring_view relative_to_directory_sv, std::wstring filename);
CLASS_DECL_ZTOOLSO std::string MakeFullPath(std::string_view relative_to_directory_sv, std::string filename);

template<typename T = std::wstring>
CLASS_DECL_ZTOOLSO T GetRelativeFName(NullTerminatedString sRelativeToFName, NullTerminatedString sFileName);

CLASS_DECL_ZTOOLSO std::string GetRelativePath(std::string_view relative_to_file_path_sv, std::string_view filename_sv);
CLASS_DECL_ZTOOLSO std::string GetRelativePathForDisplay(cs::string_view_sz relative_to_file_path, cs::string_sz filename);


// wraps the argument in double quotes if a space appears in the argument;
// if the argument comes wrapped in double quotes, the argument is not modified
CLASS_DECL_ZTOOLSO std::string EscapeCommandLineArgument(std::string argument);
CLASS_DECL_ZTOOLSO std::wstring EscapeCommandLineArgument(std::wstring argument);

// if the argument comes wrapped in double quotes, the quotes are removed
CLASS_DECL_ZTOOLSO std::string UnescapeCommandLineArgument(std::string argument);


#ifdef WIN_DESKTOP

enum class WindowsSpecialFolder { Desktop, Windows, Documents, ProgramFiles32, ProgramFiles64 };
CLASS_DECL_ZTOOLSO std::string GetWindowsSpecialFolder(WindowsSpecialFolder folder);

CLASS_DECL_ZTOOLSO std::vector<std::string> GetLogicalDrivesVector();

#endif

CLASS_DECL_ZTOOLSO const std::string& GetDownloadsDirectory();

CLASS_DECL_ZTOOLSO std::string CreateUuid();

// Returns the device ID. On Windows, this is based on the MAC address.
CLASS_DECL_ZTOOLSO const std::string& GetDeviceId();

CLASS_DECL_ZTOOLSO std::string GetDeviceUserName();

// Subtags are separated by either underscores or hyphens.
CLASS_DECL_ZTOOLSO std::string GetLocaleLanguage(bool separate_subtags_by_underscores = true);

#ifdef WIN32

// Wraps the Win32 GetLocaleInfo call.
CLASS_DECL_ZTOOLSO const std::string& GetLocaleInformation(LCTYPE LCType);

#endif


template<typename NT, typename DT>
constexpr double CreateProportion(NT&& numerator, DT&& denominator)
{
    if( denominator == 0 )
        return 0;

    return static_cast<double>(std::forward<NT>(numerator)) / static_cast<double>(std::forward<DT>(denominator));
}


template<typename PT = int, typename NT, typename DT>
constexpr PT CreatePercent(NT&& numerator, DT&& denominator)
{
    return static_cast<PT>(100 * CreateProportion(std::forward<NT>(numerator), std::forward<DT>(denominator)));
}


template<typename T>
constexpr double CreatePercentMultiplier(T number_values)
{
    return 100.0 / std::max(number_values, static_cast<T>(1));
}


struct OperatingSystemDetails
{
    std::string operating_system;
    std::string version_number;
    std::optional<std::string> build_number;
};

CLASS_DECL_ZTOOLSO const char* GetOperatingSystemName();
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
CLASS_DECL_ZTOOLSO std::string PathGetVolume(std::string_view path_sv);
#endif

CLASS_DECL_ZTOOLSO std::string ReplaceInvalidFileChars(std::string filename, char replace_with_ch);

CLASS_DECL_ZTOOLSO char GetUnusedCharacter(std::string_view text_sv, char starting_ch);

template<typename T>
constexpr const char* PluralizeWord(const T& count, const char* const word_for_one = "", const char* const word_for_rest = "s")
{
    return ( count == 1 ) ? word_for_one :
                            word_for_rest;
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
