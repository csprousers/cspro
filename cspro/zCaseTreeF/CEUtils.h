#pragma once
// CEUtils.h: interface for the CCapiEUtils class.
//
//////////////////////////////////////////////////////////////////////

#include <zCaseTreeF/zCaseTreeF.h>


class ZCASETREEF_API CCapiEUtils
{
public:
    static CString  ToString( csprochar* pszText, int iMaxBuff );
    static bool     InArray( CArray<CString,CString>* pArray, CString csStr );

    static CString StripValue    ( CIMSAString CSValue, bool bNumericClean );

    static bool IsNumeric        ( TCHAR c );
    static bool IsNumeric        ( CString S, int iDecimalKeyAscii, bool bStripString);

    static int     GetIndex( int x, CArray<int,int>& aIntArray );
    static int     Union(  CArray<int,int>& aIntArray1, CArray<int,int>& aIntArray2, CArray<int,int>& aIntUnion);
};
