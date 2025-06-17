#include "StdAfx.h"
#include "FieldColorsDlg.h"


BEGIN_MESSAGE_MAP(FieldColorsDlg, CDialog)
    ON_WM_DRAWITEM()
    ON_CONTROL_RANGE(BN_CLICKED, IDC_FIELD_COLORS_UNVISITED, IDC_FIELD_COLORS_SKIPPED, OnColor)
    ON_BN_CLICKED(IDC_FIELD_COLORS_RESET, OnReset)
END_MESSAGE_MAP()


namespace
{
    std::optional<FieldStatus> IdToFieldStatus(UINT nID)
    {
        if( nID == IDC_FIELD_COLORS_UNVISITED ) return FieldStatus::Unvisited;
        if( nID == IDC_FIELD_COLORS_VISITED )   return FieldStatus::Visited;
        if( nID == IDC_FIELD_COLORS_CURRENT )   return FieldStatus::Current;
        if( nID == IDC_FIELD_COLORS_SKIPPED )   return FieldStatus::Skipped;

        return std::nullopt;
    }
}


FieldColorsDlg::FieldColorsDlg(const CDEFormFile& form_file, CWnd* pParent /*= nullptr*/)
    :   CDialog(FieldColorsDlg::IDD, pParent),
        m_formFile(form_file),
        m_fieldColors(m_formFile.GetEvaluatedFieldColors())
{
}


void FieldColorsDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    std::optional<FieldStatus> field_status = IdToFieldStatus(nIDCtl);

    if( field_status.has_value() )
    {
        COLORREF color = m_fieldColors.GetColor(*field_status).ToCOLORREF();

        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);
        dc.FillSolidRect(&lpDrawItemStruct->rcItem, color);
        dc.DrawEdge(&lpDrawItemStruct->rcItem, BDR_RAISEDOUTER, BF_RECT);
    }

    CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}


void FieldColorsDlg::OnColor(UINT nID)
{
    std::optional<FieldStatus> field_status = IdToFieldStatus(nID);
    ASSERT(field_status.has_value());

    COLORREF color = m_fieldColors.GetColor(*field_status).ToCOLORREF();

    CColorDialog color_dlg;
    color_dlg.m_cc.rgbResult = color;
    color_dlg.m_cc.Flags |= CC_RGBINIT | CC_SOLIDCOLOR;

    if( color_dlg.DoModal() != IDOK || color_dlg.GetColor() == color )
        return;

    m_fieldColors.SetColor(*field_status, PortableColor::FromCOLORREF(color_dlg.GetColor()));

    Invalidate();
}


void FieldColorsDlg::OnReset()
{
    m_fieldColors = FieldColors().GetEvaluatedFieldColors(m_formFile);

    Invalidate();
}
