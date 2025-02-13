#include "StdAfx.h"
#include "CustomProperties.h"


namespace PropertyGrid
{
    // ------------------------------------------
    // HeadingProperty
    // ------------------------------------------
    HeadingProperty::HeadingProperty(const CString& heading_name)
        :   CMFCPropertyGridProperty(heading_name)
    {
    }

    void HeadingProperty::OnDrawName(CDC* pDC, CRect rect)
    {
        // change the font to bold
        LOGFONT lf;
        pDC->GetCurrentFont()->GetLogFont(&lf);
        lf.lfWeight = FW_BOLD;

        CFont new_font;
        new_font.CreateFontIndirect(&lf);
        pDC->SelectObject(&new_font);

        // position the text and draw it
        rect.top += ( rect.Height() - abs(lf.lfHeight) ) - 4;
        rect.left += 3;
        pDC->DrawText(GetName(), &rect, DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    }



    // ------------------------------------------
    // ReadOnlyTextProperty
    // ------------------------------------------
    ReadOnlyTextProperty::ReadOnlyTextProperty(const CString& property_name, const TCHAR* property_description, CString value)
        :   CMFCPropertyGridProperty(property_name, COleVariant(value), property_description)
    {
        AllowEdit(FALSE);
    }

    void ReadOnlyTextProperty::OnDrawName(CDC* pDC, CRect rect)
    {
        COLORREF old_text_color = pDC->SetTextColor(GrayTextColor);
        CMFCPropertyGridProperty::OnDrawName(pDC, rect);
        pDC->SetTextColor(old_text_color);
    }

    void ReadOnlyTextProperty::OnDrawValue(CDC* pDC, CRect rect)
    {
        COLORREF old_text_color = pDC->SetTextColor(GrayTextColor);
        CMFCPropertyGridProperty::OnDrawValue(pDC, rect);
        pDC->SetTextColor(old_text_color);
    }
}
