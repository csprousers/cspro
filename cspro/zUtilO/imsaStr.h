#pragma once

//***************************************************************************
//  File name: IMSAStr.h
//
//  Description:
//       Header for CIMSAString
//                  other string functions
//
//  History:    Date       Author   Comment
//              ---------------------------
//              21 Jan 98   bmd     Created from IMPS 4.1
//              12 Feb 98   bmd     Added DataTime()
//              20 Feb 98   gsf     Added LogfontToText and TextToLogfont
//              27 Feb 98   bmd     Remove IsAlpha()
//              28 Feb 98   bmd     Merged IMPSMisc
//              28 Feb 98   bmd     Modified strtoken to capture separator
//              02 Mar 98   bmd     GetNextWord() --> GetToken()
//              02 Mar 98   bmd     GetNextWordMdf() --> GetToken()
//              02 Mar 98   bmd     PadLeft() --> AdjustLenLeft()
//              02 Mar 98   bmd     PadRight() --> AdjustLenRight()
//              03 Mar 98   bmd     Remove UnquoteDelimit()
//              01 Dec 99   gsf     added AddParens(), StripParens()
//              23 Mar 00   gsf     added bDontStripQuotes parameter to GetToken()
//
//***************************************************************************
//
//  class CIMSAString : public CString
//
//  Description:
//      Add functionality to CString for IMSA.
//
//  Comparison/analysis
//      IsNumeric           Is string a number (can have sign and decimal point).
//      IsName              Is string an IMPS name (IMPS 3.1 and IMPS 4.x).
//      GetLongestWordSize  Get the size (CSize) of the longest word in the string.
//      ReverseFindOneOf    Search for the end of the string of a character in a set.
//      CompareNoCase       Compare string to a resource string given the id.
//
//  Conversion
//      MakeName            Make an IMSA name from a label.
//      MakeNumeric         Convert string to a valid number.
//
//  Adjustment/formatting
//      AdjustLenLeft       Adjust the length of the string on the left, pad if necessary.
//      AdjustLenRight      Adjust the length of the string on the right, pad if necessary.
//      Wrap                Wrap a long string by creating an array of strings of a given size.
//      QuoteDelimit        Put quote marks around a string.
//      GetToken            Get a token from an input stream.
//      AddParens           Adds a number in parens, e.g., "(2)", good for occurrences
//      StripParens         Removes a number in parens from the end
//
//  Numerical conversion
//      Val                 Convert string to an integer value.
//      fVal                Convert string to a floating point value.
//      Str                 Convert value to a string.
//
//  Date/time strings
//      Date                Create a date string for a given CTime.
//      Time                Create a time string for a given CTime.
//      DateTime            Create a date/time string for a given CTime.
//
//***************************************************************************
//***************************************************************************
//
//  BOOL CIMSAString::IsNumeric();
//
//      Return value
//          TRUE if the string is a number, otherwise FALSE.
//
//      Remarks
//          The first character can be a plus or minus sign and
//          there can be a decimal point (period) within the number.
//          All other characters must be the digits from 0 to 9.
//
//---------------------------------------------------------------------------
//
//  bool CIMSAString::IsName();
//
//      Return value
//          TRUE if the string is an IMSA name, otherwise FALSE.
//
//      Remarks
//          An IMSA name is 1 to 16 characters long and consists of
//          letters (A-Z), digits, and imbedded underline (_).
//          The first character must be a letter.
//
//---------------------------------------------------------------------------
//
//  CSize CIMSAString::GetLongestWordSize(CDC* pDC);
//
//      Parameters
//          pDC                 Pointer to a device context for measurement.
//
//      Return value
//          CSize containing the size of the longest word in the string.
//
//---------------------------------------------------------------------------
//
//  int CIMSAString::ReverseFindOneOf(const csprochar* pszSet);
//
//      Parameters
//          pszSet              Set of characters to search for.
//
//      Return value
//          The index of the first occurrence from the end of the string
//          of any one of the characters in the character set.
//          If none of the characters is found, returns -1.
//
//---------------------------------------------------------------------------
//
//  int CIMSAString::CompareNoCase(int iID);
//
//      Parameters
//          iID                 Resource id value.
//
//      Return value
//           0      This strings are identical (ignoring case)
//          -1      This CIMSAString is less than the resource string (ignoring case)
//           1      This CIMSAString is greater than the resource string (ignoring case)
//
//***************************************************************************
//
//  void CIMSAString::MakeName();
//
//      Remarks
//          Make this string into a valid IMSA name.
//          Converts bad characters to underlines (_).
//          Makes the first alphabetic character the first character.
//          Remove trailing underlines (_).
//
//---------------------------------------------------------------------------
//
//    void CIMSAString::MakeNumeric();
//
//      Remarks
//          Uses the characters in the string to make a number.
//          A number can start with a minus sign.  It contains
//          digits (0-9) and can have a single decimal point (period).
//
//**************************************************************************
//
//  CIMSAString CIMSAString::AdjustLenLeft(int iLen, csprochar = SPACE);
//
//      Parameters
//          iLen                Length of the resulting string
//          csprochar                Character to pad with.
//
//      Remarks
//          If iLen is less than the string length, the string is trucated.
//          If iLen is greater than the string length, the string is padded.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CIMSAString::AdjustLenRight(int iLen, csprochar = SPACE);
//
//      Parameters
//          iLen                Length of the resulting string
//          csprochar                Character to pad with.
//
//      Remarks
//          If iLen is less than the string length, the string is trucated.
//          If iLen is greater than the string length, the string is padded.
//
//---------------------------------------------------------------------------
//
//  int CIMSAString::Wrap(CStringArray& acsLine, int iColLeft = 0, int iColRight = 132, BOOL bStripCRLF = FALSE);
//
//      Parameters
//          acsLine             An array of CStrings where the wrapped lines are stored.
//          iColLeft            Left hand margin, column where string starts
//          iColRight           Right hand margin, column where wrapping ends
//          bStripCRLF          Whether to remove \r and \n characters
//                              (used when outputting Note= commands that
//                              were previously formatted for use in a CEdit box)
//
//      Return value
//          Number of lines wrapped, that is size of CStringArray.
//
//---------------------------------------------------------------------------
//
//  void CIMSAString::QuoteDelimit();
//
//      Remarks
//          Will fail if string contains both a single and double quote.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CIMSAString::GetToken(const csprochar* pszSeparators = SPACE_COMMA_STR, csprochar* pcFound = NULL, BOOL bDontStripQuotes = FALSE);
//
//      Parameters
//          pszSeparators       A pointer to a string of all separators considered.
//          pcFound             A pointer to a a character where separator found is stored.
//          bDontStripQuotes    Do not strip leading single and double quote character (gsf 23-mar-00).
//
//      Return value
//          Token found with separators and quotes removed.
//
//      Remarks
//          Single or double quotes can surround the token.
//          If token is last token of the string, the found character will be EOS.
//          If a quote string token is followed immediately be another token,
//          the found character will be the first character in the separator string.
//          The token is removed from the initial CIMSAString.
//          Uses strtoken() [see below]
//
//---------------------------------------------------------------------------
//
//  int64_t CIMSAString::Val();
//
//      Return value
//          An _int64 containing converted value of the string.
//          The string can have leading spaces or tabs.  The first
//          character can be a plus (+) or minus (-) sign.  The
//          remaining characters must be digits (0-9).  The conversion
//          stops on the first non-digit.
//          This function uses atoi64 (see below).
//
//---------------------------------------------------------------------------
//
//  long double CIMSAString::fVal();
//
//      Return value
//          A long double containing converted value of the string.
//          The function stops reading the input string at the
//          first character that it cannot recognize as part of a number.
//          The fuctions uses atof (see C run time functions).
//
//---------------------------------------------------------------------------
//
//  void CIMSAString::Str(int64_t iVal, int iLen = NONE, csprochar cPad = SPACE);
//
//      Parameters
//          iVal                Integer 64 value to be converted.
//          iLen                The length of the string.  If NONE, don't pad.
//          cPad                The character to pad the string on the left.
//
//      Return value
//          Converted string.
//
//      Remarks
//          This function uses i64toa (see below).
//
//---------------------------------------------------------------------------
//
//  void CIMSAString::Str(int iVal, int iLen = NONE, csprochar cPad = SPACE);
//
//      Parameters
//          iVal                Integer value to be converted.
//          iLen                The length of the string.  If NONE, don't pad.
//          cPad                The character to pad the string on the left.
//
//      Return value
//          Converted string.
//
//      Remarks
//          This function uses i64toa (see below).
//
//---------------------------------------------------------------------------
//
//  void CIMSAString::Str(UINT uVal, int iLen = NONE, csprochar cPad = SPACE);
//
//      Parameters
//          iVal                Unsigned integer value to be converted.
//          iLen                The length of the string.  If NONE, don't pad.
//          cPad                The character to pad the string.
//
//      Return value
//          Converted string.
//
//      Remarks
//          This function uses i64toa (see below).
//
//***************************************************************************
//
//  void CIMSAString::Date(CTime time);
//
//      Parameters
//          time                The time to be displayed.
//
//      Remarks
//          Creates a string containing the date, converted according to the system regional
//          short date settings.
//
//---------------------------------------------------------------------------
//
//  void CIMSAString::Time(CTime time);
//
//      Parameters
//          time                The time to be displayed.
//
//      Return value
//          String containing the time, converted according to the system regional
//          time settings.
//
//---------------------------------------------------------------------------
//
//  void CIMSAString::Date();
//
//      Return value
//          String containing the current system date, converted according to
//          the system regional short date settings.
//
//---------------------------------------------------------------------------
//
//  void CIMSAString::Time();
//
//      Return value
//          String containing the current system time, converted according to
//          the system regional time settings.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CIMSAString::DateTime(CTime time);
//
//      Parameters
//          time                The time to be displayed.
//
//      Return value
//          String containing the data and time, converted according to
//          the system regional time settings.
//
//---------------------------------------------------------------------------
//
//  CIMSAString CIMSAString::DateTime();
//
//      Return value
//          String containing the current system date and time, converted according to
//          the system regional time settings.
//
//***************************************************************************
//***************************************************************************
//***************************************************************************
//
//  Description:
//      Additional run time string functions
//
//  Conversions
//      i64toc                  Int64 to character array.
//      atoi64                  Character string to int64.
//      i64toa                  Int64 to character string.
//
//      atod                    Character string to double.
//      dtoa                    Double to character string.
//
//  Tokens
//      strtoken                Tokenize a string with some tokens quote strings
//
//***************************************************************************
//---------------------------------------------------------------------------
//
//  BOOL i64toc (int64_t iValue, csprochar pszValue[], UINT uLen, BOOL bZeroFill = FALSE);
//
//      Function
//          Converts a 64 bit integer to a character array.
//
//      Parameters
//          iValue              Value to be converted.
//          pszValue            Pointer to the beginning of the the character array.
//          uLen                Length of the character array.
//          bZeroFill           Whether the result should be zero filled or not.
//
//      Return value
//          TRUE if conversion successful, FALSE if overflow.
//
//      Remarks
//          Number is right justified in the character array.
//          Negative values have a leading minus (-) sign.
//              If bZeroFill is TRUE, the first character is minus sign.
//
//---------------------------------------------------------------------------
//
//  int64_t atoi64(wstring_view text_sv);
//
//      Function
//          Convert a character string view to a 64 bit integer.
//
//      Parameters
//          text                Character string view containing the value to be converted.
//
//      Return value
//          Integer value result.  IMSA_BAD_INT64 if non-numeric characters encountered.
//
//      Remarks
//          Leading blanks are ignored.
//          The first character can be a minus (-) or plus (+).
//          If the entire string is blank, zero is returned.
//          If non-numeric characters are encounted within or after the digits,
//              IMSA_BAD_INT64 is returned.
//
//---------------------------------------------------------------------------
//
//  csprochar* i64toa (int64_t iValue, csprochar pszValue[]);
//
//      Function
//          Converts a 64 bit integer to a character string (null terminated).
//
//      Parameters
//          iValue              Value to be converted.
//          pszValue            Character string containing converted value.
//
//      Return value
//          Pointer to string containing the converted value.
//
//      Remarks
//          Negative value has a leading minus (-) sign.
//
//---------------------------------------------------------------------------
//
//  double atod (wstring_view text_sv, UINT uDec = 0);
//
//      Function
//          Convert a character string (null terminated) to a  64 bit integer.
//
//      Parameters
//          text                Character string view containing the value to be converted.
//          uDec                Number of decimal places if implied decimal.
//
//      Return value
//          Integer value result.  IMSA_BAD_DOUBLE if non-numeric characters encountered.
//
//      Remarks
//          Leading blanks are ignored.
//          The first character can be a minus (-) or plus (+).
//          The decimal point can be a period(.) or comma(,).
//          The number of decimal places is used ONLY if there are implied decimal places.
//          If the entire string is blank, zero is returned.
//          If non-numeric characters are encounted within or after the digits,
//              IMSA_BAD_DOUBLE is returned.
//
//---------------------------------------------------------------------------
//
//  TCHAR* dtoa (double dValue, TCHAR pszValue[], int iDec = 0, TCHAR cDec = DOT, false bZeroDec = TRUE);
//
//      Function
//          Converts a double float to a character string (null terminated).
//
//      Parameters
//          dValue              Value to be converted.
//          pszValue            Character string containing converted value.
//          iDec                Number of decimal places.
//          cDec                Decimal character (DOT, COMMA, or SPACE).
//          bZeroDec            Is there a zero before the decimal point.
//
//      Return value
//          Pointer to string containing the converted value.
//
//      Remarks
//          Negative value has a leading minus (-) sign.
//          If the decimal character is SPACE, the decimal point is implied (NOT in character string).
//
//---------------------------------------------------------------------------
//
//  BOOL isnum (csprochar* pszValue, UINT uLen);
//
//      Parameters
//          pszValue            Character array to be tested.
//          uLen                Length of character array to be tested.
//
//      Return value
//          TRUE if all the characters are digits (0-9), otherwise FALSE.
//
//---------------------------------------------------------------------------
//
//  csprochar* strtoken(csprochar* pszLine, const csprochar* pszSeparators, csprochar* pcFound);
//
//      Parameters
//          pszLine             To get first token a pointer to line being tokenized.
//                              To get the next token, use NULL.
//          pszSeparators       A pointer to a string of all separators considered.
//          pcFound             A pointer to a a character where separator found is stored.
//
//      Return value
//          Pointer to the token within the line, NULL if no more tokens are found.
//
//      Remarks
//          Single or double quotes can surround a token.
//          Each call modifies pszLine by substituting a NULL character for each
//          separator found.
//          If token is last token of the string, the found character will be EOS.
//          If a quote string token is followed immediately be another token,
//          the found character will be the first character in the separator string.
//          The token is removed from the initial CIMSAString.
//
//***************************************************************************
//***************************************************************************
//***************************************************************************

#include <zUtilO/zUtilO.h>

class Serializer;

constexpr TCHAR SPACE       = ' ';
constexpr TCHAR DOT         = '.';
constexpr TCHAR COMMA       = ',';
constexpr TCHAR HYPHEN      = '-';
constexpr TCHAR PLUS        = '+';
constexpr TCHAR ZERO        = '0';
constexpr TCHAR COLON       = ':';
constexpr TCHAR TAB         = '\t';
constexpr TCHAR EOS         = '\0';
constexpr TCHAR UNDERSCORE  = '_';
constexpr TCHAR SINGLEQUOTE = '\'';
constexpr TCHAR DOUBLEQUOTE = '\"';

#define TAB_STR             _T("\t")
#define SPACE_COMMA_STR     _T(" ,")
#define SPACE_COMMA_TAB_STR _T(" ,\t")

#define NONE   -1

#define IMSA_BAD_INT64  -1000000000000000000
#define IMSA_BAD_DOUBLE ( -MAXVALUE )

#include <stdarg.h>


/////////////////////////////////////////////////////////////////////////////
//
//                               CIMSAString
//
/////////////////////////////////////////////////////////////////////////////

class CLASS_DECL_ZUTILO CIMSAString : public CString
{
public:
// Constructors (CIMSAString overloads all the CString constructors)
    CIMSAString() {}
    CIMSAString(const CString& s) : CString(s) {}
    CIMSAString(const CIMSAString& stringSrc) : CString(stringSrc)  {}
    CIMSAString(const TCHAR* psz) : CString(psz) {}
    CIMSAString(TCHAR ch, int nRepeat = 1) : CString(ch, nRepeat) {}
    CIMSAString(const TCHAR* pch, int nLength) : CString(pch, nLength) {}

// Comparison/analysis
    static bool IsNumeric(wstring_view text_sv, bool use_decimal_character_locale = true);
    static bool IsNumericOrSpecial(wstring_view text_sv, bool use_decimal_character_locale = true);
    static bool IsNumeric(const CString& text, bool use_decimal_character_locale = true) { return IsNumeric(wstring_view(text), use_decimal_character_locale); }
    bool IsNumeric(bool use_decimal_character_locale = true) const { return IsNumeric(*this, use_decimal_character_locale); }

    BOOL IsNumericU() const;

    static bool IsInteger(wstring_view text_sv);
    bool IsInteger() const { return IsInteger(*this); }

    static bool IsName(wstring_view name_sv);
    bool IsName() const { return IsName(*this); }

    static bool IsReservedWord(wstring_view word_sv);
    bool IsReservedWord() const { return IsReservedWord(*this); }

    static void ValidateName(NullTerminatedString name);

#ifdef WIN_DESKTOP
    static CSize GetLongestWordSize(CDC* pDC, const CString& text);
#endif
    int ReverseFindOneOf(const csprochar* pszSet) const;

#ifdef WIN_DESKTOP
    int CompareNoCase(int iID) const;
#endif

    int CompareNoCase(LPCTSTR lpsz) const;

// Conversion
    static CString MakeName(wstring_view base_name_sv, size_t max_name_length = SIZE_MAX);
    static CString MakeNameRestrictLength(wstring_view base_name_sv) { return MakeName(base_name_sv, 48); }
    void MakeName()                                                  { *this = MakeName(*this); }
    void MakeNameRestrictLength()                                    { *this = MakeNameRestrictLength(*this); }

    // returns a valid name that is also not reserved
    template<typename ECC = void*>
    static std::wstring CreateUnreservedName(wstring_view text_sv, ECC extra_check_callback = nullptr);

    static CString MakeNumeric(wstring_view text_sv);
    void MakeNumeric() { *this = MakeNumeric(*this); }

    static TCHAR GetDecChar();

    // Adjustment/formatting
    CIMSAString AdjustLenLeft(int iLen,csprochar cPad = SPACE) const;
    static CString AdjustLenLeft(CString csStr,int iLen,TCHAR cPad = SPACE);
    CIMSAString AdjustLenRight(int, csprochar = SPACE) const;
    static CString MakeExactLength(CString string, int new_length) { return SO::MakeExactLength(string, static_cast<size_t>(new_length)); }
    inline void MakeExactLength(int new_length)                    { SO::MakeExactLength(*this, static_cast<size_t>(new_length)); }

    void QuoteDelimit();
    CIMSAString GetToken(const csprochar* pszSeparators = SPACE_COMMA_STR, csprochar* pcFound = NULL, BOOL bDontStripQuotes = FALSE);
    void AddParens (UINT, UINT);    // gsf 12/01/99
    void StripParens();         // gsf 12/01/99
    int ReplaceNames(CString sOldName, CString sNewName);

// Numerical conversion
    static int64_t Val(wstring_view text_sv);
    int64_t Val() const { return Val(*this); }

    static double fVal(wstring_view text_sv);
    double fVal() const { return fVal(*this); }

    void Str(int64_t iVal, int iLen=NONE, csprochar cPad=SPACE);
    void Str(int iVal,     int iLen=NONE, csprochar cPad=SPACE)  { Str((int64_t) iVal, iLen, cPad);   }
    void Str(UINT uVal,    int iLen=NONE, csprochar cPad=SPACE)  { Str((int64_t) uVal, iLen, cPad);   }

#ifdef WIN_DESKTOP
// Date/time access
    void Date(CTime);
    void Date()  { Date(CTime::GetCurrentTime()); }
    void Time(CTime);
    void Time()  { Time(CTime::GetCurrentTime()); }
    CIMSAString DateTime(CTime);
    CIMSAString DateTime() { return DateTime(CTime::GetCurrentTime()); }

    int Wrap(CStringArray& acsLine, int iColLeft=0, int iColRight=132, BOOL bStripCRLF=FALSE) const;
#endif

// To avoid casting
    CIMSAString Mid(int iFirst) const             { return CIMSAString(CString::Mid(iFirst)); }
    CIMSAString Mid(int iFirst, int iCount) const { return CIMSAString(CString::Mid(iFirst, iCount)); }
    CIMSAString Left(int iCount) const            { return CIMSAString(CString::Left(iCount)); }
    CIMSAString Right(int iCount) const           { return CIMSAString(CString::Right(iCount)); }

// Assignment
    const CIMSAString& operator=(const TCHAR* str)     { CString::operator=(str); return *this; }
    const CIMSAString& operator=(const CIMSAString& s) { CString::operator=(s);   return *this; }
    const CIMSAString& operator=(const CString& cs)    { CString::operator=(cs);  return *this; }

    BOOL operator< (const CIMSAString& csRHS);
    BOOL operator> (const CIMSAString& csRHS);
    BOOL operator<=(const CIMSAString& csRHS);
    BOOL operator>=(const CIMSAString& csRHS);

    void serialize(Serializer& ar);
};


template<typename ECC/* = void* */>
std::wstring CIMSAString::CreateUnreservedName(const wstring_view text_sv, ECC extra_check_callback/* = nullptr*/)
{
    std::wstring name_candidate = SO::ToUpper(text_sv);

    for( int i = 0; ; ++i )
    {
        bool valid = ( IsName(name_candidate) && !IsReservedWord(name_candidate) );

        if constexpr(!std::is_same_v<ECC, void*>)
        {
            if( valid )
                valid = extra_check_callback(name_candidate);
        }

        if( valid )
            return name_candidate;

        name_candidate = CS2WS(MakeName(text_sv));

        if( i > 0 )
            name_candidate.append(FormatTextCS2WS(_T("_%d"), i));
    }
}



/////////////////////////////////////////////////////////////////////////////
//
//                          Non-class string fuctions
//
/////////////////////////////////////////////////////////////////////////////

CLASS_DECL_ZUTILO BOOL    i64toc(int64_t iValue, csprochar pszValue[], UINT uLen, BOOL bZeroFill = FALSE);
CLASS_DECL_ZUTILO int64_t atoi64(wstring_view text_sv);
CLASS_DECL_ZUTILO TCHAR*  i64toa(int64_t iValue, TCHAR pszValue[]);

CLASS_DECL_ZUTILO double atod(wstring_view text_sv, unsigned int implied_decimals = 0);
CLASS_DECL_ZUTILO TCHAR* dtoa(double dValue, TCHAR* pszValue, UINT uDec = 0, TCHAR cDec = DOT, bool bZeroDec = true);

CLASS_DECL_ZUTILO csprochar* strtoken(csprochar* pszLine, const csprochar* pszSeparators, csprochar* pcFound);

CLASS_DECL_ZUTILO CString NumberToString(double value);

template<typename T = double>
CLASS_DECL_ZUTILO T StringToNumber(wstring_view text_sv);

CLASS_DECL_ZUTILO unsigned CountNewlines(NullTerminatedString text);
