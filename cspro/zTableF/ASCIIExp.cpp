//***************************************************************************
//  File name: ASCIIExp.cpp
//
//  Description:
//  Export grid as tab delimited ASCII.
//
//***************************************************************************

#include "StdAfx.h"
#include "ASCIIExp.h"
#include <ostream>


CTableGridExporterASCII::CTableGridExporterASCII()
{
    GetPrivateProfileString(_T("intl"), _T("sThousand"), _T(","), m_pszThousand, 8, _T("WIN.INI"));
}

void CTableGridExporterASCII::StartFile(_tostream& os)
{
}

void CTableGridExporterASCII::EndFile(_tostream& os)
{
}

void CTableGridExporterASCII::StartFormats(_tostream& os)
{
}

void CTableGridExporterASCII::WriteFormat(_tostream& os, const CFmt& fmt)
{
}

void CTableGridExporterASCII::EndFormats(_tostream& os)
{
}

void CTableGridExporterASCII::StartTable(_tostream& os, int iNumCols)
{
    UNREFERENCED_PARAMETER(os);
    UNREFERENCED_PARAMETER(iNumCols);
}

void CTableGridExporterASCII::EndTable(_tostream& os)
{
}

void CTableGridExporterASCII::WriteTitle(_tostream& os, const CFmt& fmt, const CString& sTitle)
{
    if (!sTitle.IsEmpty()) {
        os << (LPCTSTR)sTitle << _T("\r\n");
        /*CString sText = sTitle + _T("\r\n");
        os.write(sText,sText.GetLength());*/
    }
}

void CTableGridExporterASCII::WriteSubTitle(_tostream& os, const CFmt& fmt, const CString& sSubTitle)
{
    if (!sSubTitle.IsEmpty()) {
        os << (LPCTSTR)sSubTitle << _T("\r\n");
        /*CString sText = sSubTitle + _T("\r\n");
        os.write(sText,sText.GetLength());*/
    }
}

void CTableGridExporterASCII::StartRow(_tostream& os, int iRow, const CDWordArray& aRowHeaders)
{
}

void CTableGridExporterASCII::EndRow(_tostream& os)
{
    os << _T("\r\n");
}

void CTableGridExporterASCII::WriteCell(_tostream& os,
                        int iCol,
                        int iRow,
                        const CFmt& fmt,
                        const CString& sCellData,
                        const CJoinRegion& join,
                        const CArray<CJoinRegion>& aColHeaders)
{
    UNREFERENCED_PARAMETER(fmt);
    UNREFERENCED_PARAMETER(aColHeaders);

    // add tab delimiter
    if (iCol != 0) {
        os << TAB_STR;
    }

    // skip cells that are not part of a join
    if (iCol == join.iStartCol && iRow == join.iStartRow) {

        CString sVal;

        // replace '-' with '0'
        if (sCellData.GetLength()==1 && sCellData[0]=='-') {
            sVal = _T("0");
        }
        else {

            // remove ',' from cell data
            for (int i = 0; i < sCellData.GetLength(); ++i) {
                if (sCellData[i] != *m_pszThousand) {
                    sVal += sCellData[i];
                }
            }
        }

        // write cell data
        os << (LPCTSTR)sVal;
    }

}

