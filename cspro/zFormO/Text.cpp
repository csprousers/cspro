#include "StdAfx.h"
#include "Text.h"


// --------------------------------------------------------------------------
// CDEBox
// --------------------------------------------------------------------------

IMPLEMENT_DYNAMIC(CDEText, CDEItemBase)


CDEText::CDEText(const CString& initial_text/* = SO::EmptyCString*/)
    :   m_font(PortableFont::TextDefault),
        m_useDefaultFont(true)
{
    SetItemType(Text);

    if( &initial_text != &SO::EmptyCString )
        SetText(initial_text);
}


bool CDEText::Build(CSpecFile& frmFile, bool bSilent/* = false*/)
{
    CString csCmd, csArg;
    CString csText;                 // gsf 8-mar-01

    bool bDone = false;
    bool bGotTextAlready = false;            // gsf 16-mar-01

    SetColor(PortableColor::Black);
    //Initially Set the default font

    while (!bDone && frmFile.GetLine(csCmd, csArg) == SF_OK) {

        ASSERT (csCmd.GetLength() > 0);

        if (csCmd[0] == '.')    // then it's a comment line, ignore
            continue;

        if (csCmd[0] == '[') {

            frmFile.UngetLine();  // so calling func can figure out what we have
            bDone = true;
        }
        else if( csCmd.CompareNoCase(FRM_CMD_NAME) == 0 )
            SetName(csArg);
        else if( csCmd.CompareNoCase(FRM_CMD_POSITION) == 0 )
            SetDims (csArg);
        else if( csCmd.CompareNoCase(FRM_CMD_TEXT) == 0 ) {
            // gsf 8-mar-01
            if (bGotTextAlready) {
                // this means it's multiline text
                csText += _T("\r\n");
            }
            csText += csArg;
            SetText(csText);
            bGotTextAlready = true;
        }

        else if( csCmd.CompareNoCase(FRM_CMD_HORZ_ALIGN)==0) {
            if (csArg.CompareNoCase(FRM_CMD_ALIGN_LEFT)==0) {
                m_horizontalAlignment = HorizontalAlignment::Left;
            }
            else if (csArg.CompareNoCase(FRM_CMD_ALIGN_CENTER)==0) {
                m_horizontalAlignment = HorizontalAlignment::Center;
            }
            else if (csArg.CompareNoCase(FRM_CMD_ALIGN_RIGHT)==0) {
                m_horizontalAlignment = HorizontalAlignment::Right;
            }
            else  {
                ASSERT(false);
            }
        }

        else if( csCmd.CompareNoCase(FRM_CMD_VERT_ALIGN)==0) {
            if (csArg.CompareNoCase(FRM_CMD_ALIGN_TOP)==0) {
                m_verticalAlignment = VerticalAlignment::Top;
            }
            else if (csArg.CompareNoCase(FRM_CMD_ALIGN_MIDDLE)==0) {
                m_verticalAlignment = VerticalAlignment::Middle;
            }
            else if (csArg.CompareNoCase(FRM_CMD_ALIGN_BOTTOM)==0) {
                m_verticalAlignment = VerticalAlignment::Bottom;
            }
            else  {
                ASSERT(false);
            }
        }

        else if( csCmd.CompareNoCase(FRM_CMD_LOGFONT) == 0 ) {
            if(!csArg.IsEmpty()) {
                m_font.BuildFromPre80String(csArg);
                m_useDefaultFont = false;
            }
        }

        else if( csCmd.CompareNoCase(FRM_CMD_FORMNUM) == 0 ) {
            SetFormNum (csArg);
        }

        else if( csCmd.CompareNoCase(FRM_CMD_COLOR) == 0 ) {
            SetColor(PortableColor::FromCOLORREF(_ttoi(csArg)));
        }

        else {                      // Incorrect attribute
            if (!bSilent) {
                ErrorMessage::Display(FormatText(_T("Incorrect [Text] attribute\n\n%s"), (LPCTSTR)csCmd));
            }
            ASSERT(false);
        }
    }

    return bDone;
}


void CDEText::Save(CSpecFile& frmFile, bool bWriteHdr) const 
{
    CString csOutput, csTemp;

    if (bWriteHdr) {
        frmFile.PutLine(HEAD_TEXT);
    }

    if (! GetName().IsEmpty()) {
        frmFile.PutLine(FRM_CMD_NAME, GetName());
    }

    if (!GetDims().IsRectEmpty()) {
        // when a text field is the text tag for a field
        // w/in a roster, the dims aren't applicable
        WriteDimsToStr (csOutput);
        frmFile.PutLine(FRM_CMD_POSITION, csOutput);
    }

    // gsf 8-mar-01
    CString csText = GetText();
    TCHAR* pszText = csText.GetBuffer(0);
    csText.ReleaseBuffer();

    // gsf 16-mar-01:  had to rewrite this because
    // blank lines in multitext were being skipped over

    // check for multiline text
    TCHAR* pszFind = _tcschr(pszText, '\r');
    if (pszFind == NULL) {
        frmFile.PutLine(FRM_CMD_TEXT, csText);
    }
    else {
        int iStrlen = csText.GetLength();
        TCHAR* pszSource = pszText;
        TCHAR cDest[1000];
        while (true) {
            pszFind = _tcschr(pszSource, '\r');
            if (pszFind == NULL) {
                pszFind = pszText + iStrlen;
            }
            TCHAR* pszDest = cDest;
            int iLen = pszFind - pszSource;
            memmove (pszDest, pszSource, iLen * sizeof(TCHAR));
            *(pszDest + iLen) = NULL;
            frmFile.PutLine(FRM_CMD_TEXT, pszDest);

            // skip over cr/lf pair
            pszSource = pszFind + 2;
            if (pszSource - pszText >= iStrlen) {
                break;
            }
        }
    }

    if (GetFormNum() != NONE) {
        frmFile.PutLine(FRM_CMD_FORMNUM, GetFormNum()+1);
    }

    if (!m_useDefaultFont) {
        frmFile.PutLine(FRM_CMD_LOGFONT, m_font.GetPre80String());
    }

    if (m_horizontalAlignment.has_value()) {
        switch(*m_horizontalAlignment) {
        case HorizontalAlignment::Left:
            csTemp = FRM_CMD_ALIGN_LEFT;
            break;
        case HorizontalAlignment::Center:
            csTemp = FRM_CMD_ALIGN_CENTER;
            break;
        case HorizontalAlignment::Right:
            csTemp = FRM_CMD_ALIGN_RIGHT;
            break;
        default:
            ASSERT(FALSE);
        }
        frmFile.PutLine(FRM_CMD_HORZ_ALIGN, csTemp);
    }
    if (m_verticalAlignment.has_value()) {
        switch(*m_verticalAlignment) {
        case VerticalAlignment::Top:
            csTemp = FRM_CMD_ALIGN_TOP;
            break;
        case VerticalAlignment::Middle:
            csTemp = FRM_CMD_ALIGN_MIDDLE;
            break;
        case VerticalAlignment::Bottom:
            csTemp = FRM_CMD_ALIGN_BOTTOM;
            break;
        default:
            ASSERT(FALSE);
        }
        frmFile.PutLine(FRM_CMD_VERT_ALIGN, csTemp);
    }

    if (m_color != PortableColor::Black) {
        frmFile.PutLine(FRM_CMD_COLOR, (int)m_color.ToCOLORREF());
    }

    frmFile.PutLine(_T(" "));             // blank line to sep the groups
}


void CDEText::serialize(Serializer& ar)
{
    CDEItemBase::serialize(ar);

    if( ( ar.IsSaving() && !FormSerialization::isRepeated(this, ar) ) ||
        ( ar.IsLoading() && !FormSerialization::CheckRepeated(this, ar) ) )
    {
        ar & m_font
           & m_useDefaultFont;

        if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
        {
            ar.SerializeEnum(m_horizontalAlignment);
            ar.SerializeEnum(m_verticalAlignment);
            ar & m_color;
        }

        else
        {
            ar.SerializeEnum(m_horizontalAlignment.emplace());
            ar.SerializeEnum(m_verticalAlignment.emplace());
            m_color = PortableColor::FromCOLORREF(ar.Read<COLORREF>());
        }
    }
}


namespace
{
    template<bool DrawText>
    CRect DrawWorker(CDC* pDC, const CDEText& text)
    {
        int saved_dc = pDC->SaveDC();

        pDC->SelectObject(text.GetFont().GetCFont());

        if constexpr(DrawText)
        {
            pDC->SetTextColor(text.GetColor().ToCOLORREF());
        }

        CRect drawn_rect = text.GetDims();
        LONG width = 0;
        LONG y = drawn_rect.top;

        SO::ForeachLine(UndelimitCRLF(text.GetText()), true,
            [&](wstring_view line)
            {
                CSize size = pDC->GetTextExtent(line.data(), line.length());

                if constexpr(DrawText)
                {
                    pDC->TextOut(drawn_rect.left, y, line.data(), line.length());
                }

                width = std::max(width, size.cx);
                y += size.cy;

                return true;
            });

        drawn_rect.right = drawn_rect.left + width;
        drawn_rect.bottom = y;

        pDC->RestoreDC(saved_dc);

        return drawn_rect;
    }
}

void CDEText::DrawMultiline(CDC* pDC) const
{
    DrawWorker<true>(pDC, *this);
}

void CDEText::DrawMultiline(CDC* pDC)
{
    CRect drawn_rect = DrawWorker<true>(pDC, *this);
    SetDims(drawn_rect);    
}

CSize CDEText::CalculateDimensions(CDC* pDC) const
{
    CRect drawn_rect = DrawWorker<false>(pDC, *this);
    return drawn_rect.Size();
}



// --------------------------------------------------------------------------
// CDETextSet
// --------------------------------------------------------------------------

void CDETextSet::RemoveText(size_t index)
{
    ASSERT(index < m_texts.size());
    m_texts.erase(m_texts.begin() + index);
}


void CDETextSet::RemoveAllTexts()
{
    m_texts.clear();
}


void CDETextSet::serialize(Serializer& ar)
{
    ar & m_texts;
}
