#include "StdAfx.h"
#include "FormPropertiesDlg.h"


BEGIN_MESSAGE_MAP(CFormPropDlg, CDialog)
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_COLOR, OnColor)
    ON_BN_CLICKED(IDC_APPLY, OnApplyColor)
    ON_BN_CLICKED(IDC_RESET, OnResetColor)
    ON_BN_CLICKED(IDC_SETCAPTUREPOS_CHECK, OnSetCapturePosCheck)
    ON_BN_CLICKED(IDC_SETCAPTUREPOS_APPLY, OnSetCapturePosApply)
    ON_BN_CLICKED(IDC_SETCAPTUREPOS_DISABLE, OnSetCapturePosDisable)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CPagePropDlg dialog

CFormPropDlg::CFormPropDlg(CDEGroup* pGroup, CFormScrollView* pParent)
    :   CDialog(CFormPropDlg::IDD, pParent),
        m_sFormLabel(pGroup->GetLabel()),
        m_sFormName(pGroup->GetName()),
        m_frmcolor(FormDefaults::FormBackgoundColor.ToCOLORREF()),
        m_bSetcaptureposChecked(false),
        m_iSetcaptureposX(0),
        m_iSetcaptureposY(0),
        m_pGroup(pGroup),
        m_pMyParent(pParent)
{
}


void CFormPropDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);

    DDX_Text(pDX, IDC_FORM_NAME, m_sFormName);
    DDX_Text(pDX, IDC_FORM_LABEL, m_sFormLabel);
    DDV_MaxChars(pDX, m_sFormLabel, 100);

    DDX_Check(pDX,IDC_SETCAPTUREPOS_CHECK,m_bSetcaptureposChecked);
    DDX_Text(pDX,IDC_SETCAPTUREPOS_X,m_iSetcaptureposX);
    DDX_Text(pDX,IDC_SETCAPTUREPOS_Y,m_iSetcaptureposY);
}


BOOL CFormPropDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    OnSetCapturePosCheck(); // enable or disable the setcapturepos values

    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CFormPropDlg::OnOK()
{
    if( !ValidateSetCapturePos() )
        return;

    UpdateData(true);

    if(m_sFormLabel.IsEmpty())
    {
        AfxMessageBox(_T("Label cannot be empty"));
        GetDlgItem(IDC_FORM_LABEL)->SetFocus();
        return;
    }

    if(m_sFormName.IsEmpty())
    {
        AfxMessageBox(_T("Name cannot be empty"));
        GetDlgItem(IDC_FORM_NAME)->SetFocus();
        return;
    }

    if(!CIMSAString::IsName(m_sFormName))
    {
        AfxMessageBox(_T("Not a Valid Name"));
        return;
    }

    if(CIMSAString::IsReservedWord(m_sFormName))
    {
        AfxMessageBox(FormatText(_T("%s is a reserved word"), m_sFormName.GetString()));
        return;
    }

    if((m_pGroup->GetName().CompareNoCase(m_sFormName) !=0) && m_pMyParent)
    {
        CFormDoc* pDoc = m_pMyParent->GetDocument();

        if(!AfxGetMainWnd()->SendMessage(UWM::Form::IsNameUnique, (WPARAM)m_sFormName.GetString(), (LPARAM)pDoc))
        {
            GetDlgItem (IDC_FORM_NAME)->SetFocus();
            return;
        }
    }

    CDialog::OnOK();
}


void CFormPropDlg::OnColor()
{
    CColorDialog dlg;
    dlg.m_cc.rgbResult = m_frmcolor;
    COLORREF custcol[16];
    custcol[0] = GetSysColor(COLOR_3DFACE);
    custcol[1] = GetSysColor(COLOR_3DFACE);
    custcol[2] = GetSysColor(COLOR_3DFACE);
    custcol[3] = GetSysColor(COLOR_3DFACE);
    custcol[4] = GetSysColor(COLOR_3DFACE);
    dlg.m_cc.Flags |=  CC_RGBINIT|CC_SOLIDCOLOR;
    dlg.m_cc.lpCustColors = custcol;
    if(dlg.DoModal()==IDOK)
    {
        m_frmcolor = dlg.GetColor();
        GetDlgItem(IDC_COLOR)->Invalidate();
    }
}

void CFormPropDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if( nIDCtl == IDC_COLOR )
    {
        CDC dc;
        dc.Attach(lpDrawItemStruct->hDC);
        dc.FillSolidRect(&lpDrawItemStruct->rcItem, m_frmcolor);//COLORREF
        dc.DrawEdge(&lpDrawItemStruct->rcItem, BDR_RAISEDOUTER ,BF_RECT);
    }

    CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CFormPropDlg::OnApplyColor()
{
    if (AfxMessageBox(_T("The color will be applied to all forms.\nDo you want to continue?"),MB_YESNO|MB_ICONEXCLAMATION, 0) == IDYES)
    {
        if (m_pMyParent->GetCurForm())
        {
            PortableColor color = PortableColor::FromCOLORREF(m_frmcolor);

            for (int i = 0; i < m_pMyParent->GetFormFile()->GetNumForms(); i++)
                m_pMyParent->GetFormFile()->GetForm(i)->SetBackgroundColor(color);

            OnOK();
        }
    }
}


void CFormPropDlg::OnResetColor()
{
    m_frmcolor = FormDefaults::FormBackgoundColor.ToCOLORREF();
    UpdateData(TRUE);
    GetDlgItem(IDC_COLOR)->Invalidate();
}



// 20120405 allow the setcapturepos value to be set in the designer

void CFormPropDlg::OnSetCapturePosCheck()
{
    BOOL checked = ((CButton*)GetDlgItem(IDC_SETCAPTUREPOS_CHECK))->GetCheck();

    GetDlgItem(IDC_SETCAPTUREPOS_X)->EnableWindow(checked);
    GetDlgItem(IDC_SETCAPTUREPOS_Y)->EnableWindow(checked);
    GetDlgItem(IDC_SETCAPTUREPOS_APPLY)->EnableWindow(checked);
}


void CFormPropDlg::OnSetCapturePosApply()
{
    if( !ValidateSetCapturePos() )
        return;

    if( AfxMessageBox(_T("The custom position settings will be applied to all forms.\n")
                      _T("Do you want to continue?"), MB_YESNO | MB_ICONEXCLAMATION, 0) == IDYES )
    {
        if( m_pMyParent->GetCurForm() )
        {
            for( int i = 0; i < m_pMyParent->GetFormFile()->GetNumForms(); i++ )
                m_pMyParent->GetFormFile()->GetForm(i)->SetCapturePos(m_iSetcaptureposX,m_iSetcaptureposY);
        }
    }
}



void CFormPropDlg::OnSetCapturePosDisable()
{
    if( AfxMessageBox(_T("All custom position settings will be disabled from all forms.\n")
                      _T("Do you want to continue?"), MB_YESNO | MB_ICONEXCLAMATION, 0) == IDYES )
    {
        if( m_pMyParent->GetCurForm() )
        {
            for( int i = 0; i < m_pMyParent->GetFormFile()->GetNumForms(); i++ )
                m_pMyParent->GetFormFile()->GetForm(i)->SetCapturePos(-1,-1);
        }

        ((CButton*)GetDlgItem(IDC_SETCAPTUREPOS_CHECK))->SetCheck(FALSE);
        OnSetCapturePosCheck();
    }
}


bool CFormPropDlg::ValidateSetCapturePos()
{
    if( !m_bSetcaptureposChecked )
        return true;

    CString str;

    GetDlgItem(IDC_SETCAPTUREPOS_X)->GetWindowText(str);
    int xval = _wtoi(str);

    GetDlgItem(IDC_SETCAPTUREPOS_Y)->GetWindowText(str);
    int yval = _wtoi(str);

    if( xval < 0 || yval < 0 )
    {
        AfxMessageBox(_T("The X and Y coordinates must be non-negative."));
        return false;
    }

    // in case the number entered was an alpha or a decimal, set it to the integer it was set to by _wtoi
    GetDlgItem(IDC_SETCAPTUREPOS_X)->SetWindowText(IntToString(xval));
    GetDlgItem(IDC_SETCAPTUREPOS_Y)->SetWindowText(IntToString(yval));

    return true;
}
