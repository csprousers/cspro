#pragma once
//***************************************************************************
//  File name: HTMLExpt.h
//
//  Description:
//       Table grid exporter for HTML.
//
//***************************************************************************
#include <zTableF/GridExpt.h>

/////////////////////////////////////////////////////////////////////////////
//
//                             CHTMLStyleInfo
//
// Stores style information for HTML operations (font, color, etc.)
//
/////////////////////////////////////////////////////////////////////////////
class CHTMLStyleInfo : public CObject
{
DECLARE_DYNAMIC(CHTMLStyleInfo)

// construction
public:
    CHTMLStyleInfo() {}
    CHTMLStyleInfo(const CString& sStyle) : m_sStyle(sStyle)  {}
    CHTMLStyleInfo(const CHTMLStyleInfo& x) {
        *this=x;
    }

// access
public:
    const CString& GetStyle(void) const { return m_sStyle; }
    void SetStyle(const CString& sStyle) { m_sStyle=sStyle; }

// operators
public:
    void operator=(const CHTMLStyleInfo& x) {
        SetStyle(x.GetStyle());
    }
    bool operator==(const CHTMLStyleInfo& x) {
        return (GetStyle()==x.GetStyle());
    }

// members
private:
    CString m_sStyle;        // font face, color, etc. in css format
};

typedef CArray<CHTMLStyleInfo, CHTMLStyleInfo&> CHTMLStyleInfoArray;

//////////////////////////////
//  CTableGridExporterHTML
//
// Export grid as table in HTML.
////////////////////////////

class CTableGridExporterHTML : public CTableGridExporter
{

public:

    CTableGridExporterHTML(int iLogPixelsY);

    virtual void StartFile(_tostream& os);
    virtual void EndFile(_tostream& os);

    virtual void StartFormats(_tostream& os);
    virtual void WriteFormat(_tostream& os, const CFmt& fmt);
    virtual void EndFormats(_tostream& os);

    virtual void StartTable(_tostream& os, int iNumCols);
    virtual void EndTable(_tostream& os);

    virtual void WriteTitle(_tostream& os, const CFmt& fmt, const CString& sTitle);
    virtual void WriteSubTitle(_tostream& os, const CFmt& fmt, const CString& sSubTitle);

    virtual void StartHeaderRows(_tostream& os);
    virtual void EndHeaderRows(_tostream& os);
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

protected:

    void UpdateRowHeaders(int iRow, int iRowStubLevel);
    CIMSAString HeadersToString(const CDWordArray& aHeaders);
    CIMSAString HeadersToString(const CArray<CJoinRegion>& aHeaders);

private:

    CHTMLStyleInfoArray m_aStyleInfo;
    int m_iLogPixelsY;
    int m_iNumCols;
    CIMSAString m_sRowHeaderString;
    bool bInHeaderRows;
};
