//***************************************************************************
//  File name: RTFExpt.cpp
//
//  Description:
//  Export grid as table in MS Rich Text Format (RTF).
//
//***************************************************************************

#include "StdAfx.h"
#include "RTFExpt.h"
#include <ostream>

IMPLEMENT_DYNAMIC(CRTFFontInfo, CObject)

namespace {

CString GetRTFFontInfo(int iFontInfo, LOGFONT lf, int iLogPixelsY) {
    CString sRet;

    int iHalfPoints=abs(MulDiv(lf.lfHeight,72,iLogPixelsY)*2);
    sRet.Format(_T("\\fs%d\\f%d"), iHalfPoints, iFontInfo);

    if (lf.lfWeight>=FW_BOLD) {
        // bold on
        sRet+=_T("\\b");
    }
    else {
        // bold off
        sRet+=_T("\\b0");
    }

    if (lf.lfItalic) {
        // italics on
        sRet+=_T("\\i");
    }
    else {
        // italics off
        sRet+=_T("\\i0");
    }

    return sRet;
}


CString GetRTFTextColorInfo(int iColorText)
{
    CString sRet;

    sRet.Format(_T("\\cf%d "), iColorText);
    return sRet;
}


CString GetRTFFillColorInfo(int iColorFill)
{
    CString sRet;

    sRet.Format(_T("\\clcbpat%d "), iColorFill);
    return sRet;
}


CString GetRTFLineInfo(LINE eLeft, LINE eTop, LINE eRight, LINE eBottom)
{
    CString sRet;

    // left
    if (eLeft==LINE_THIN) {
        sRet+=_T("\\clbrdrl\\brdrs ");
    }
    else if (eLeft==LINE_THICK) {
        sRet+=_T("\\clbrdrl\\brdrth ");
    }

    // top
    if (eTop==LINE_THIN) {
        sRet+=_T("\\clbrdrt\\brdrs ");
    }
    else if (eTop==LINE_THICK) {
        sRet+=_T("\\clbrdrt\\brdrth ");
    }

    // right
    if (eRight==LINE_THIN) {
        sRet+=_T("\\clbrdrr\\brdrs ");
    }
    else if (eRight==LINE_THICK) {
        sRet+=_T("\\clbrdrr\\brdrth ");
    }

    // bottom
    if (eBottom==LINE_THIN) {
        sRet+=_T("\\clbrdrb\\brdrs ");
    }
    else if (eBottom==LINE_THICK) {
        sRet+=_T("\\clbrdrb\\brdrth ");
    }
    return sRet;
}


CString GetRTFLineInfo(const CFmt& fmt)
{
    return GetRTFLineInfo(fmt.GetLineLeft(), fmt.GetLineTop(), fmt.GetLineRight(), fmt.GetLineBottom());
}


CString GetRTFVertAlignInfo(TAB_VALIGN v)
{
    CString sRet;

    // vertical alignment
    switch (v) {
    case VALIGN_TOP:
        sRet=_T("\\clvertalt ");
        break;
    case VALIGN_MID:
        sRet=_T("\\clvertalc ");
        break;
    case VALIGN_BOTTOM:
        sRet=_T("\\clvertalb ");
        break;
    default:
        ASSERT(FALSE);
        break;
    }
    return sRet;
}

CString GetRTFVertAlignInfo(const CFmt& fmt)
{
    return GetRTFVertAlignInfo(fmt.GetVertAlign());
}


CString GetRTFHorzAlignInfo(TAB_HALIGN h)
{
    CString sRet;

    // horizontal alignment
    switch (h) {
    case HALIGN_LEFT:
        sRet=_T("\\ql ");
        break;
    case HALIGN_CENTER:
        sRet=_T("\\qc ");
        break;
    case HALIGN_RIGHT:
        sRet=_T("\\qr ");
        break;
    default:
        ASSERT(FALSE);
        break;
    }
    return sRet;
}

CString GetRTFHorzAlignInfo(const CFmt& fmt)
{
    return GetRTFHorzAlignInfo(fmt.GetHorzAlign());
}

int FindRTFFontInfo(const CString& sFace, const CRTFFontInfoArray& aFontInfo)
{
    bool bFound=false;
    int iFontInfo=0;
    for (iFontInfo=0 ; iFontInfo<aFontInfo.GetSize() ; iFontInfo++) {
        const CRTFFontInfo& info=aFontInfo[iFontInfo];
        if (info.GetFace()==sFace) {
            bFound=true;
            break;
        }
    }
    ASSERT(bFound);
    return iFontInfo;
}


int FindRTFColorInfo(COLORREF rgb, const CRTFColorInfoArray& aColorInfo)
{
    bool bFound=false;
    int iColorInfo=0 ;
    for (iColorInfo=0 ; iColorInfo<aColorInfo.GetSize() ; iColorInfo++) {
        COLORREF info=aColorInfo[iColorInfo];
        if (info==rgb) {
            bFound=true;
            break;
        }
    }
    ASSERT(bFound);
    return iColorInfo;
}

} // anon namespace

CTableGridExporterRTF::CTableGridExporterRTF(int iLeftMargin,
                                             int iRightMargin,
                                             int iLogPixelsY)
: m_iLeftMargin(iLeftMargin)
, m_iRightMargin(iRightMargin)
, m_iLogPixelsY(iLogPixelsY)
{
}

void CTableGridExporterRTF::StartFile(_tostream& os)
{
    //CString start(_T("{\\rtf1\\ansi \\def0\r\n"));
    os << (LPCTSTR)_T("{\\rtf1\\ansi \\def0\r\n");
    //os.write(start,start.GetLength());
}

void CTableGridExporterRTF::EndFile(_tostream& os)
{
    //CString end( _T("}\r\n"));
    os <<(LPCTSTR) _T("}\r\n");
    //os.write(end,end.GetLength());
}

void CTableGridExporterRTF::StartFormats(_tostream& os)
{
    // add in black and white, so that we are sure to have them available
    m_aColorInfo.Add(rgbBlack);
    m_aColorInfo.Add(rgbWhite);
}

void CTableGridExporterRTF::WriteFormat(_tostream& os, const CFmt& fmt)
{
    // add font to font list
    CFont* pFont=fmt.GetFont();
    if (pFont) {
        // this fmt has an active font!
        LOGFONT lf;
        pFont->GetLogFont(&lf);
        CRTFFontInfo info(lf.lfFaceName, lf.lfPitchAndFamily);

        // see if we've seen this font face before ...
        bool bFoundFontInfo=false;
        for (int iFontInfo=0 ; iFontInfo<m_aFontInfo.GetSize() ; iFontInfo++) {
            if (m_aFontInfo[iFontInfo]==info) {
                bFoundFontInfo=true;
                break;
            }
        }
        if (!bFoundFontInfo) {
            // add this font face
            m_aFontInfo.Add(info);
        }
    }

    // add text color to color list
    FMT_COLOR colorText=fmt.GetTextColor();
    if (!colorText.m_bUseDefault) {
        COLORREF rgb=colorText.m_rgb;

        // see if we've seen this font face before ...
        bool bFoundColorInfo=false;
        for (int iColorInfo=0 ; iColorInfo<m_aColorInfo.GetSize() ; iColorInfo++) {
            if (m_aColorInfo[iColorInfo]==rgb) {
                bFoundColorInfo=true;
                break;
            }
        }
        if (!bFoundColorInfo) {
            // add this color ...
            m_aColorInfo.Add(rgb);
        }
    }

    // add fill color to color list
    FMT_COLOR colorFill=fmt.GetFillColor();
    if (!colorFill.m_bUseDefault) {
        COLORREF rgb=colorFill.m_rgb;

        // see if we've seen this font face before ...
        bool bFoundColorInfo=false;
        for (int iColorInfo=0 ; iColorInfo<m_aColorInfo.GetSize() ; iColorInfo++) {
            if (m_aColorInfo[iColorInfo]==rgb) {
                bFoundColorInfo=true;
                break;
            }
        }
        if (!bFoundColorInfo) {
            // add this color ...
            m_aColorInfo.Add(rgb);
        }
    }
}

void CTableGridExporterRTF::EndFormats(_tostream& os)
{
    ////////////////////////////////////////////////////////////////////////////////
    // add fonts to the RTF font table ...
    os << (LPCTSTR)_T("{\\fonttbl{f");

    for (int iFontInfo=0 ; iFontInfo<m_aFontInfo.GetSize() ; iFontInfo++) {
        CRTFFontInfo info=m_aFontInfo[iFontInfo];

        os << (LPCTSTR)_T("\\f");
        os << iFontInfo;

        // bits 4-7 indicate the pitch and family ... bug fix    csc 3/29/05
        if ((info.GetPitchAndFamily()&FF_SWISS)==FF_SWISS) {
            os << (LPCTSTR)_T("\\fswiss ");
        }
        else if ((info.GetPitchAndFamily()&FF_ROMAN)==FF_ROMAN) {
            os << (LPCTSTR)_T("\\froman ");
        }
        else if ((info.GetPitchAndFamily()&FF_MODERN)==FF_MODERN) {
            os << (LPCTSTR)_T("\\fmodern ");
        }
        else if ((info.GetPitchAndFamily()&FF_SCRIPT)==FF_SCRIPT) {
            os << (LPCTSTR)_T("\\fmodern ");
        }
        else if ((info.GetPitchAndFamily()&FF_DECORATIVE)==FF_DECORATIVE) {
            os << (LPCTSTR)_T("\\fdecor ");
        }
        else {
            os << (LPCTSTR)_T("\\fnil ");
        }
        os << (LPCTSTR)info.GetFace();
        os << (LPCTSTR) _T(";}");

        if (iFontInfo<m_aFontInfo.GetSize()-1) {
            os << (LPCTSTR)_T("{");
        }
    }
    os << (LPCTSTR)_T("}\r\n");

    ////////////////////////////////////////////////////////////////////////////////
    // add colors to the color table ...
    CString sStuff;
    os << (LPCTSTR)_T("{\\colortbl ");
    for (int iColorInfo=0 ; iColorInfo<m_aColorInfo.GetSize() ; iColorInfo++) {
        COLORREF rgb=m_aColorInfo[iColorInfo];
        sStuff.Format(_T("\\red%d\\green%d\\blue%d;"), GetRValue(rgb), GetGValue(rgb), GetBValue(rgb));
        os << (LPCTSTR)sStuff;
    }
    os << (LPCTSTR)_T("}\r\n");
}

void CTableGridExporterRTF::StartTable(_tostream& os, int iNumCols)
{
    UNREFERENCED_PARAMETER(iNumCols);

    os << (LPCTSTR)_T("\\margl") << m_iLeftMargin << (LPCTSTR)_T("\\margr") << m_iRightMargin << (LPCTSTR)_T("\r\n");

    os << (LPCTSTR)_T("\\pard \\sl0");
    os << (LPCTSTR)_T("\r\n");
}

void CTableGridExporterRTF::EndTable(_tostream& os)
{
    os << (LPCTSTR)_T("\\pard \r\n");
}

void CTableGridExporterRTF::WriteTitle(_tostream& os, const CFmt& fmt, const CString& sTitle)
{
    os << (LPCTSTR)_T("{");

    // horizontal alignment
    os << (LPCTSTR)GetRTFHorzAlignInfo(fmt);

    // text color info
    int iColorText=FindRTFColorInfo(fmt.GetTextColor().m_rgb, m_aColorInfo);
    os <<(LPCTSTR) GetRTFTextColorInfo(iColorText);

    // fill color info
    CString sHighlight;
    sHighlight.Format(_T("\\highlight%d "), FindRTFColorInfo(fmt.GetFillColor().m_rgb, m_aColorInfo));
    os << (LPCTSTR)sHighlight;

    // font info (see CRTFFontInfo for more info)
    LOGFONT lf;
    CFont* pFont=fmt.GetFont();
    ASSERT(pFont);
    pFont->GetLogFont(&lf);
    int iFontInfo=FindRTFFontInfo(lf.lfFaceName, m_aFontInfo);
    os << (LPCTSTR)GetRTFFontInfo(iFontInfo, lf, m_iLogPixelsY);

    // title text
    CString sRTFTitleText;
    CString rtfChar;
    for(int i = 0; i < sTitle.GetLength(); i++){
        rtfChar.Format(_T("\\u%hd?"),sTitle[i]);
        sRTFTitleText.Append(rtfChar);
    }

    os << (LPCTSTR)_T("\r\n") << (LPCTSTR)sRTFTitleText << (LPCTSTR)_T("\r\n\\par \\pard\r\n");

    os << (LPCTSTR)_T("}\r\n");
}

void CTableGridExporterRTF::WriteSubTitle(_tostream& os, const CFmt& fmt, const CString& sSubTitle)
{
    os << (LPCTSTR)_T("{");

    // horizontal alignment
    os << (LPCTSTR)GetRTFHorzAlignInfo(fmt);

    // text color info
    int iColorText=FindRTFColorInfo(fmt.GetTextColor().m_rgb, m_aColorInfo);
    os << (LPCTSTR)GetRTFTextColorInfo(iColorText);

    // fill color info
    CString sHighlight;
    sHighlight.Format(_T("\\highlight%d"), FindRTFColorInfo(fmt.GetFillColor().m_rgb, m_aColorInfo));
    os << (LPCTSTR)sHighlight;

    // font info (see CRTFFontInfo for more info)
    LOGFONT lf;
    CFont* pFont=fmt.GetFont();
    ASSERT(pFont);
    pFont->GetLogFont(&lf);
    int iFontInfo=FindRTFFontInfo(lf.lfFaceName, m_aFontInfo);
    os << (LPCTSTR)GetRTFFontInfo(iFontInfo, lf, m_iLogPixelsY);

    // title text
     // title text
    CString sRTFSubTitle;
    CString rtfChar;
    for(int i = 0; i < sRTFSubTitle.GetLength(); i++){
        rtfChar.Format(_T("\\u%hd?"),sSubTitle[i]);
        sRTFSubTitle.Append(rtfChar);
    }
    os << (LPCTSTR)_T("\r\n") << sRTFSubTitle << (LPCTSTR)_T("\r\n\\par \\pard\r\n");

    os << (LPCTSTR)_T("}\r\n");
}

void CTableGridExporterRTF::StartRow(_tostream& os, int iRow, const CDWordArray& aRowHeaders)
{
    m_currRowCells.RemoveAll();
}

void CTableGridExporterRTF::EndRow(_tostream& os)
{
    //  Get widths
    const int iTotalWidth = 65 * 144;
    const int iStubWidth = iTotalWidth / 4;
    const int iNumCols = m_currRowCells.GetCount();
    const int iColWidth = iNumCols > 0 ?
        (iTotalWidth - iStubWidth) / iNumCols :
        iTotalWidth - iStubWidth;
    const int iColStart = 0;
    int iWidth=0;

    bool bSomeOutput=false; //=true if this row has some output

    os << (LPCTSTR)_T("\\trowd \\trgaph60\r\n");

    // write out first pass cell data (widths, merge, align, line, fill color)
    for (int iCell = 0; iCell < iNumCols; ++iCell) {

        const CRTFCellInfo& cellInfo = m_currRowCells.GetAt(iCell);

        bool bHorzMerge = (cellInfo.iCol != cellInfo.join.iStartCol);
        bool bVertMerge = (cellInfo.iRow != cellInfo.join.iStartRow);

        bSomeOutput=true;

        if (bHorzMerge) {
            os << (LPCTSTR)_T("\\clmrg ");
        }
        else {
            os << (LPCTSTR)_T("\\clmgf ");
        }
        if (bVertMerge) {
            os << (LPCTSTR)_T("\\clvmrg ");
        }
        else {
            os << (LPCTSTR)_T("\\clvmgf ");
        }

        if (!bHorzMerge) {

            os << (LPCTSTR)GetRTFLineInfo(cellInfo.fmt);

            // cell vertical alignment
            os << (LPCTSTR)GetRTFVertAlignInfo(cellInfo.fmt);

            // cell fill color ...
            int iColorFill=FindRTFColorInfo(cellInfo.fmt.GetFillColor().m_rgb, m_aColorInfo);
            os << (LPCTSTR)GetRTFFillColorInfo(iColorFill);
        }

        if (cellInfo.iCol == iColStart) {
            iWidth = iStubWidth;
        }
        else {
            iWidth += iColWidth;
        }

        os << (LPCTSTR)_T("\\cellx");
        os << iWidth;

        os << (LPCTSTR)_T("\r\n");
    }

    os << (LPCTSTR)_T("\\intbl\r\n");

    // write out second pass cell data
    for (int iCell = 0; iCell < iNumCols; ++iCell) {

        const CRTFCellInfo& cellInfo = m_currRowCells.GetAt(iCell);

            // horz alignment ...
            os << (LPCTSTR)GetRTFHorzAlignInfo(cellInfo.fmt);

            // cell data (leave blank if part of a merge)
            bool bMerge = (cellInfo.iRow != cellInfo.join.iStartRow || cellInfo.iCol != cellInfo.join.iStartCol);
            CString sData = bMerge ? CString() : cellInfo.sCellData;

            // font and color info ...
            CString sFontInfo;
            if (!cellInfo.sCellData.IsEmpty()) {

                // color info
                int iColorText=FindRTFColorInfo(cellInfo.fmt.GetTextColor().m_rgb, m_aColorInfo);
                os << (LPCTSTR)GetRTFTextColorInfo(iColorText);

                // figure out which font table entry we should use (see CRTFFontInfo for more info)
                LOGFONT lf;
                CFont* pFont=cellInfo.fmt.GetFont();
                ASSERT(pFont);
                pFont->GetLogFont(&lf);
                int iFontInfo=FindRTFFontInfo(lf.lfFaceName, m_aFontInfo);
                int iHalfPoints=abs(MulDiv(lf.lfHeight,72,m_iLogPixelsY)*2);
                sFontInfo=GetRTFFontInfo(iFontInfo, lf, m_iLogPixelsY)+_T("\r\n");
            }
            os << (LPCTSTR)sFontInfo << (LPCTSTR)cellInfo.sCellData;

            // end this cell
            os << (LPCTSTR)_T("\\cell\r\n");
    }
    if (bSomeOutput) {
        os << (LPCTSTR)_T("\\row\r\n");
    }
}

void CTableGridExporterRTF::WriteCell(_tostream& os,
                        int iCol,
                        int iRow,
                        const CFmt& fmt,
                        const CString& sCellData,
                        const CJoinRegion& join,
                        const CArray<CJoinRegion>& aColHeaders)
{
    UNREFERENCED_PARAMETER(aColHeaders);

    const int iNewCell = m_currRowCells.GetCount();
    m_currRowCells.SetSize(iNewCell + 1);
    CRTFCellInfo& cellInfo = m_currRowCells.GetAt(iNewCell);
    cellInfo.iCol = iCol;
    cellInfo.iRow = iRow;
    cellInfo.fmt = fmt;
if(fmt.GetID()!= FMT_ID_DATACELL){
    CString sRTFCellData;
    CString rtfChar;
    for(int i = 0; i < sCellData.GetLength(); i++){
        rtfChar.Format(_T("\\u%hd?"),sCellData[i]);
        sRTFCellData.Append(rtfChar);
    }
    cellInfo.sCellData = sRTFCellData;
}
else{
    cellInfo.sCellData = sCellData;
}
    cellInfo.join = join;
}

