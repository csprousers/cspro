#pragma once
//***************************************************************************
//  File name: RTFExpt.h
//
//  Description:
//       Table grid exporter for Rich Text Format (RTF).
//
//***************************************************************************

#include <zTableF/GridExpt.h>


/////////////////////////////////////////////////////////////////////////////
//
//                             CRTFFontInfo
//
// Stores font information for RTF operations.
//
/////////////////////////////////////////////////////////////////////////////
class CRTFFontInfo : public CObject
{
DECLARE_DYNAMIC(CRTFFontInfo)

// construction
public:
    CRTFFontInfo() {}
    CRTFFontInfo(const CString& sFace, BYTE iPitchAndFamily) :
        m_sFace(sFace), m_iPitchAndFamily(iPitchAndFamily) {}
    CRTFFontInfo(const CRTFFontInfo& x) {
        *this=x;
    }

// access
public:
    const CString& GetFace(void) const { return m_sFace; }
    BYTE GetPitchAndFamily(void) const { return m_iPitchAndFamily; }
    void SetFace(const CString& sFace) { m_sFace=sFace; }
    void SetPitchAndFamily(BYTE iPitchAndFamily) { m_iPitchAndFamily=iPitchAndFamily; }

// operators
public:
    void operator=(const CRTFFontInfo& x) {
        SetFace(x.GetFace());
        SetPitchAndFamily(x.GetPitchAndFamily());
    }
    bool operator==(const CRTFFontInfo& x) {
        return (GetFace()==x.GetFace() && GetPitchAndFamily()==x.GetPitchAndFamily());
    }

// members
private:
    CString m_sFace;       // font face, ex: "Arial" ... from logfont lfFaceName member
    BYTE m_iPitchAndFamily;  // pitch and family (codes to "swiss") ... logfont lfPitchAndFamily member
};

typedef CArray<CRTFFontInfo, CRTFFontInfo&> CRTFFontInfoArray;
typedef CArray<COLORREF, COLORREF> CRTFColorInfoArray;

//////////////////////////////
//  CTableGridExporterRTF
//
// Export grid as table in MS Rich Text Format (RTF).
////////////////////////////

class CTableGridExporterRTF : public CTableGridExporter
{

public:

    CTableGridExporterRTF(int iLeftMargin,
                          int iRightMargin,
                          int iLogPixelsY);

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

    virtual bool IgnoreFormatting() { return false; } // GHM 20100818

    virtual void WriteCell(_tostream& os,
                           int iCol,
                           int iRow,
                           const CFmt& fmt,
                           const CString& sCellData,
                           const CJoinRegion& join,
                           const CArray<CJoinRegion>& aColHeaders);
private:

    struct CRTFCellInfo {
        int iCol;
        int iRow;
        CFmt fmt;
        CString sCellData;
        CJoinRegion join;
    };
    CArray<CRTFCellInfo,CRTFCellInfo> m_currRowCells;

    CRTFFontInfoArray m_aFontInfo;
    CRTFColorInfoArray m_aColorInfo;
    int m_iLeftMargin;
    int m_iRightMargin;
    int m_iLogPixelsY;
};
