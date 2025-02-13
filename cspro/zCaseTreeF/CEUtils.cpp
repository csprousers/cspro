// CEUtils.cpp: implementation of the CCapiEUtils class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "CEUtils.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#define new DEBUG_NEW
#endif

CString CCapiEUtils::StripValue(CIMSAString CSValue, bool bNumericClean /*quita los ceros y blancos que estén a la izquierda*/)
{
    //quita todos los blancos y valores 0 que estén a la izquierda
    //de Value. (si es que los hay).
    if( SO::IsBlank(CSValue) ){

        CString CSWhite(_T(""));

        return CSWhite;

    } else {

        bool bDirtyCharWasFound = false;
        int iValueLength = CSValue.GetLength();
        CString CSChar;
        int i=0;
        for( /*int*/ i=0; i<iValueLength && !bDirtyCharWasFound; i++){

            CSChar = CSValue.GetAt(i);
            if( CSChar!=CString( (csprochar) WHITE_SPACE_ASCII ) ){

                if( !bNumericClean ){

                    //si no es una limpieza numérica, el caracter no blanco ya ha sido encontrado.
                    bDirtyCharWasFound = true;

                } else {

                    //en caso de requerirse de una limpieza de tipo numérica, además el caracter debe ser distinto de "0"
                    if( CSChar!=CString(_T("0")) ){

                        bDirtyCharWasFound = true;
                    }
                }
            }

        }


        if( bDirtyCharWasFound ){

            i--;
            //ahora i tiene el índice del primer no blanco y no cero :


            CString CSCleanedValue;
            for( int j=i; j<iValueLength; j++){

                CSCleanedValue = CSCleanedValue + CSValue.GetAt(j) ;

            }


            //INIT 05-Mar-2001 F.A.B.N.
            if( bNumericClean && CSCleanedValue==CString((csprochar)DECIMAL_KEY_ASCII) ){

                CSCleanedValue = CSCleanedValue + _T("0"); // ==> forma un ".0" para que sea reconocido como numerico por CCapiEUtils::IsNumeric(CString,int)

            }
            //END 05-Mar-2001 F.A.B.N.

            return CSCleanedValue; //ok.

        } else {

            if( bNumericClean==true ) return CString(_T("0"));
            else return CString(_T(""));
        }
    }
}

bool CCapiEUtils::IsNumeric(TCHAR c)
{
    return ( c >= _T('0') && c <= '9' );
}

bool CCapiEUtils::IsNumeric( CString S , int iDecimalKeyAscii, bool bStripString)
{
    bool bReturnValue = true;
    int iNumNumericSymbols = 0;

    if(bStripString){
        S = clearString(S,true);
    }

    int iStringLength = S.GetLength();
    if( iStringLength>0 ){

        //
        int iFirstIndex = 0;
        int iLastIndex  = S.GetLength()-1;

        //
        if( S.GetAt(0)==_T('+') || S.GetAt(0)=='-' ){

            iFirstIndex++;
        }

        //
        int  iNumPointsFounded = 0;
        csprochar c;
        for(int iIndex=iFirstIndex; (iIndex<=iLastIndex) && (iNumPointsFounded<=1) && (bReturnValue==true); iIndex++){

            c = S.GetAt( iIndex );
            if( IsNumeric(c)==false ){

                if( (CString) c == CString((csprochar)iDecimalKeyAscii) ){

                    iNumPointsFounded++;
                    if(iNumPointsFounded>1){

                        bReturnValue = false;
                    }

                } else {

                    bReturnValue = false;
                }

            } else {

                iNumNumericSymbols++;
            }
        }

    } else {

        bReturnValue = false;
    }

    if( iNumNumericSymbols>0 ){

        return bReturnValue;

    } else {

        return false;
    }
}

CString CCapiEUtils::ToString(csprochar *pszText, int iMaxBuff)
{
    CString s = _T("");
    for(int i=0; i<iMaxBuff && pszText[i]!=_T('\0'); i++){
        s = s + CString(pszText[i]);
    }
    return s;
}

bool CCapiEUtils::InArray( CArray<CString,CString>* pArray, CString csStr )
{
    bool    bInArray = false;
    int     iSize    = pArray ? pArray->GetSize() : 0;
    for(int i=0; !bInArray && i<iSize; i++){
        if( pArray->ElementAt(i)==csStr ){
            bInArray = true;
        }
    }

    return bInArray;
}

int CCapiEUtils::GetIndex( int x, CArray<int,int>& aIntArray )
{
    int iIndex = -1;
    int iSize = aIntArray.GetSize();
    for(int i=0; iIndex==-1 && i<iSize; i++){
        if(aIntArray.GetAt(i)==x){
            iIndex = i;
        }
    }
    return iIndex;
}

int CCapiEUtils::Union(  CArray<int,int>& aIntArray1, CArray<int,int>& aIntArray2, CArray<int,int>& aIntUnion)
{
    aIntUnion.RemoveAll();

    int iSize1 = aIntArray1.GetSize();
    for(int i=0; i<iSize1; i++){
        aIntUnion.Add( aIntArray1.GetAt(i) );
    }

    int iSize2 = aIntArray2.GetSize();
    for(int j=0; j<iSize2; j++){
        if(GetIndex(aIntArray2.GetAt(j),aIntUnion)==-1){
            aIntUnion.Add(aIntArray2.GetAt(j));
        }
    }

    return aIntUnion.GetSize();
}
