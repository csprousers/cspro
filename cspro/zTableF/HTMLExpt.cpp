//***************************************************************************
//  File name: HTMLExpt.cpp
//
//  Description:
//  Export grid as table in HTML.
//
//***************************************************************************

#include "StdAfx.h"
#include "HTMLExpt.h"
#include <ostream>

IMPLEMENT_DYNAMIC(CHTMLStyleInfo, CObject)

namespace {

CString GetHTMLFontInfo(const LOGFONT& lf, int iLogPixelsY)
{
    int iPoints;
    CString sRet;

    iPoints=abs(MulDiv(lf.lfHeight,72,iLogPixelsY));
    sRet.Format(_T("font-family: %s; font-size: %dpt; font-weight: %d; font-style: %s; text-decoration: %s; "), lf.lfFaceName, iPoints, lf.lfWeight, (lf.lfItalic==0?_T("normal"):_T("italic")), (lf.lfUnderline==0?_T("none"):_T("underline")));
    return sRet;
}


CString GetHTMLTextColorInfo(COLORREF c)
{
    CString sRet;

    sRet.Format(_T("color: #%02x%02x%02x; "), GetRValue(c), GetGValue(c), GetBValue(c));
    return sRet;
}


CString GetHTMLTextColorInfo(const CFmt& fmt)
{
    return GetHTMLTextColorInfo(fmt.GetTextColor().m_rgb);
}


CString GetHTMLFillColorInfo(COLORREF c)
{
    CString sRet;

    sRet.Format(_T("background-color: #%02x%02x%02x; "), GetRValue(c), GetGValue(c), GetBValue(c));
    return sRet;
}


CString GetHTMLFillColorInfo(const CFmt& fmt)
{
    return GetHTMLFillColorInfo(fmt.GetFillColor().m_rgb);
}


CString GetHTMLVertAlignInfo(TAB_VALIGN v)
{
    CString sRet;

    // vertical alignment
    switch (v) {
    case VALIGN_TOP:
        sRet=_T("vertical-align: top; ");
        break;
    case VALIGN_MID:
        sRet=_T("vertical-align: middle; ");
        break;
    case VALIGN_BOTTOM:
        sRet=_T("vertical-align: bottom; ");
        break;
    default:
        ASSERT(FALSE);
        break;
    }
    return sRet;
}


CString GetHTMLVertAlignInfo(const CFmt& fmt)
{
    return GetHTMLVertAlignInfo(fmt.GetVertAlign());
}


CString GetHTMLHorzAlignInfo(TAB_HALIGN h)
{
    CString sRet;

    // horizontal alignment
    switch (h) {
    case HALIGN_LEFT:
        sRet=_T("text-align: left; ");
        break;
    case HALIGN_CENTER:
        sRet=_T("text-align: center; ");
        break;
    case HALIGN_RIGHT:
        sRet=_T("text-align: right; ");
        break;
    default:
        ASSERT(FALSE);
        break;
    }
    return sRet;
}

CString GetHTMLHorzAlignInfo(const CFmt& fmt)
{
    return GetHTMLHorzAlignInfo(fmt.GetHorzAlign());
}


CString GetHTMLIndent(LEFTRIGHT eLR, const CFmt& fmt)
{
    CString sRet;

    int iEm=fmt.GetIndent(eLR)*8/1440;  // rough value, in em's
    if (iEm>0) {
        if (eLR==LEFT) {
            sRet.Format(_T("padding-left: %dem; "),iEm);
        }
        else {
            sRet.Format(_T("padding-right: %dem; "),iEm);
        }
    }
    return sRet;
}


CString GetHTMLLineInfo(LINE eLeft, LINE eTop, LINE eRight, LINE eBottom)
{
    CString sRet;

    // left
    if (eLeft==LINE_THIN) {
        sRet+=_T("border-left: solid black 1px; ");
    }
    else if (eLeft==LINE_THICK) {
        sRet+=_T("border-left: solid black 2px; ");
    }

    // top
    if (eTop==LINE_THIN) {
        sRet+=_T("border-top: solid black 1px; ");
    }
    else if (eTop==LINE_THICK) {
        sRet+=_T("border-top: solid black 2px; ");
    }

    // right
    if (eRight==LINE_THIN) {
        sRet+=_T("border-right: solid black 1px; ");
    }
    else if (eRight==LINE_THICK) {
        sRet+=_T("border-right: solid black 2px; ");
    }

    // bottom
    if (eBottom==LINE_THIN) {
        sRet+=_T("border-bottom: solid black 1px; ");
    }
    else if (eBottom==LINE_THICK) {
        sRet+=_T("border-bottom: solid black 2px; ");
    }
    return sRet;
}


CString GetHTMLLineInfo(const CFmt& fmt)
{
    return GetHTMLLineInfo(fmt.GetLineLeft(), fmt.GetLineTop(), fmt.GetLineRight(), fmt.GetLineBottom());
}

CString BuildHTMLStyleInfo(const CFmt& fmt, int iLogPixelsY)
{
    CString sRet;

    sRet = _T(" { ");

    // font attributes ...
    CFont* pFont=fmt.GetFont();
    LOGFONT lf;
    ASSERT_VALID(pFont);

    pFont->GetLogFont(&lf);
    sRet += GetHTMLFontInfo(lf, iLogPixelsY);

    // color ...
    sRet += GetHTMLTextColorInfo(fmt);

    // fill color...
    sRet += GetHTMLFillColorInfo(fmt);

    // indentation ...
    sRet += GetHTMLIndent(LEFT, fmt) + GetHTMLIndent(RIGHT, fmt);

    // lines ...
    sRet += GetHTMLLineInfo(fmt);

    // alignment
    sRet += GetHTMLHorzAlignInfo(fmt);
    sRet += GetHTMLVertAlignInfo(fmt);


    // end id declaration
    sRet += _T("}");

    return sRet;
}

/////////////////////////////////////////////////////////////////////////////////
//
//          int FindHTMLStyleInfo
//
/////////////////////////////////////////////////////////////////////////////////
int FindHTMLStyleInfo(const CString& sStyle, const CHTMLStyleInfoArray& aStyleInfo)
{
    bool bStyleFound=false;
    int iStyleInfo=0 ;
    for (iStyleInfo=0 ; iStyleInfo<aStyleInfo.GetSize() ; iStyleInfo++) {
        const CHTMLStyleInfo& styleInfo=aStyleInfo[iStyleInfo];
        if (styleInfo.GetStyle()==sStyle) {
            bStyleFound=true;
            break;
        }
    }
    ASSERT(bStyleFound);
    return iStyleInfo;
}

// replace HTML special chars in text string with appropriate esc seqs
CIMSAString EscapeHTMLSpecialChars(const CIMSAString& sSrc)
{
    static CMap<_TCHAR, _TCHAR, CIMSAString, CIMSAString&> gHTMLEscMap;
    if (gHTMLEscMap.GetCount() == 0) {
        // first time - init the map
        gHTMLEscMap.SetAt(_T('<'), CIMSAString(_T("&lt;")));
        gHTMLEscMap.SetAt(_T('>'), CIMSAString(_T("&gt;")));
        gHTMLEscMap.SetAt(_T('&'), CIMSAString(_T("&amp;")));
        gHTMLEscMap.SetAt(_T('"'), CIMSAString(_T("&quot;")));
        gHTMLEscMap.SetAt(_T('\n'), CIMSAString(_T("<br />")));
        gHTMLEscMap.SetAt(_T('\r'), CIMSAString(_T("<br />")));
    }
    CIMSAString sDst;
    // replace special chars with esc sequences
    for (int i = 0; i < sSrc.GetLength(); ++i) {
        _TCHAR c = sSrc.GetAt(i);

        // special handling for spaces
        if (c == _T(' ')) {
            // if it is a space at start of string, after a newline or adjacent to another
            // space then we assume it there for spacing so we replace with html non-breaking space
            // otherwise we assume it is a regular space and leave it alone.
            if (i == 0 || sSrc.GetAt(i-1) == _T(' ') || sSrc.GetAt(i-1) == _T('\n') ||
                (i < sSrc.GetLength() - 1 && sSrc.GetAt(i+1) == _T(' '))) {
                sDst += _T("&nbsp;");
            }
            else {
                sDst += _T(' ');
            }
        }
        else if (c == _T('\r')) {
            // special handling for line feed/cr combo
            sDst += _T("<br />");
            if (i < sSrc.GetLength() - 1 && sSrc.GetAt(i+1) == _T('\n')) {
                ++i; // skip the newline
            }
        }
        else {

            CIMSAString sEscSeq;
            if (gHTMLEscMap.Lookup(c, sEscSeq)) {
                sDst += sEscSeq;
            }
            else {
                sDst += c;
            }
        }
    }
    return sDst;
}

} // anon namespace

CTableGridExporterHTML::CTableGridExporterHTML(int iLogPixelsY)
: m_iLogPixelsY(iLogPixelsY),
  bInHeaderRows(false)
{
}

void CTableGridExporterHTML::StartFile(_tostream& os)
{
    //////////////////////////////////////////////////////////////
    // put out html header ...

    // start HTML ...
    os << (LPCTSTR) _T("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"http://www.w3.org/tr/xhtml1/DTD/xhtml1-strict.dtd\">") << std::endl;
    os << (LPCTSTR) _T("<html>\n<head>\n<meta http-equiv=\"Content-type\" content=\"text/html;charset=iso-8859-1\" /><title>CSPro Generated Table</title>") << std::endl;
}

void CTableGridExporterHTML::EndFile(_tostream& os)
{
    os << (LPCTSTR) _T("</body>\n</html>\n");
}


void CTableGridExporterHTML::StartFormats(_tostream& os)
{
    // start css ...
    os << (LPCTSTR) _T("<style type=\"text/css\">\n<!--\n");

    // data cells should not wrap
    os << (LPCTSTR) _T("td {white-space: nowrap;}") << std::endl;

}

void CTableGridExporterHTML::WriteFormat(_tostream& os, const CFmt& fmt)
{
    // build the style string
    CHTMLStyleInfo styleInfo(BuildHTMLStyleInfo(fmt, m_iLogPixelsY));

    // see if this style is already included in our info array
    bool bFound=false;
    int iStyleInfo=0 ;
    for (iStyleInfo=0 ; iStyleInfo<m_aStyleInfo.GetSize() ; iStyleInfo++) {
        if (m_aStyleInfo[iStyleInfo]==styleInfo) {
            bFound=true;
            break;
        }
    }

    if (!bFound) {
        // new style !
        m_aStyleInfo.Add(styleInfo);

        os << (LPCTSTR) _T(".Style") << iStyleInfo << _T("   ") << (LPCTSTR)styleInfo.GetStyle() << std::endl;
    }
}

void CTableGridExporterHTML::EndFormats(_tostream& os)
{
    // end css definitions
    os << (LPCTSTR) _T("-->\n</style>\n");

    // end HTML heading info ...
    os << (LPCTSTR)  _T("</head>\n<body>\n");
}

void CTableGridExporterHTML::StartTable(_tostream& os, int iNumCols)
{
    os << (LPCTSTR) _T("<table width=\"100%\" cellspacing=\"0\" cellpadding=\"3\">") << std::endl;

    m_iNumCols = iNumCols;
}

void CTableGridExporterHTML::EndTable(_tostream& os)
{
    os << (LPCTSTR) _T("</table>") << std::endl;

}

void CTableGridExporterHTML::WriteTitle(_tostream& os, const CFmt& fmt, const CString& sTitle)
{
    if (!sTitle.IsEmpty()) {
        // title style
        CString sTitleStyle=BuildHTMLStyleInfo(fmt, m_iLogPixelsY);
        int iTitleStyle=FindHTMLStyleInfo(sTitleStyle, m_aStyleInfo);

        // write title out as caption element for table
        os << (LPCTSTR) _T("<caption class=\"Style") << iTitleStyle << _T("\">\n");
        os << (LPCTSTR) EscapeHTMLSpecialChars(sTitle) << std::endl;
        os << (LPCTSTR) _T("</caption> ") << std::endl;
    }
}

void CTableGridExporterHTML::WriteSubTitle(_tostream& os, const CFmt& fmt, const CString& sSubTitle)
{
    if (!sSubTitle.IsEmpty()) {
        // subtitle style
        CString sSubTitleStyle=BuildHTMLStyleInfo(fmt, m_iLogPixelsY);
        int iSubTitleStyle=FindHTMLStyleInfo(sSubTitleStyle, m_aStyleInfo);

        // write out subtitle as first row of table - ideally it would be part
        // of caption element (with title) but HTML does not allow divs within
        // the caption so we can't set separate alignment and other block styles
        // on the title and subtitle if they are combined
        os << (LPCTSTR) _T("\t<tr>") << std::endl;
        os << (LPCTSTR) _T("\t\t<th class=\"Style") << iSubTitleStyle << _T("\" colspan=\"") << m_iNumCols <<(LPCTSTR) _T("\">");
        os << (LPCTSTR) EscapeHTMLSpecialChars(sSubTitle);
        os << (LPCTSTR) _T("</th>") << std::endl;
        os << (LPCTSTR) _T("\t</tr>") << std::endl;
    }
}

void CTableGridExporterHTML::StartHeaderRows(_tostream& /*os*/)
{
    bInHeaderRows = true;
}

void CTableGridExporterHTML::EndHeaderRows(_tostream& /*os*/)
{
    bInHeaderRows = false;
}


CIMSAString CTableGridExporterHTML::HeadersToString(const CDWordArray& aHeaders)
{
    CIMSAString sHeaderString;
    for (int i = 0; i < aHeaders.GetCount(); ++i) {
        sHeaderString += _T("r");
        sHeaderString += IntToString((int)aHeaders.GetAt(i));
        if (i < aHeaders.GetCount() - 1) {
            sHeaderString += _T(" ");
        }
    }
    return sHeaderString;
}

CIMSAString CTableGridExporterHTML::HeadersToString(const CArray<CJoinRegion>& aHeaders)
{
    CIMSAString sHeaderString;
    for (int i = 0; i < aHeaders.GetCount(); ++i) {
        sHeaderString += _T("c");
        sHeaderString += IntToString(static_cast<int>(aHeaders.GetAt(i).iStartRow));
        sHeaderString += _T(".");
        sHeaderString += IntToString(aHeaders.GetAt(i).iStartCol);
        if (i < aHeaders.GetCount() - 1) {
            sHeaderString += _T(" ");
        }
    }
    return sHeaderString;
}

void CTableGridExporterHTML::StartRow(_tostream& os, int /*iRow*/, const CDWordArray& aRowHeaders)
{
    os << (LPCTSTR) _T("\t<tr>") << std::endl;

    // save off row headers to be used as headers for each cell in the row
    m_sRowHeaderString = HeadersToString(aRowHeaders);
}

void CTableGridExporterHTML::EndRow(_tostream& os)
{
    os << (LPCTSTR) _T("\t</tr>") << std::endl;
}

void CTableGridExporterHTML::WriteCell(_tostream& os,
                        int iCol,
                        int iRow,
                        const CFmt& fmt,
                        const CString& sCellData,
                        const CJoinRegion& join,
                        const CArray<CJoinRegion>& aColHeaders)
{
    if (join.iStartRow != iRow || join.iStartCol!=iCol) {
        // inside of a merged area ... skip this cell
        return;
    }

    // determine if this is a header cell (stub or spanner/col head)
    bool bHeaderCell;
    if (fmt.GetID() == FMT_ID_STUB || fmt.GetID() == FMT_ID_STUBHEAD) {
        bHeaderCell = true; // stub
    }
    else if ((fmt.GetID() == FMT_ID_CAPTION || fmt.GetID() == FMT_ID_AREA_CAPTION)
             && iCol == 0) {
         bHeaderCell = true; // caption in first col treated as header but
                             // caption in the middle is treated as data for html table
                             // since it is uncool to have th cells inside the table
    }
    else if (fmt.GetID() == FMT_ID_SPANNER || fmt.GetID() == FMT_ID_COLHEAD) {
        bHeaderCell = true; // column header or spanner
    }
    else {
        bHeaderCell = false; // data cell
    }

    // start cell declaration ...
    os << (LPCTSTR) _T("\t\t");
    if (bHeaderCell) {
        os << (LPCTSTR) _T("<th ");
    }
    else {
        os << (LPCTSTR) _T("<td ");
    }

    // add id for row stubs/col heads to be used in headers,
    if ((fmt.GetID() == FMT_ID_CAPTION || fmt.GetID() == FMT_ID_AREA_CAPTION ||
        (fmt.GetID() == FMT_ID_STUBHEAD && !(sCellData.IsEmpty())))
        && iCol == 0) {
         // for row stubs ids are "r1" for row 1, "r2" for row etc...
         os << (LPCTSTR) _T("id=\"r") << iRow << _T("\" ");
    }
    else if (fmt.GetID() == FMT_ID_SPANNER || fmt.GetID() == FMT_ID_COLHEAD) {
        // for col headers/spanners ids are c1.1 for row 1 col 1, c1.2 row 1 col 2,...
        // need both col and row in this case since can have both col head and spanner
        // for same col number.
        os << (LPCTSTR) _T("id=\"c") << iRow << _T(".") << iCol << _T("\" ");
    }

    // add headers
    const CIMSAString sColHeaderString(HeadersToString(aColHeaders));
    if (!SO::IsBlank(m_sRowHeaderString) || !SO::IsBlank(sColHeaderString)) {
        os << (LPCTSTR) _T("headers=\"");
        if (!SO::IsBlank(sColHeaderString)) {
            os << (LPCTSTR) sColHeaderString;
            if (!SO::IsBlank(m_sRowHeaderString)) {
                os << (LPCTSTR) _T(" ");
            }
        }
        os << (LPCTSTR) m_sRowHeaderString;
        os << (LPCTSTR) _T("\" ");
    }

    // link to format style ...
    CString sStyle=BuildHTMLStyleInfo(fmt, m_iLogPixelsY);
    int iStyleInfo=FindHTMLStyleInfo(sStyle, m_aStyleInfo);
    os << (LPCTSTR) _T("class=\"Style") << iStyleInfo << _T("\" ");

    // column merge
    if (join.iStartCol != join.iEndCol) {
        ASSERT(iCol == join.iStartCol);
        os << (LPCTSTR) _T("colspan=\"") << (join.iEndCol - join.iStartCol + 1) << (LPCTSTR)_T("\" ");
    }

    // row merge
    if (join.iStartRow != join.iEndRow) {
        ASSERT(iRow == join.iStartRow);
        os << (LPCTSTR) _T("rowspan=\"") << (join.iEndRow - join.iStartRow + 1) << (LPCTSTR)_T("\" ");
    }

    os << (LPCTSTR)_T(">");

    // cell contents
    CString sCellContents = EscapeHTMLSpecialChars(sCellData);
    if (sCellContents.IsEmpty()) {
        sCellContents=_T("&nbsp;");
    }
    os << (LPCTSTR) sCellContents;

    // end cell
    if (bHeaderCell) {
        os << (LPCTSTR) _T("</th>");
    }
    else {
        os << (LPCTSTR) _T("</td>");
    }
    os << std::endl;
}

