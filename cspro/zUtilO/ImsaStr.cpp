//***************************************************************************
//  File name: ImsaStr.cpp
//
//  Description:
//       Implementation of CIMSAString class
//
//  NOTE: Place change history in the *.h file.
//
//***************************************************************************

#include "StdAfx.h"
#include "ImsaStr.h"
#include <zToolsO/Special.h>


namespace
{
    constexpr TCHAR ConvertAccentChar(TCHAR ch)
    {
        switch( ch )
        {
            case _T('à'):
            case _T('À'):
            case _T('â'):
            case _T('Â'):
            case _T('á'):
            case _T('Á'):
            case _T('ã'):
            case _T('Ã'):
                return _T('A');
            case _T('ç'):
            case _T('Ç'):
                return _T('C');
            case _T('é'):
            case _T('É'):
            case _T('è'):
            case _T('È'):
            case _T('ê'):
            case _T('Ê'):
            case _T('ë'):
            case _T('Ë'):
                return _T('E');
            case _T('î'):
            case _T('Î'):
            case _T('ï'):
            case _T('Ï'):
            case _T('í'):
            case _T('Í'):
                return _T('I');
            case _T('ô'):
            case _T('Ô'):
            case _T('ó'):
            case _T('Ó'):
            case _T('õ'):
            case _T('Õ'):
                return _T('O');
            case _T('ñ'):
            case _T('Ñ'):
                return _T('N');
            case _T('ù'):
            case _T('Ù'):
            case _T('ú'):
            case _T('Ú'):
            case _T('û'):
            case _T('Û'):
            case _T('ü'):
            case _T('Ü'):
                return _T('U');
            default:
                return ch;
        }
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::IsNumeric
//
/////////////////////////////////////////////////////////////////////////////
char CIMSAString::GetDecChar()
{
#ifdef WIN32
    static char decimal_ch =
        []()
        {
            const std::string& decimal_char = GetLocaleInformation(LOCALE_SDECIMAL);
            return !decimal_char.empty() ? decimal_char.front() :
                                           '.';
        }();

    return decimal_ch;
#else
    return '.';
#endif
}


bool CIMSAString::IsNumeric(const wstring_view text_sv, bool use_decimal_character_locale/* = true*/)
{
    if( text_sv.empty() )
        return false;

    auto text_sv_itr = text_sv.cbegin();
    auto text_sv_end = text_sv.cend();
    std::optional<char> decimal_ch;

    // skip past the sign
    if( *text_sv_itr == '-' || *text_sv_itr == '+' )
    {
        ++text_sv_itr;

        if( text_sv_itr == text_sv_end )
            return false;
    }

    for( ; text_sv_itr != text_sv_end; ++text_sv_itr )
    {
        if( !is_digit(*text_sv_itr) )
        {
            // multiple decimal characters not allowed
            if( decimal_ch.has_value() )
                return false;

            decimal_ch = use_decimal_character_locale ? GetDecChar() : DOT;

            if( *text_sv_itr != *decimal_ch )
                return false;
        }
    }

    return true;
}


bool CIMSAString::IsNumericOrSpecial(const std::string_view text_sv, const bool use_decimal_character_locale/* = true*/)
{
    return ( IsNumeric(text_sv, use_decimal_character_locale) ||
             SpecialValues::StringIsSpecial(text_sv) );
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::IsNumericU
//
/////////////////////////////////////////////////////////////////////////////

// BMD 23 Jul 2003

BOOL CIMSAString::IsNumericU() const {

    int i = 0;
    int iDotCount = 0;
    if (GetLength() == 0)  {
        return FALSE;
    }

    if (GetAt(i) == HYPHEN || GetAt(i)==PLUS)  {
        // negative sign
        i++;
    }
    while (i < GetLength())  {
        if (!is_digit(GetAt(i)))  {
            if (GetAt(i) == DOT || GetAt(i) == COMMA)  {
                if (++iDotCount > 1)  {
                    return FALSE;
                }
            }
            else  {
                return FALSE;
            }
        }
        i++;
    }
    return TRUE;
}


bool CIMSAString::IsInteger(std::string_view text_sv)
{
    // skip past the sign
    if( !text_sv.empty() && ( text_sv.front() == '-' || text_sv.front() == '+' ) )
        text_sv.remove_prefix(1);

    // at least one digit must exist
    if( text_sv.empty() )
        return false;

    for( const char ch : text_sv )
    {
        if( !is_digit(ch) )
            return false;
    }

    return true;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::IsName
//
/////////////////////////////////////////////////////////////////////////////

bool CIMSAString::IsName(const std::string_view name_sv)
{
    auto name_sv_itr = name_sv.cbegin();
    const auto& name_sv_end = name_sv.cend();

    // first char must be alpha
    if( name_sv_itr == name_sv_end || !is_alpha(*name_sv_itr) )
        return false;

    // additional characters must be alpha, numeric, or an underscore
    while( ++name_sv_itr != name_sv_end )
    {
        if( !is_tokch(*name_sv_itr) )
            return false;
    }

    // last char cannot be an underscore
    if( *(name_sv_itr - 1) == UNDERSCORE )
        return false;

    return true;
}

/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::ReverseFindOneOf
//
/////////////////////////////////////////////////////////////////////////////

int CIMSAString::ReverseFindOneOf(const csprochar* pszSet) const {

    int iRetVal = NONE;
    size_t i;
    int j;

    for (i = 0 ; i < _tcslen(pszSet) ; i++)  {
        j = ReverseFind(pszSet[i]);
        if (j > iRetVal)  {
            iRetVal = j;
        }
    }
    return iRetVal;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::MakeName
//
/////////////////////////////////////////////////////////////////////////////

CString CIMSAString::MakeName(const wstring_view base_name_sv, const size_t max_name_length/* = SIZE_MAX*/)
{
    CString text = base_name_sv;
    text.MakeUpper();

    for( int i = 0; i < text.GetLength(); ++i )
    {
        text.SetAt(i, ConvertAccentChar(text.GetAt(i)));

        if( !is_tokch(text.GetAt(i)) )
        {
            // previously labels like "Survey Date: Day" would lead to names with multiple underscore
            // characters (SURVEY_DATE__DAY); now only put a single underscore for sequences of invalid characters
            if( i == 0 || text.GetAt(i - 1) != UNDERSCORE )
            {
                text.SetAt(i, UNDERSCORE);
            }

            else
            {
                text = text.Left(i) + text.Mid(i + 1);
                i--;
            }
        }
    }

    // strip off left-side non-alpha characters
    while( !text.IsEmpty() && !is_alpha(text.GetAt(0)) )
        text = text.Mid(1);

    // potentially shorten the name
    if( static_cast<size_t>(text.GetLength()) > max_name_length )
        text.Truncate(max_name_length);

    // strip off right-side trailing underscores
    text.TrimRight(UNDERSCORE);

    // make sure we're not empty
    if( text.IsEmpty() )
        text =_T("NAME");

    ASSERT(IsName(text));

    return text;
}


std::string CIMSAString::CreateUnreservedName(const std::string_view text_sv,
                                              const std::function<bool(const std::string& name_candidate)>& extra_check_callback/* = {}*/)
{
    std::string name_candidate = SO::ToUpper(text_sv);

    for( int i = 0; ; ++i )
    {
        if( IsName(name_candidate) && !IsReservedWord(name_candidate) )
        {
            if( !extra_check_callback || extra_check_callback(name_candidate) )
                return name_candidate;
        }

        name_candidate = MakeName(text_sv);

        if( i > 0 )
            name_candidate.append(FormatText("_%d", i));
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::MakeNumeric
//
/////////////////////////////////////////////////////////////////////////////

CString CIMSAString::MakeNumeric(const wstring_view text_sv)
{
    CString numeric_text;
    bool added_number = false;

    if( !text_sv.empty() )
    {
        const TCHAR* itr = text_sv.data();
        const TCHAR* text_end = text_sv.data() + text_sv.length();

        // negative number indicator
        if( *itr == HYPHEN )
        {
            numeric_text.AppendChar(HYPHEN);
            ++itr;
        }

        const TCHAR decimal_ch = GetDecChar();
        bool added_decimal = false;

        for( ; itr != text_end; ++itr )
        {
            if( is_digit(*itr) )
            {
                added_number = true;
            }

            else if( *itr == decimal_ch && !added_decimal )
            {
                added_decimal = true;
            }

            else
            {
                continue;
            }

            numeric_text.AppendChar(*itr);
        }
    }

    if( !added_number )
        numeric_text = _T("0");

    return numeric_text;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::AdjustLenLeft
//
/////////////////////////////////////////////////////////////////////////////

CIMSAString CIMSAString::AdjustLenLeft (int iLen, csprochar cPad /*=SPACE*/) const
{
    if (GetLength() < iLen)  {
        return CIMSAString(cPad, iLen - GetLength()) + *this;
    }
    else  {
        return Right(iLen);
    }
}

CString CIMSAString::AdjustLenLeft(CString csStr,int iLen,TCHAR cPad/*= SPACE*/)
{
    if( csStr.GetLength() < iLen )
        return CString(cPad,iLen - csStr.GetLength()) + csStr;

    else
        return csStr.Right(iLen);
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::AdjustLenRight
//
/////////////////////////////////////////////////////////////////////////////

CIMSAString CIMSAString::AdjustLenRight (int iLen, csprochar cPad /*=SPACE*/) const {

    if (GetLength() < iLen)  {
        return *this + CIMSAString(cPad, iLen - GetLength());
    }
    else  {
        return Left(iLen);
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::GetToken
//
/////////////////////////////////////////////////////////////////////////////

CIMSAString CIMSAString::GetToken(const csprochar* pszSeparators /* = SPACE_COMMA_STR */, csprochar* pcFound /* = NULL */, BOOL bDontStripQuotes /* = FALSE */) {

    ASSERT(pszSeparators != NULL);
    ASSERT(_tcslen(pszSeparators) > 0);

    UINT uLen = GetLength();
    if (uLen == 0) {
        if (pcFound != NULL) {
            *pcFound = pszSeparators[0];
        }
        return _T("");
    }
    csprochar* pszLine = GetBuffer(0);
    csprochar* pszToken;
    if (bDontStripQuotes) {
        pszToken = std::wcstok(pszLine, pszSeparators,NULL);
    }
    else {
        pszToken = strtoken(pszLine, pszSeparators, pcFound);
    }
    if (pszToken == NULL) {
        if (pcFound != NULL) {
            *pcFound = pszSeparators[0];
        }
        *pszLine = EOS;
        ReleaseBuffer();
        return _T("");
    }
    CIMSAString csReturn = pszToken;
    UINT uLenTok = csReturn.GetLength();
    UINT uDiff = pszLine + uLen - pszToken - uLenTok;   // BMD 10 Sep 2004
    if (uDiff == 0) {
        *pszLine = EOS;
    }
    else {
        //memmove(pszLine, pszToken + uLenTok + 1, uDiff);   // BMD 10 Sep 2004
        memmove(pszLine, pszToken + uLenTok + 1, uDiff * sizeof(TCHAR)); // 20111212 for unicode
    }
    ReleaseBuffer();
    return csReturn;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::QuoteDelimit
//
/////////////////////////////////////////////////////////////////////////////

void CIMSAString::QuoteDelimit()
{
    bool bHasSingle = (Find('\'') != -1);
    bool bHasDouble = (Find('"') != -1);
    ASSERT(( bHasSingle != bHasDouble ) || ( !bHasSingle && !bHasDouble ));

    if (bHasSingle)  {
        *this = _T('"') + *this + _T('"');
    }
    else {
        *this = _T('\'') + *this + _T('\'');
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::Val
//
/////////////////////////////////////////////////////////////////////////////

int64_t CIMSAString::Val(const wstring_view text_sv)
{
    return atoi64(text_sv);
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::fVal
//
/////////////////////////////////////////////////////////////////////////////

double CIMSAString::fVal(const wstring_view text_sv)
{
    ASSERT(CIMSAString(text_sv).IsNumericU()); // 21 Jun 2006
    return atod(text_sv);
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::Str
//
/////////////////////////////////////////////////////////////////////////////

void CIMSAString::Str(int64_t iVal, int iLen /*=NONE*/, csprochar cPad /*=SPACE*/) {

    ASSERT(iLen == NONE || iLen > 0);

    BOOL bNegative = (iVal < 0);
    csprochar* pVal;

    if (bNegative)  {
        iVal = -iVal;
    }

    Empty();
    pVal = GetBuffer(21);
    i64toa (iVal, pVal);
    ReleaseBuffer();

    if (iLen != NONE)  {
        #ifdef _DEBUG
            ASSERT(GetLength() <= iLen);
            if (bNegative)  ASSERT(GetLength() <= iLen-1);
        #endif
        if (cPad == SPACE && bNegative) {
            *this = HYPHEN + *this;
        }
        *this = AdjustLenLeft(iLen, cPad);
        if (bNegative && cPad != SPACE)  {
            ASSERT(GetAt(0) == cPad);
            SetAt(0, HYPHEN);
            }
    }
    else if (bNegative)  {
        *this = HYPHEN + *this;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::AddParens
//
//  adds a number in paresn to the end of the string (good for occurrences)
//
//      u    = the number to add in parens
//      uLen = the number of digits to work with
//
//  e.g. cs = "foo"; cs.AddParens (9,2); cs --> "foo(09)"
//
/////////////////////////////////////////////////////////////////////////////

void CIMSAString::AddParens (UINT u, UINT uLen)
{
    // gsf 5/20/97: added iLen parameter

    int iDigits = 1;
    while (TRUE) {
        uLen = uLen / 10;
        if (uLen == 0) {
            break;
        }
        iDigits++;
    }

    CIMSAString csTemp;
    csTemp.Str(u, iDigits);

    int iThisLen = GetLength();
    int iTempLen = csTemp.GetLength();
    csprochar* pszTempStr = csTemp.GetBuffer(0);
    csprochar* pszThisStr = GetBuffer(iThisLen+iTempLen+2);

    *(pszThisStr + iThisLen) = _T('(');
    for (int i=0; i<iTempLen; i++) {
        *(pszThisStr + iThisLen + i + 1) = *(pszTempStr + i);
    }
    *(pszThisStr + iThisLen + iTempLen + 1) = _T(')');
    *(pszThisStr + iThisLen + iTempLen + 2) = 0;
    ReleaseBuffer(-1);      // use -1 because we changed the length
}

/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::StripParens
//
//  strips a number in parens from the end of the string
//
//  e.g. cs = "foo(09)"; cs.StringParens (9,2); cs --> "foo"
//
/////////////////////////////////////////////////////////////////////////////

void CIMSAString::StripParens()
{
    int len = ReverseFind('(');
    GetBuffer(0);
    ReleaseBuffer(len);
}

int CIMSAString::ReplaceNames(CString sOldName, CString sNewName)
{
    int iStart = 0;
    int iNumReplaces = 0;
    CIMSAString sCopyOfOriginal(*this);
    int iLength = GetLength();
    sCopyOfOriginal.MakeUpper();
    sNewName.MakeUpper();
    sOldName.MakeUpper();
    int iOldNameLength = sOldName.GetLength();
    while ((iStart = sCopyOfOriginal.Find(sOldName,iStart)) != -1){
        CIMSAString sProbableName = Mid(iStart,iOldNameLength+1);
        sProbableName.Trim();
        if(!sProbableName.IsName() || sProbableName.CompareNoCase(sOldName) == 0){
            CIMSAString sLeft = Left(iStart);
            CIMSAString sRight = Right(iLength - iStart - iOldNameLength);
            *this = sLeft+sNewName+sRight;
            iNumReplaces++;
        }
        iStart++;
    }
    return iNumReplaces;
}
/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::operator >
//
/////////////////////////////////////////////////////////////////////////////

BOOL CIMSAString::operator> (const CIMSAString& csRHS)

        { return !(*this <= csRHS); }


/////////////////////////////////////////////////////////////////////////////
//
//  CIMSAString::operator <=
//
/////////////////////////////////////////////////////////////////////////////

BOOL CIMSAString::operator<=(const CIMSAString& csRHS)

        { return (*this < csRHS || *this == csRHS);  }


/////////////////////////////////////////////////////////////////////////////
//
//      CIMSAString::operator >=
//
/////////////////////////////////////////////////////////////////////////////

BOOL CIMSAString::operator>=(const CIMSAString& csRHS)

        { return (*this > csRHS || *this == csRHS);  }


/////////////////////////////////////////////////////////////////////////////
//
//  CIMSAString::operator <
//
/////////////////////////////////////////////////////////////////////////////

BOOL CIMSAString::operator<(const CIMSAString& csRHS)  {

    if (!this->IsNumeric() || !csRHS.IsNumeric())  {
        return this->CString::Compare(csRHS) < 0;
//      return cs1.CString::Compare(cs2) < 0;
    }
    if (this->IsEmpty() || csRHS.IsEmpty())  {
        return this->CString::Compare(csRHS) < 0;
    }
    // handle signed numeric CIMSAStrings
    csprochar c1, c2;
    c1 = this->GetAt(0);
    c2 = csRHS.GetAt(0);
    // detect opposite signs
    if (c1 != HYPHEN && c2 == HYPHEN)  {
        return FALSE;
    }
    if (c1 == HYPHEN && c2 != HYPHEN)  {
        return TRUE;
    }
    // strip signs, if applicable

    CIMSAString csMag1(*this);
    CIMSAString csMag2(csRHS);

    if (c1==PLUS || c1==HYPHEN)  {
       csMag1 = this->Mid(1);
    }
    if (c2==PLUS || c2==HYPHEN)  {
        csMag2 = csRHS.Mid(1);
    }
    BOOL bMagnitude = (csMag1.CString::Compare(csMag2) < 0);

    if (c1 == HYPHEN && c2 == HYPHEN)  {
        return !bMagnitude;
    }
    else  {
        return bMagnitude;
    }
}


/////////////////////////////////////////////////////////////////////////////
//
//                                atoi64
//
/////////////////////////////////////////////////////////////////////////////

int64_t atoi64(wstring_view text_sv)
{
    text_sv = SO::TrimLeft(text_sv);

    if( text_sv.empty() )
        return 0;

    int64_t value = 0;
    bool negative = false;
    bool first_char = true;

    for( const TCHAR ch : text_sv )
    {
        if( first_char )
        {
            first_char = false;

            // allow a sign as the first character
            if( ( negative = ( ch == HYPHEN ) ) == true || ch == PLUS )
                continue;
        }

        unsigned digit = static_cast<unsigned>(ch - '0');

        if( digit <= 9 )
        {
            value = ( 10 * value ) + digit;
        }

        else
        {
            return IMSA_BAD_INT64;
        }
    }

    return negative ? ( -1 * value ) : value;
}


/////////////////////////////////////////////////////////////////////////////
//
//                                i64toc
//
/////////////////////////////////////////////////////////////////////////////

BOOL i64toc (int64_t iValue, csprochar pszValue[], UINT uLen, BOOL bZeroFill /* = FALSE */) {

    BOOL bNegative;
    int64_t iVal, iv;
    int i;

    if (iValue < 0) {
        bNegative = TRUE;
        iVal = -iValue;
    }
    else {
        bNegative = FALSE;
        iVal = iValue;
    }
    for (i = uLen - 1 ; i >= 0 ; i--) {
        if (iVal == 0) {
            if (i == (int) uLen - 1) {
                pszValue[i] = _T('0');
            }
            else {
                i++;
            }
            break;
        }
        else {
            iv = iVal / 10;
            pszValue[i] = (csprochar)(_T('0') | (iVal - 10 * iv));
            iVal = iv;
        }
    }
    if (i < 0) {
        return FALSE;
    }
    if (bZeroFill) {
        if (bNegative) {
            while (i > 1) {
                i--;
                pszValue[i] = ZERO;
            }
            if (i == 0) {
                return FALSE;
            }
            i--;
            pszValue[i] = HYPHEN;
        }
        else {
            while (i > 0) {
                i--;
                pszValue[i] = ZERO;
            }
        }
    }
    else {
        if (bNegative) {
            if (i == 0) {
                return FALSE;
            }
            i--;
            pszValue[i] = HYPHEN;
        }
        while (i > 0) {
            i--;
            pszValue[i] = SPACE;
        }
    }
    return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
//
//                                i64toa
//
/////////////////////////////////////////////////////////////////////////////

csprochar* i64toa (int64_t iValue, TCHAR pszValue[]) {

    TCHAR pszTemp[21];
    _tmemset(pszTemp, EOS, 21);

    BOOL bNegative;
    int64_t iVal, iv;
    int i;

    if (iValue < 0) {
        bNegative = TRUE;
        iVal = -iValue;
    }
    else {
        bNegative = FALSE;
        iVal = iValue;
    }
    for (i = 19 ; i >= 0 ; i--) {
        if (iVal == 0) {
            if (i == 19) {
                pszTemp[i] = _T('0');
            }
            else {
                i++;
            }
            break;
        }
        else {
            iv = iVal / 10;
            pszTemp[i] = (csprochar)(_T('0') | (iVal - 10 * iv));
            iVal = iv;
        }
    }
    if (bNegative) {
        i--;
        pszTemp[i] = HYPHEN;
    }
    memmove(pszValue, pszTemp + i, ( 21 - i ) * sizeof(TCHAR));
    return pszValue;
}


/////////////////////////////////////////////////////////////////////////////
//
//                                atod
//
/////////////////////////////////////////////////////////////////////////////

double atod(const std::string_view text_sv, const unsigned int implied_decimals/* = 0*/)
{
    const char* itr = text_sv.data();
    const char* const text_sv_end = text_sv.data() + text_sv.length();

    // skip past spaces
    while( itr < text_sv_end && *itr == SPACE )
        ++itr;

    const bool negative = ( itr < text_sv_end && *itr == HYPHEN );

    if( negative )
        ++itr;

    std::optional<unsigned int> decimals;
    double value = 0;

    for( ; itr < text_sv_end; ++itr )
    {
        if( *itr == DOT || *itr == COMMA )
        {
            if( decimals.has_value() )
                return IMSA_BAD_DOUBLE;

            decimals = 0;
        }

        else
        {
            const unsigned char digit_value = *itr - '0';

            if( digit_value > 9 )
                return IMSA_BAD_DOUBLE;

            value = value * 10 + digit_value;

            if( decimals.has_value() )
                decimals = *decimals + 1;
        }
    }

    // apply the decimal
    unsigned int number_decimals = decimals.value_or(implied_decimals);

    if( number_decimals > 0 )
    {
        if( number_decimals < _countof(Power10) )
        {
            value /= Power10[number_decimals];
        }

        else
        {
            for( ; number_decimals != 0; --number_decimals )
                value /= 10;
        }
    }

    // apply the negative
    return negative ? ( -1 * value ) :
                      value;
}


/////////////////////////////////////////////////////////////////////////////
//
//                                dtoa
//
/////////////////////////////////////////////////////////////////////////////

TCHAR* dtoa(double dValue, TCHAR* pszValue, UINT uDec/* = 0*/, TCHAR cDec/*= DOT*/, bool bZeroDec/*= true*/)
{
    int64_t kDec, kInt;
    TCHAR pszTemp[30];
    TCHAR* pszStart = pszValue;

    double dInt;
    double dDec = modf(dValue, &dInt);

    ASSERT(uDec < _countof(Power10));
    double k = Power10[uDec];

    if (dValue < 0) {
        kInt = (int64_t) -dInt;
        kDec = (int64_t) (-dDec * k + 0.5);
        *pszStart = HYPHEN;
        pszStart++;
    }
    else {
        kInt = (int64_t) dInt;
        kDec = (int64_t) (dDec * k + 0.5);
    }
    if (kDec >= k) {
        kInt++;
    }
    if (kInt > 0 || uDec == 0 || bZeroDec) {
        i64toa(kInt, pszTemp);
        size_t l = _tcslen(pszTemp);
        memmove(pszStart, pszTemp, l * sizeof(TCHAR)); // 20111212 sizeof for unicode
        pszStart += l;
    }
    if (uDec > 0) {
        if (cDec != SPACE) {
            *pszStart = cDec;
            pszStart++;
        }
        i64toc(kDec, pszTemp, uDec, TRUE);
        memmove(pszStart, pszTemp, uDec * sizeof(TCHAR)); // 20111212 sizeof for unicode
        pszStart += uDec;
    }
    *pszStart = EOS;
    return pszValue;
}


/////////////////////////////////////////////////////////////////////////////
//
//                                strtoken
//
/////////////////////////////////////////////////////////////////////////////

csprochar* strtoken(csprochar* pszValue, const csprochar* pszSeparators, csprochar* pcFound) {

    ASSERT(pszSeparators != NULL);

    static csprochar* pszSaveChar;
    csprochar* pszChar;

    if (pszValue == NULL) {
        pszChar = pszSaveChar;
    }
    else {
        pszChar = pszValue;
    }
//  Scan off separators
    if (*pszChar == EOS) {
        pszSaveChar = pszChar;
        if (pcFound != NULL) {
            *pcFound = EOS;
        }
        return NULL;
    }
    else {
        while (_tcschr(pszSeparators, (int) *pszChar) != NULL) {
            pszChar++;
            if (*pszChar == 0) {
                break;
            }
        }
        if (*pszChar == EOS) {
            pszSaveChar = pszChar;
            if (pcFound != NULL) {
                *pcFound = EOS;
            }
            return NULL;
        }
    }
//  If QUOTE

    if (*pszChar == '\"' || *pszChar == '\'') {
        csprochar szQuote = *pszChar;
        pszChar++;
        pszSaveChar = pszChar;
        while (*pszSaveChar != EOS && *pszSaveChar != szQuote) {
            pszSaveChar++;
        }
        if (pcFound == NULL) {
            if (*pszSaveChar == szQuote) {
                *pszSaveChar = EOS;
                pszSaveChar++;
            }
        }
        else {
            if (*pszSaveChar == EOS) {
                *pcFound = EOS;
            }
            else {
                *pszSaveChar = EOS;
                pszSaveChar++;
                if (_tcschr(pszSeparators, (int) *pszSaveChar) != NULL) {
                    *pcFound = *pszSaveChar;
                }
                else {
                    *pcFound = EOS;
                }
            }
        }
        return pszChar;
    }
//  If not QUOTE
    pszSaveChar = pszChar;
    while (_tcschr(pszSeparators, (int) *pszSaveChar) == NULL) {
        if (*pszSaveChar == EOS) {
            break;
        }
        pszSaveChar++;
    }
    if (pcFound != NULL) {
        *pcFound = *pszSaveChar;
    }
    if (*pszSaveChar != EOS) {
        *pszSaveChar = EOS;
        pszSaveChar++;
    }
    return pszChar;
}


/////////////////////////////////////////////////////////////////////////////
//
//                           CIMSAString::IsReservedWord
//
/////////////////////////////////////////////////////////////////////////////
bool CIMSAString::IsReservedWord(const std::string_view word_sv)
{
    return ( WindowsDesktopMessage::Send(UWM::UtilO::IsReservedWord, &word_sv) == 1 );
}


void CIMSAString::serialize(Serializer& ar)
{
    ar & static_cast<CString&>(*this);
}


template<typename T/* = double*/>
T StringToNumber(const wstring_view text_sv)
{
    if( CIMSAString::IsNumeric(text_sv) )
        return atod(text_sv);

    const double* const special_value = SpecialValues::StringIsSpecial<const double*>(UTF8_TODO::GetUtf8(text_sv));

    if( special_value != nullptr )
        return *special_value;

    if constexpr(std::is_same_v<T, double>)
    {
        return DEFAULT;
    }

    else
    {
        return std::nullopt;
    }
}

template CLASS_DECL_ZUTILO double StringToNumber(wstring_view text_sv);
template CLASS_DECL_ZUTILO std::optional<double> StringToNumber(wstring_view text_sv);


template<typename T/* = double*/>
T StringToNumber(const std::string_view text_sv)
{
    if( CIMSAString::IsNumeric(text_sv) )
        return atod(text_sv);

    const double* const special_value = SpecialValues::StringIsSpecial<const double*>(text_sv);

    if( special_value != nullptr )
        return *special_value;

    if constexpr(std::is_same_v<T, double>)
    {
        return DEFAULT;
    }

    else
    {
        return std::nullopt;
    }
}

template CLASS_DECL_ZUTILO double StringToNumber(std::string_view text_sv);
template CLASS_DECL_ZUTILO std::optional<double> StringToNumber(std::string_view text_sv);


size_t CountNewlines(const cs::string_sz text)
{
    size_t newlines = 0;

    for( const char* text_itr = text.c_str(); ; ++text_itr )
    {
        char ch = *text_itr;

        if( ch == '\0' )
        {
            return newlines;
        }

        else if( ch == '\r' )
        {
            ++newlines;

            // skip over the \n of a \r\n combination
            if( text_itr[1] == '\n' )
                ++text_itr;
        }

        else if( ch == '\n' )
        {
            ++newlines;
        }
    }
}
