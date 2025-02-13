#pragma once
//***************************************************************************
//  File name: ASCIIExp.h
//
//  Description:
//       Table grid exporter for ASCII tab-delimited.
//
//***************************************************************************

#include <zTableF/GridExpt.h>

//////////////////////////////
//  CTableGridExporterASCII
//
// Export grid as tab-delimited ASCII (works well in Excel)
////////////////////////////

class CTableGridExporterASCII : public CTableGridExporter
{

public:

    CTableGridExporterASCII();

    virtual void StartFile(_tostream& os);
    virtual void EndFile(_tostream& os);

    virtual void StartFormats(_tostream& os);
    virtual void WriteFormat(_tostream& os, const CFmt& fmt);
    virtual void EndFormats(_tostream& os);

    virtual void StartTable(_tostream& os, int iNumCols);
    virtual void EndTable(_tostream& os);

    virtual void WriteTitle(_tostream& os, const CFmt& fmt, const CString& sTitle);
    virtual void WriteSubTitle(_tostream& os, const CFmt& fmt, const CString& sSubTitle);

    virtual void StartHeaderRows(_tostream& os) {}
    virtual void EndHeaderRows(_tostream& os) {}
    virtual void StartRow(_tostream& os, int iRow, const CDWordArray& aRowHeaders);
    virtual void EndRow(_tostream& os);

    virtual bool IgnoreFormatting() { return true; } // GHM 20100818

    virtual void WriteCell(_tostream& os,
                           int iCol,
                           int iRow,
                           const CFmt& fmt,
                           const CString& sCellData,
                           const CJoinRegion& join,
                           const CArray<CJoinRegion>& aColHeaders);

private:
    TCHAR m_pszThousand[8];

};
