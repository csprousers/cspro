// CmpFmtD.cpp : implementation file
//

#include "StdAfx.h"
#include "CmpFmtD.h"
#include "FmtClDlg.h"
#include "FmtFontD.h"
#include "TabDoc.h"
#include "TabView.h"

// CCompFmtDlg dialog
const COLORREF BOX_INDETERMINATE = RGB(255,255,254);
IMPLEMENT_DYNAMIC(CCompFmtDlg, CDialog)
CCompFmtDlg::CCompFmtDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CCompFmtDlg::IDD, pParent)
    , m_bHide(FALSE)
    , m_bCustom(FALSE)
    , m_bExtendFont(FALSE)
    , m_bExtendTxtColor(FALSE)
    , m_bExtendFillColor(FALSE)
    , m_bExtendIndent(FALSE)
    , m_sFontDesc(_T(""))
    , m_sUnitsR(_T(""))
    , m_sUnitsL(_T(""))
    , m_fIndentLeft(0)
    , m_fIndentRight(0)
    , m_bDefaultIndentation(FALSE)
    , m_bShowDefaults(TRUE)
    , m_bSpanCells(FALSE)
    , m_bHideZero(FALSE)
{
    m_pArrTblBase = NULL;
    m_pFmt = NULL;
    m_pDefFmt = NULL;
    m_bDefFillColor = true;
    m_bDefTxtColor = true;
    m_bDisableHide = false;
    m_bDisableZeroHide = false;
    m_pTabView = NULL;
    m_iSetMultipleFonts = -1;
    m_iDecimal4Kludge =-1;
}

CCompFmtDlg::~CCompFmtDlg()
{
}

void CCompFmtDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_HIDECOMP, m_bHide);
    DDX_Check(pDX, IDC_CUSTOMTXT, m_bCustom);
    DDX_Check(pDX, IDC_EXTND_FONT, m_bExtendFont);
    DDX_Check(pDX, IDC_EXTND_LINES, m_bExtendLines);
    DDX_Check(pDX, IDC_EXTND_TXTCOLOR, m_bExtendTxtColor);
    DDX_Check(pDX, IDC_EXTND_FILLCOLOR, m_bExtendFillColor);
    DDX_Check(pDX, IDC_EXTND_INDENT, m_bExtendIndent);
    DDX_Control(pDX, IDC_BRDRL, m_BorderL);
    DDX_Control(pDX, IDC_BRDRR, m_BorderR);
    DDX_Control(pDX, IDC_BRDRT, m_BorderT);
    DDX_Control(pDX, IDC_BRDRB, m_BorderB);
    DDX_Control(pDX, IDC_DECIMALS, m_decimals);
    DDX_Control(pDX, IDC_ALGNH, m_AlignH);
    DDX_Control(pDX, IDC_ALGNV, m_AlignV);
    if (!pDX->m_bSaveAndValidate) {
        UpdateFontDescription();
    }
    DDX_Text(pDX, IDC_FONT_DESC, m_sFontDesc);
    DDX_Text(pDX, IDC_UNITSR, m_sUnitsR);
    DDX_Text(pDX, IDC_UNITSL, m_sUnitsL);
    DDX_Text(pDX, IDC_INDENTL, m_fIndentLeft);
    DDX_Text(pDX, IDC_INDENTR, m_fIndentRight);
    DDX_Check(pDX, IDC_DEFAULT_INDENTATION, m_bDefaultIndentation);
    DDX_Check(pDX, IDC_SPAN_CELLS, m_bSpanCells);
    DDX_Control(pDX, IDC_HIDEZERO, m_HideZero);
    DDX_Check(pDX, IDC_HIDEZERO, m_bHideZero);
}


BEGIN_MESSAGE_MAP(CCompFmtDlg, CDialog)
    ON_BN_CLICKED(IDC_CHNGFONT, OnBnClickedChngfont)
    ON_BN_CLICKED(IDC_BUTTON_TXTCOLR, OnBnClickedButtonTxtcolr)
    ON_BN_CLICKED(IDC_BUTTON_FILLCOLR, OnBnClickedButtonFillcolr)
    ON_BN_CLICKED(IDOK, OnBnClickedOk)
    ON_BN_CLICKED(IDC_RESET, OnBnClickedReset)
    ON_WM_DRAWITEM()
    ON_BN_CLICKED(IDC_DEFAULT_INDENTATION, OnBnClickedDefaultIndentation)
END_MESSAGE_MAP()


// CCompFmtDlg message handlers

void CCompFmtDlg::OnBnClickedChngfont()
{
    LOGFONT lfFont;
    BOOL bUseDef= FALSE;
    if(m_pFmt->IsFontCustom()){
        m_pFmt->GetFont()->GetLogFont(&lfFont);
        bUseDef=FALSE;
    }
    else {
        ASSERT(m_pDefFmt->GetFont());
        m_pDefFmt->GetFont()->GetLogFont(&lfFont);
        bUseDef = TRUE;
    }
    ASSERT((!m_bShowDefaults && !bUseDef) || m_bShowDefaults);
    CFmtFontDlg dlg(&lfFont, CF_SCREENFONTS|CF_EFFECTS /* | CF_FIXEDPITCHONLY*/);   // csc 3/31/2004 ... invokes customized CFontDialog, without "strikeout"
    dlg.m_bUseDefault = bUseDef;
    dlg.m_bShowUseDefault = m_bShowDefaults;
    m_pDefFmt->GetFont()->GetLogFont(&dlg.m_lfDef);
    if(dlg.DoModal() == IDOK){
        if(dlg.m_bUseDefault){
            m_iSetMultipleFonts = 1;
            CFont* pFont = NULL;
            m_pFmt->SetFont(pFont);
            ASSERT(m_pDefFmt->GetFont());
            LOGFONT lf;
            m_pDefFmt->GetFont()->GetLogFont(&lf);
            m_sFontDesc = PortableFont(lf).GetDescription();
            GetDlgItem(IDC_FONT_DESC)->SetWindowText(m_sFontDesc);
        }
        else {
            m_iSetMultipleFonts = 2;
            m_pFmt->SetFont(dlg.m_cf.lpLogFont);
            ASSERT(m_pFmt->GetFont());
            LOGFONT lf;
            m_pFmt->GetFont()->GetLogFont(&lf);
            m_sFontDesc = PortableFont(lf).GetDescription();
            GetDlgItem(IDC_FONT_DESC)->SetWindowText(m_sFontDesc);
        }
    }
}

void CCompFmtDlg::OnBnClickedButtonTxtcolr()
{
    CFmtColorDlg dlg;
    dlg.m_bUseDef = m_pFmt->GetTextColor().m_bUseDefault;
    dlg.m_bShowUseDefault = m_bShowDefaults;
    ASSERT((!m_bShowDefaults && !dlg.m_bUseDef) || m_bShowDefaults);
    dlg.m_colorDef = m_pDefFmt->GetTextColor().m_rgb;
    dlg.m_cc.rgbResult = m_colorText;
    dlg.m_cc.Flags |=  CC_RGBINIT|CC_SOLIDCOLOR;
    if(dlg.DoModal()==IDOK)
    {
        if(dlg.m_bUseDef && m_pDefFmt->GetTextColor().m_rgb == dlg.GetColor() ){
            FMT_COLOR fmtColor;
            fmtColor.m_bUseDefault = true;
            m_pFmt->SetTextColor(fmtColor);
            m_bDefTxtColor = true;

            m_colorText = m_pDefFmt->GetTextColor().m_rgb;
            GetDlgItem(IDC_BUTTON_TXTCOLR)->Invalidate();
        }
        else {
            m_colorText = dlg.GetColor();

            FMT_COLOR fmtColor;
            fmtColor.m_bUseDefault = false;
            m_bDefTxtColor = false;
            fmtColor.m_rgb = m_colorText;
            m_pFmt->SetTextColor(fmtColor);

            GetDlgItem(IDC_BUTTON_TXTCOLR)->Invalidate();
        }
    }
}

void CCompFmtDlg::OnBnClickedButtonFillcolr()
{
    CFmtColorDlg dlg;
    dlg.m_bUseDef = m_pFmt->GetFillColor().m_bUseDefault;
    dlg.m_bShowUseDefault = m_bShowDefaults;
    ASSERT((!m_bShowDefaults && !dlg.m_bUseDef) || m_bShowDefaults);
    dlg.m_colorDef = m_pDefFmt->GetFillColor().m_rgb;
    dlg.m_cc.rgbResult = m_colorFill;
    dlg.m_cc.Flags |=  CC_RGBINIT|CC_SOLIDCOLOR;
    if(dlg.DoModal()==IDOK)
        {
        if(dlg.m_bUseDef && m_pDefFmt->GetFillColor().m_rgb == dlg.GetColor()){
            FMT_COLOR fmtColor;
            fmtColor.m_bUseDefault = true;
            m_pFmt->SetFillColor(fmtColor);
            m_bDefFillColor = true;
            m_colorFill = m_pDefFmt->GetFillColor().m_rgb;
            GetDlgItem(IDC_BUTTON_FILLCOLR)->Invalidate();
        }
        else {
            m_colorFill = dlg.GetColor();

            FMT_COLOR fmtColor;
            fmtColor.m_bUseDefault = false;
            fmtColor.m_rgb = m_colorFill;
            m_pFmt->SetFillColor(fmtColor);
            m_bDefFillColor = false;

            GetDlgItem(IDC_BUTTON_FILLCOLR)->Invalidate();
        }
    }

}

void CCompFmtDlg::OnBnClickedOk()
{
    OnOK();
    if(!m_pArrTblBase){
        if (m_eGridComp != FMT_ID_DATACELL) {
            m_pFmt->SetLineLeft((LINE)m_BorderL.GetItemData(m_BorderL.GetCurSel()));
            m_pFmt->SetLineRight((LINE)m_BorderL.GetItemData(m_BorderR.GetCurSel()));
            m_pFmt->SetLineTop((LINE)m_BorderL.GetItemData(m_BorderT.GetCurSel()));
            m_pFmt->SetLineBottom((LINE)m_BorderL.GetItemData(m_BorderB.GetCurSel()));
        }

        m_pFmt->SetHorzAlign((TAB_HALIGN)m_AlignH.GetItemData(m_AlignH.GetCurSel()));
        m_pFmt->SetVertAlign((TAB_VALIGN)m_AlignV.GetItemData(m_AlignV.GetCurSel()));

        CUSTOM custom = m_pFmt->GetCustom();
        m_bCustom ? custom.m_bIsCustomized = true  : custom.m_bIsCustomized = false;
        if(!custom.m_bIsCustomized ){
            custom.m_sCustomText =_T("");
        }
        m_pFmt->SetCustom(custom);

        CDataCellFmt* pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pFmt);
        if(pDataCellFmt){
            pDataCellFmt->SetNumDecimals((NUM_DECIMALS)m_decimals.GetItemData(m_decimals.GetCurSel()));
        }
        switch(m_eGridComp){
        case FMT_ID_TITLE:
        case FMT_ID_SUBTITLE:
        case FMT_ID_PAGENOTE:
        case FMT_ID_ENDNOTE:
            break;
        case FMT_ID_STUBHEAD:
            m_pFmt->SetLinesExtend(m_bExtendLines?true:false);
            break;
        case FMT_ID_CAPTION:
        case FMT_ID_AREA_CAPTION:
            (m_bSpanCells) ? m_pFmt->SetSpanCells(SPAN_CELLS_YES):m_pFmt->SetSpanCells(SPAN_CELLS_NO);
            //Fall into in the case condition to set the rest of the stuff and then break
        case FMT_ID_SPANNER:
            m_pFmt->SetLinesExtend(m_bExtendLines?true:false);
            //BMD wanted caption /spanner show hide mode
            (m_bHide && !m_bDisableHide) ? m_pFmt->SetHidden(HIDDEN_YES):m_pFmt->SetHidden(HIDDEN_NO);
            break;
        case FMT_ID_STUB:
        case FMT_ID_COLHEAD:
            (m_bHide && !m_bDisableHide) ? m_pFmt->SetHidden(HIDDEN_YES):m_pFmt->SetHidden(HIDDEN_NO);
            (m_bHideZero && !m_bDisableZeroHide) ? pDataCellFmt->SetZeroHidden(true):pDataCellFmt->SetZeroHidden(false);
            m_pFmt->SetFontExtends(m_bExtendFont?true:false);
            m_pFmt->SetLinesExtend(m_bExtendLines?true:false);
            m_pFmt->SetTextColorExtends(m_bExtendTxtColor?true:false);
            m_pFmt->SetFillColorExtends(m_bExtendFillColor?true:false);
            m_pFmt->SetIndentationExtends(m_bExtendIndent?true:false);
            break;
        case FMT_ID_DATACELL:
            break;
        default:break;
        }


        //do if not using default
        FMT_COLOR fmtTxtColor;
        //for now
        //  m_bDefTxtColor =false;
        fmtTxtColor.m_bUseDefault = m_bDefTxtColor;
        if(!fmtTxtColor.m_bUseDefault) {
            fmtTxtColor.m_rgb = m_colorText;
            m_pFmt->SetTextColor(fmtTxtColor);
        }
        //do if not using default
        FMT_COLOR fmtFillColor;
        //for now
        //  m_bDefFillColor =false;
        fmtFillColor.m_bUseDefault = m_bDefFillColor;
        if(!fmtFillColor.m_bUseDefault) {
            fmtFillColor.m_rgb = m_colorFill;
            m_pFmt->SetFillColor(fmtFillColor);
        }

        if (m_bDefaultIndentation) {
            m_pFmt->SetIndent(LEFT, INDENT_DEFAULT);
            m_pFmt->SetIndent(RIGHT, INDENT_DEFAULT);
        }
        else {
            if(m_pFmt->GetUnits() == UNITS_METRIC){
                m_pFmt->SetIndent(LEFT,CmToTwips(m_fIndentLeft));
                m_pFmt->SetIndent(RIGHT,CmToTwips(m_fIndentRight));
            }
            else {
                m_pFmt->SetIndent(LEFT,InchesToTwips(m_fIndentLeft));
                m_pFmt->SetIndent(RIGHT,InchesToTwips(m_fIndentRight));
            }
        }
    }
    else {
        UpdateMultiFmtsOnOk();
    }
}

void CCompFmtDlg::OnBnClickedReset()
{
    //reset lines
    m_pFmt->SetLineTop(LINE_DEFAULT);
    m_pFmt->SetLineBottom(LINE_DEFAULT);
    m_pFmt->SetLineLeft(LINE_DEFAULT);
    m_pFmt->SetLineRight(LINE_DEFAULT);
   //reset align
    m_pFmt->SetHorzAlign(HALIGN_DEFAULT);
    m_pFmt->SetVertAlign(VALIGN_DEFAULT);
    //reset color
    FMT_COLOR fmtColor;
    fmtColor.m_bUseDefault = true;
    m_pFmt->SetTextColor(fmtColor);
    m_pFmt->SetFillColor(fmtColor);

    //reset extends
    m_pFmt->SetFontExtends(m_pDefFmt->GetFontExtends());
    m_pFmt->SetLinesExtend(m_pDefFmt->GetLinesExtend());
    m_pFmt->SetTextColorExtends(m_pDefFmt->GetTextColorExtends());
    m_pFmt->SetIndentationExtends(m_pDefFmt->GetIndentationExtends());

    //Set Custom
    m_pFmt->SetCustom(m_pDefFmt->GetCustom());
    m_pFmt->SetHidden(HIDDEN_DEFAULT);

    //SpanCells
    m_pFmt->SetSpanCells(SPAN_CELLS_DEFAULT);
    //Reset Font
    CFont* pFont = NULL;
    m_pFmt->SetFont(pFont);
//Now let the controls reflect the defaults
    m_BorderL.SetCurSel(0);
    m_BorderR.SetCurSel(0);
    m_BorderT.SetCurSel(0);
    m_BorderB.SetCurSel(0);

    m_AlignH.SetCurSel(0);
    m_AlignV.SetCurSel(0);

    CDataCellFmt* pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pFmt);
    if(pDataCellFmt){
        pDataCellFmt->SetNumDecimals(NUM_DECIMALS_DEFAULT);
        m_decimals.SetCurSel(0);
        CDataCellFmt* pDefDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pDefFmt);
        pDataCellFmt->SetZeroHidden(pDefDataCellFmt->GetZeroHidden());
        m_bHideZero = pDefDataCellFmt->GetZeroHidden();
    }
    if(m_pDefFmt->IsTextCustom() ){
        ((CButton*)GetDlgItem(IDC_CUSTOMTXT))->SetCheck(TRUE);
    }
    else {
        ((CButton*)GetDlgItem(IDC_CUSTOMTXT))->SetCheck(FALSE);
    }

    m_colorFill =  m_pDefFmt->GetFillColor().m_rgb;
    m_colorText =  m_pDefFmt->GetTextColor().m_rgb;


    LOGFONT lf;
    m_pDefFmt->GetFont()->GetLogFont(&lf);
    m_sFontDesc = PortableFont(lf).GetDescription();
    if(m_pArrTblBase){//on reset make sure the ok on fonts works
        CFont* pFont = NULL;
        m_pFmt->SetFont(pFont);
        m_iSetMultipleFonts =1;
    }

    m_bExtendFont =m_pDefFmt->GetFontExtends();
    m_bExtendLines =m_pDefFmt->GetLinesExtend();
    m_bExtendTxtColor =m_pDefFmt->GetTextColorExtends();
    m_bExtendFillColor =m_pDefFmt->GetFillColorExtends();
    m_bExtendIndent =m_pDefFmt->GetIndentationExtends();

    m_bCustom = m_pDefFmt->GetCustom().m_bIsCustomized;
    m_bSpanCells =m_pDefFmt->GetSpanCells() == SPAN_CELLS_YES;

    UpdateData(FALSE);

    GetDlgItem(IDC_BUTTON_TXTCOLR)->Invalidate();
    GetDlgItem(IDC_BUTTON_FILLCOLR)->Invalidate();
}

void CCompFmtDlg::UpdateFontDescription()
{
    if(!m_pArrTblBase){
        if(m_pFmt->IsFontCustom()){
            ASSERT(m_pFmt->GetFont());
            LOGFONT lf;
            m_pFmt->GetFont()->GetLogFont(&lf);
            m_sFontDesc = PortableFont(lf).GetDescription();
        }
        else {
            ASSERT(m_pDefFmt->GetFont());
            LOGFONT lf;
            m_pDefFmt->GetFont()->GetLogFont(&lf);
            m_sFontDesc = PortableFont(lf).GetDescription();
        }
    }
    else {
        bool bIndeterminate = false;
        for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
            CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(iIndex ==0){
                if(pFmt && pFmt->IsFontCustom()){
                    ASSERT(pFmt->GetFont());
                    LOGFONT lf;
                    pFmt->GetFont()->GetLogFont(&lf);
                    m_sFontDesc = PortableFont(lf).GetDescription();
                }
                else {
                    LOGFONT lf;
                    m_pDefFmt->GetFont()->GetLogFont(&lf);
                    m_sFontDesc = PortableFont(lf).GetDescription();
                }
                continue;
            }
            else {
                if(pFmt && pFmt->IsFontCustom()){
                    ASSERT(pFmt->GetFont());
                    LOGFONT lf;
                    pFmt->GetFont()->GetLogFont(&lf);
                    if(m_sFontDesc.CompareNoCase(PortableFont(lf).GetDescription()) != 0){
                        bIndeterminate = true;
                        break;
                    }
                }
                else {
                    LOGFONT lf;
                    ASSERT(m_pDefFmt->GetFont());
                    m_pDefFmt->GetFont()->GetLogFont(&lf);
                    if(m_sFontDesc .CompareNoCase(PortableFont(lf).GetDescription()) != 0){
                        bIndeterminate = true;
                        break;
                    }
                }
            }
        }
        if(bIndeterminate){
            m_sFontDesc = _T("Multiple Fonts.");
        }
    }
}

BOOL CCompFmtDlg::OnInitDialog()
{
    ASSERT(m_pDefFmt);
    if(!m_pArrTblBase){
        ASSERT(m_pFmt);
        m_bExtendFont =m_pFmt->GetFontExtends();
        m_bExtendLines =m_pFmt->GetLinesExtend();
        if(m_pFmt->GetSpanCells() != SPAN_CELLS_NOT_APPL){
            if(m_pFmt->GetSpanCells() ==SPAN_CELLS_YES){
                m_bSpanCells = TRUE;
            }
        }
        else {
            m_bSpanCells=FALSE;
            GetDlgItem(IDC_SPAN_CELLS)->EnableWindow(FALSE);
        }
        m_bExtendTxtColor =m_pFmt->GetTextColorExtends();
        m_bExtendFillColor =m_pFmt->GetFillColorExtends();
        m_bExtendIndent =m_pFmt->GetIndentationExtends();
        if(!m_bDisableHide){
            m_bHide = m_pFmt->GetHidden()==HIDDEN_YES;
        }
        if(!m_bDisableZeroHide){
            CDataCellFmt* pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pFmt);
            if(pDataCellFmt){
                m_bHideZero = pDataCellFmt->GetZeroHidden();
            }
        }

        if(m_pFmt->GetUnits() == UNITS_METRIC){
            m_sUnitsR = _T("cm.");
            m_sUnitsL = _T("cm.");
            m_fIndentLeft = TwipsToCm(m_pFmt->GetIndent(LEFT));
            m_fIndentRight = TwipsToCm(m_pFmt->GetIndent(RIGHT));
        }
        else {
            m_sUnitsR = _T("in.");
            m_sUnitsL = _T("in.");
            m_fIndentLeft = TwipsToInches(m_pFmt->GetIndent(LEFT));
            m_fIndentRight = TwipsToInches(m_pFmt->GetIndent(RIGHT));
        }

        // round to hundredths ...
        m_fIndentLeft=Round(m_fIndentLeft,2);
        m_fIndentRight=Round(m_fIndentRight,2);
        //    m_pFmt->SetIndent(LEFT,m_fIndentLeft);
        //    m_pFmt->SetIndent(RIGHT,m_fIndentRight);

        CDialog::OnInitDialog();
        SetWindowText(m_sTitle);
        //These are enabled only for Stub/spanner/colhead/caption
        GetDlgItem(IDC_EXTND_LINES)->EnableWindow(FALSE);
        GetDlgItem(IDC_EXTND_TXTCOLOR)->EnableWindow(FALSE);
        GetDlgItem(IDC_EXTND_FILLCOLOR)->EnableWindow(FALSE);
        GetDlgItem(IDC_EXTND_INDENT)->EnableWindow(FALSE);
        GetDlgItem(IDC_EXTND_TXTCOLOR)->EnableWindow(FALSE);
        GetDlgItem(IDC_DECIMALS)->EnableWindow(FALSE);
        GetDlgItem(IDC_EXTND_FONT)->EnableWindow(FALSE);
        GetDlgItem(IDC_HIDECOMP)->EnableWindow(FALSE);
        GetDlgItem(IDC_HIDEZERO)->EnableWindow(FALSE);
        GetDlgItem(IDC_SPAN_CELLS)->EnableWindow(FALSE);

        switch(m_eGridComp){
        case FMT_ID_STUBHEAD:
            GetDlgItem(IDC_EXTND_LINES)->EnableWindow(TRUE);
            break;
        case FMT_ID_CAPTION:
        case FMT_ID_AREA_CAPTION:
            GetDlgItem(IDC_HIDECOMP)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_LINES)->EnableWindow(TRUE);
            GetDlgItem(IDC_SPAN_CELLS)->EnableWindow(TRUE);
            break;
        case FMT_ID_SPANNER:
            //BMD wanted spanner hide
            GetDlgItem(IDC_HIDECOMP)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_LINES)->EnableWindow(TRUE);
            break;
        case FMT_ID_STUB:
            GetDlgItem(IDC_HIDEZERO)->EnableWindow(TRUE);
        case FMT_ID_COLHEAD:
            GetDlgItem(IDC_HIDECOMP)->EnableWindow(TRUE);
            GetDlgItem(IDC_CUSTOMTXT)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_LINES)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_TXTCOLOR)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_FILLCOLOR)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_INDENT)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_TXTCOLOR)->EnableWindow(TRUE);
            GetDlgItem(IDC_DECIMALS)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_FONT)->EnableWindow(TRUE);
            PrepareDecimalCtl();
            break;
        case FMT_ID_DATACELL:
            GetDlgItem(IDC_BRDRL)->EnableWindow(FALSE);
            GetDlgItem(IDC_BRDRR)->EnableWindow(FALSE);
            GetDlgItem(IDC_BRDRT)->EnableWindow(FALSE);
            GetDlgItem(IDC_BRDRB)->EnableWindow(FALSE);
            GetDlgItem(IDC_DECIMALS)->EnableWindow(TRUE);
            PrepareDecimalCtl();
            break;


        default:
            break;
        }


        PrepareLinesControls(m_pDefFmt->GetLineLeft(),m_pFmt->GetLineLeft(),&m_BorderL);
        PrepareLinesControls(m_pDefFmt->GetLineRight(),m_pFmt->GetLineRight(),&m_BorderR);
        PrepareLinesControls(m_pDefFmt->GetLineTop(),m_pFmt->GetLineTop(),&m_BorderT);
        PrepareLinesControls(m_pDefFmt->GetLineBottom(),m_pFmt->GetLineBottom(),&m_BorderB);
        if(m_pFmt->IsTextCustom()){
            m_bCustom = TRUE;
        }
        PrepareAlignControls(m_pDefFmt->GetHorzAlign(),m_pFmt->GetHorzAlign(),&m_AlignH);
        PrepareAlignControls(m_pDefFmt->GetVertAlign(),m_pFmt->GetVertAlign(),&m_AlignV);


        m_bDefTxtColor = m_pFmt->GetTextColor().m_bUseDefault;
        if(m_bDefTxtColor){
            m_colorText =  m_pDefFmt->GetTextColor().m_rgb;
        }
        else {
            m_colorText =  m_pFmt->GetTextColor().m_rgb;
        }
        m_bDefFillColor = m_pFmt->GetFillColor().m_bUseDefault;

        if(m_bDefFillColor){
            m_colorFill =  m_pDefFmt->GetFillColor().m_rgb;
        }
        else {
            m_colorFill =  m_pFmt->GetFillColor().m_rgb;
        }


        // TODO:  Add extra initialization here

        UpdateFontDescription();

        // grey out indentation if defaults are being used ...
        if ((m_pFmt->GetIndent(LEFT)==INDENT_DEFAULT || m_pFmt->GetIndent(RIGHT)==INDENT_DEFAULT) && m_pFmt->GetIndent(LEFT)!=m_pFmt->GetIndent(RIGHT)) {
            AfxMessageBox(_T("Error: one indentation is default, but both are not; setting both to default"));
            m_pFmt->SetIndent(LEFT, INDENT_DEFAULT);
            m_pFmt->SetIndent(RIGHT, INDENT_DEFAULT);
        }

        //    m_bDefaultIndentation=(m_pFmt->GetIndent(LEFT)==INDENT_DEFAULT);
        GetDlgItem(IDC_STATIC_LEFT)->EnableWindow(!m_bDefaultIndentation);
        GetDlgItem(IDC_STATIC_RIGHT)->EnableWindow(!m_bDefaultIndentation);
        GetDlgItem(IDC_UNITSL)->EnableWindow(!m_bDefaultIndentation);
        GetDlgItem(IDC_UNITSR)->EnableWindow(!m_bDefaultIndentation);
        GetDlgItem(IDC_INDENTL)->EnableWindow(!m_bDefaultIndentation);
        GetDlgItem(IDC_INDENTR)->EnableWindow(!m_bDefaultIndentation);

        if(m_bDisableHide){
            GetDlgItem(IDC_HIDECOMP)->EnableWindow(FALSE);
        }
        if(m_bDisableZeroHide){
            GetDlgItem(IDC_HIDEZERO)->EnableWindow(FALSE);
        }
        if (!m_bShowDefaults) {
            GetDlgItem(IDC_DEFAULT_INDENTATION)->ShowWindow(SW_HIDE);
        }
        Switch3WayOff();
    }
    else {
        InitMultiStuff();
    }
    UpdateData(FALSE);
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::PrepareLinesControls(LINE eLineDef, , LINE eLine ,CComboBox* pComboBox)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::PrepareLinesControls(LINE eLineDef,LINE eLine ,CComboBox* pComboBox)
 {
     if (m_bShowDefaults) {
        switch(eLineDef){
            case LINE_DEFAULT:
                ASSERT(FALSE);
                break;
            case LINE_NONE:
                pComboBox->AddString(_T("Default (None)"));
                pComboBox->SetItemData(0,LINE_DEFAULT);
                break;
            case LINE_THIN:
                pComboBox->AddString(_T("Default (Thin)"));
                pComboBox->SetItemData(0,LINE_DEFAULT);
                break;
            case LINE_THICK:
                pComboBox->AddString(_T("Default (Thick)"));
                pComboBox->SetItemData(0,LINE_DEFAULT);
                break;
            case LINE_NOT_APPL:
                pComboBox->AddString(_T("Default (Not Applicable)"));//used by data cells
                pComboBox->SetItemData(0,LINE_DEFAULT);
                break;
            default:
                ASSERT(FALSE);
                break;
        }
     }
     pComboBox->SetItemData(pComboBox->AddString(_T("None")),LINE_NONE);
     pComboBox->SetItemData(pComboBox->AddString(_T("Thin")),LINE_THIN);
     pComboBox->SetItemData(pComboBox->AddString(_T("Thick")),LINE_THICK);

    switch(eLine){
        case LINE_DEFAULT:
            ASSERT(m_bShowDefaults);
            pComboBox->SetCurSel(0);
            break;
        case LINE_NONE:
            pComboBox->SetCurSel(m_bShowDefaults ? 1 : 0);
            break;
        case LINE_THIN:
            pComboBox->SetCurSel(m_bShowDefaults ? 2 : 1);
            break;
        case LINE_THICK:
            pComboBox->SetCurSel(m_bShowDefaults ? 3 : 2);
            break;
        case LINE_NOT_APPL:
            pComboBox->SetCurSel(m_bShowDefaults ? 4 : 3);
            break;
        default:
            ASSERT(FALSE);
            break;
    }

 }

void CCompFmtDlg::PrepareAlignControls(TAB_HALIGN eHAlignDef, TAB_HALIGN eHAlign , CComboBox* pComboBox)
{
    if (m_bShowDefaults) {
        switch(eHAlignDef){
            case HALIGN_DEFAULT:
                ASSERT(FALSE);
                break;
            case HALIGN_LEFT:
                pComboBox->AddString(_T("Default (Left)"));
                pComboBox->SetItemData(0,HALIGN_DEFAULT);
                break;
            case HALIGN_CENTER:
                pComboBox->AddString(_T("Default (Center)"));
                pComboBox->SetItemData(0,HALIGN_DEFAULT);
                break;
            case HALIGN_RIGHT:
                pComboBox->AddString(_T("Default (Right)"));
                pComboBox->SetItemData(0,HALIGN_DEFAULT);
                break;
            default:
                ASSERT(FALSE);
                break;
        }
    }
    pComboBox->SetItemData(pComboBox->AddString(_T("Left")),HALIGN_LEFT);
    pComboBox->SetItemData(pComboBox->AddString(_T("Center")),HALIGN_CENTER);
    pComboBox->SetItemData(pComboBox->AddString(_T("Right")),HALIGN_RIGHT);

    switch(eHAlign){
        case HALIGN_DEFAULT:
            ASSERT(m_bShowDefaults);
            pComboBox->SetCurSel(0);
            break;
        case HALIGN_LEFT:
            pComboBox->SetCurSel(m_bShowDefaults ? 1 : 0);
            break;
        case HALIGN_CENTER:
            pComboBox->SetCurSel(m_bShowDefaults ? 2 : 1);
            break;
        case HALIGN_RIGHT:
            pComboBox->SetCurSel(m_bShowDefaults ? 3 : 2);
            break;
        default:
            ASSERT(FALSE);
            break;
    }

}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::PrepareAlignControls(TAB_VALIGN eVAlignDef, TAB_VALIGN eVAlign , CComboBox* pComboBox);
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::PrepareAlignControls(TAB_VALIGN eVAlignDef, TAB_VALIGN eVAlign , CComboBox* pComboBox)
{
    if (m_bShowDefaults) {
        switch(eVAlignDef){
            case VALIGN_DEFAULT:
                ASSERT(FALSE);
                break;
            case VALIGN_TOP:
                pComboBox->AddString(_T("Default (Top)"));
                pComboBox->SetItemData(0,VALIGN_DEFAULT);
                break;
            case VALIGN_MID:
                pComboBox->AddString(_T("Default (Middle)"));
                pComboBox->SetItemData(0,VALIGN_DEFAULT);
                break;
            case VALIGN_BOTTOM:
                pComboBox->AddString(_T("Default (Bottom)"));
                pComboBox->SetItemData(0,VALIGN_DEFAULT);
                break;
            default:
                ASSERT(FALSE);
                break;
        }
    }
    pComboBox->SetItemData(pComboBox->AddString(_T("Top")),VALIGN_TOP);
    pComboBox->SetItemData(pComboBox->AddString(_T("Middle")),VALIGN_MID);
    pComboBox->SetItemData(pComboBox->AddString(_T("Bottom")),VALIGN_BOTTOM);

    switch(eVAlign){
        case VALIGN_DEFAULT:
            ASSERT(m_bShowDefaults);
            pComboBox->SetCurSel(0);
            break;
        case VALIGN_TOP:
            pComboBox->SetCurSel(m_bShowDefaults ? 1 : 0);
            break;
        case VALIGN_MID:
            pComboBox->SetCurSel(m_bShowDefaults ? 2 : 1);
            break;
        case VALIGN_BOTTOM:
            pComboBox->SetCurSel(m_bShowDefaults ? 3 : 2);
            break;
        default:
            ASSERT(FALSE);
            break;
    }

}

void CCompFmtDlg::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    if  (nIDCtl == IDC_BUTTON_TXTCOLR)
    {
        CDC dc;
        BOOL ret = dc.Attach(lpDrawItemStruct->hDC);
        dc.FillSolidRect( &lpDrawItemStruct->rcItem,m_colorText);//COLORREF
        dc.DrawEdge(&lpDrawItemStruct->rcItem, BDR_RAISEDOUTER ,BF_RECT);
    }
    else  if (nIDCtl == IDC_BUTTON_FILLCOLR) {
        CDC dc;
        BOOL ret = dc.Attach(lpDrawItemStruct->hDC);
        dc.FillSolidRect( &lpDrawItemStruct->rcItem,m_colorFill);//COLORREF
        dc.DrawEdge(&lpDrawItemStruct->rcItem, BDR_RAISEDOUTER ,BF_RECT);
    }
    CDialog::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

void CCompFmtDlg::PrepareDecimalCtl()
{
    CDataCellFmt* pDefDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pDefFmt);
    CDataCellFmt* pDataCellFmt =  NULL;

    ASSERT(pDefDataCellFmt);
    CIMSAString sDecimal;
    switch(pDefDataCellFmt->GetNumDecimals()){
        case NUM_DECIMALS_DEFAULT:
            ASSERT(FALSE);
            break;
        case NUM_DECIMALS_ZERO:
            sDecimal =_T(" (0)");
            break;
        case NUM_DECIMALS_ONE:
            sDecimal =_T(" (1)");
            break;
        case NUM_DECIMALS_TWO:
            sDecimal =_T(" (2)");
            break;
        case NUM_DECIMALS_THREE:
            sDecimal =_T(" (3)");
            break;
        case NUM_DECIMALS_FOUR:
            sDecimal =_T(" (4)");
            break;
        case NUM_DECIMALS_FIVE:
            sDecimal =_T(" (5)");
            break;
        default:
            break;
    }
    if(m_iDecimal4Kludge != -1){
        sDecimal.Str(m_iDecimal4Kludge);
        sDecimal = _T(" (") + sDecimal +_T(")");
    }
    CIMSAString sDef = _T("Default ")+ sDecimal;
    if (m_bShowDefaults) {
        m_decimals.SetItemData(m_decimals.AddString(sDef),NUM_DECIMALS_DEFAULT);
    }
    m_decimals.SetItemData(m_decimals.AddString(_T("0")),NUM_DECIMALS_ZERO);
    m_decimals.SetItemData(m_decimals.AddString(_T("1")),NUM_DECIMALS_ONE);
    m_decimals.SetItemData(m_decimals.AddString(_T("2")),NUM_DECIMALS_TWO);
    m_decimals.SetItemData(m_decimals.AddString(_T("3")),NUM_DECIMALS_THREE);
    m_decimals.SetItemData(m_decimals.AddString(_T("4")),NUM_DECIMALS_FOUR);
    m_decimals.SetItemData(m_decimals.AddString(_T("5")),NUM_DECIMALS_FIVE);

    NUM_DECIMALS eNumDecimals = NUM_DECIMALS_DEFAULT;
    if(m_pArrTblBase && m_pArrTblBase->GetSize() > 0){
        bool bIndeterminate = false;
        for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
            CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
            CDataCellFmt* pFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pTblBase->GetFmt());
            if(iIndex ==0){
                pFmt ? eNumDecimals=pFmt->GetNumDecimals() :eNumDecimals=NUM_DECIMALS_DEFAULT;
                continue;
            }
            else {
                if(pFmt){
                    bIndeterminate = eNumDecimals !=pFmt->GetNumDecimals();
                }
                else {
                    bIndeterminate = eNumDecimals !=NUM_DECIMALS_DEFAULT;
                }
                if(bIndeterminate){
                    break;
                }
            }
        }
        if(bIndeterminate){
            m_decimals.SetCurSel(-1);
            return;
        }
    }
    else {
        pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pFmt);
        ASSERT(pDataCellFmt);
        eNumDecimals = pDataCellFmt->GetNumDecimals();
    }
    switch(eNumDecimals){
        case NUM_DECIMALS_DEFAULT:
            ASSERT(m_bShowDefaults);
            m_decimals.SetCurSel(0);
            break;
        case NUM_DECIMALS_ZERO:
            m_decimals.SetCurSel(m_bShowDefaults ? 1 : 0);
            break;
        case NUM_DECIMALS_ONE:
            m_decimals.SetCurSel(m_bShowDefaults ? 2 : 1);
            break;
        case NUM_DECIMALS_TWO:
            m_decimals.SetCurSel(m_bShowDefaults ? 3 : 2);
            break;
        case NUM_DECIMALS_THREE:
            m_decimals.SetCurSel(m_bShowDefaults ? 4 : 3);
            break;
        case NUM_DECIMALS_FOUR:
            m_decimals.SetCurSel(m_bShowDefaults ? 5 : 4);
            break;
        case NUM_DECIMALS_FIVE:
            m_decimals.SetCurSel(m_bShowDefaults ? 6 : 5);
            break;
        default:
            break;
    }
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::OnBnClickedDefaultIndentation()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::OnBnClickedDefaultIndentation()
{
    UpdateData();
    GetDlgItem(IDC_STATIC_LEFT)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_STATIC_RIGHT)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_UNITSL)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_UNITSR)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_INDENTL)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_INDENTR)->EnableWindow(!m_bDefaultIndentation);
}


/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::InitMultiStuff()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::InitMultiStuff()
{
    ASSERT(m_pArrTblBase);
    ASSERT(m_pDefFmt);
    //Extend font
    m_bExtendFont =m_pDefFmt->GetFontExtends();
    bool bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? m_bExtendFont=pFmt->GetFontExtends() :m_bExtendFont=m_pDefFmt->GetFontExtends();
            continue;
        }
        else {
            pFmt ? bIndeterminate = m_bExtendFont !=pFmt->GetFontExtends() : bIndeterminate = m_bExtendFont!=m_pDefFmt->GetFontExtends();
            if(bIndeterminate){
                m_bExtendFont = 2;
                break;
            }
        }
    }
    //Text Color
    m_bExtendTxtColor =m_pDefFmt->GetTextColorExtends();
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? m_bExtendTxtColor=pFmt->GetTextColorExtends() :m_bExtendTxtColor=m_pDefFmt->GetTextColorExtends();
            continue;
        }
        else {
            pFmt ? bIndeterminate = m_bExtendTxtColor !=pFmt->GetTextColorExtends() : bIndeterminate = m_bExtendTxtColor !=m_pDefFmt->GetTextColorExtends();
            if(bIndeterminate){
                m_bExtendTxtColor = 2;
                break;
            }
        }
    }
    //Extend Fill Color
    m_bExtendFillColor =m_pDefFmt->GetFillColorExtends();
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? m_bExtendFillColor=pFmt->GetFillColorExtends() :m_bExtendFillColor=m_pDefFmt->GetFillColorExtends();
            continue;
        }
        else {
            pFmt ? bIndeterminate = m_bExtendFillColor !=pFmt->GetFillColorExtends() : bIndeterminate = m_bExtendFillColor !=m_pDefFmt->GetFillColorExtends();
            if(bIndeterminate){
                m_bExtendFillColor = 2;
                break;
            }
        }
    }
    //Extend Indentation
    m_bExtendIndent =m_pDefFmt->GetIndentationExtends();
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? m_bExtendIndent=pFmt->GetIndentationExtends() :m_bExtendIndent=m_pDefFmt->GetIndentationExtends();
            continue;
        }
        else {
            pFmt ? bIndeterminate = m_bExtendIndent !=pFmt->GetIndentationExtends() : bIndeterminate = m_bExtendIndent !=m_pDefFmt->GetIndentationExtends();
            if(bIndeterminate){
                m_bExtendIndent = 2;
                break;
            }
        }
    }
    //Hide stuff
    if(!m_bDisableHide){
        m_bHide = m_pDefFmt->GetHidden()==HIDDEN_YES;
        bIndeterminate = false;
        for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
            CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(iIndex ==0){
                pFmt ? m_bHide = (pFmt->GetHidden()==HIDDEN_YES):m_bHide = (m_pDefFmt->GetHidden()==HIDDEN_YES);
                continue;
            }
            else {
                pFmt ? bIndeterminate = m_bHide !=(pFmt->GetHidden()==HIDDEN_YES): bIndeterminate = m_bHide !=(m_pDefFmt->GetHidden()==HIDDEN_YES);
                if(bIndeterminate){
                    m_bHide = 2;
                    break;
                }
            }
        }

    }
    //Hide stuff
    if(!m_bDisableZeroHide){
        CDataCellFmt* pDefFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pDefFmt);
        if(!pDefFmt){
            m_bDisableZeroHide = true;
        }
        else {
            m_bHideZero = pDefFmt->GetZeroHidden();
            bIndeterminate = false;
            for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
                CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
                CDataCellFmt* pFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pTblBase->GetFmt());
                if(iIndex ==0 && pFmt){
                    pFmt ? m_bHideZero= pFmt->GetZeroHidden():m_bHideZero= pDefFmt->GetZeroHidden();
                    continue;
                }
                else {
                    pFmt ? bIndeterminate = (m_bHideZero != pFmt->GetZeroHidden()): bIndeterminate = (m_bHideZero != pDefFmt->GetZeroHidden());
                    if(bIndeterminate){
                        m_bHideZero = 2;
                        break;
                    }
                }
            }
        }

    }
    //Extend lines
    m_bExtendLines =m_pDefFmt->GetLinesExtend();
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? m_bExtendFillColor=pFmt->GetLinesExtend() :m_bExtendLines=m_pDefFmt->GetLinesExtend();
            continue;
        }
        else {
            pFmt ? bIndeterminate = m_bExtendLines !=pFmt->GetLinesExtend() : bIndeterminate = m_bExtendLines !=m_pDefFmt->GetLinesExtend();
            if(bIndeterminate){
                m_bExtendLines = 2;
                break;
            }
        }
    }
    //Span cells
    if(m_pDefFmt->GetSpanCells() != SPAN_CELLS_NOT_APPL){
        if(m_pFmt->GetSpanCells() ==SPAN_CELLS_YES){
            m_bSpanCells = TRUE;
        }
        m_bSpanCells = (m_pDefFmt->GetSpanCells() ==SPAN_CELLS_YES);
        bIndeterminate = false;
        for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
            CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(iIndex ==0){
                pFmt ? m_bSpanCells = (pFmt->GetSpanCells() ==SPAN_CELLS_YES) :m_bSpanCells = (m_pDefFmt->GetSpanCells() ==SPAN_CELLS_YES);
                continue;
            }
            else {
                pFmt ? bIndeterminate = m_bSpanCells != (pFmt->GetSpanCells() ==SPAN_CELLS_YES) : bIndeterminate = m_bSpanCells !=(m_pDefFmt->GetSpanCells() ==SPAN_CELLS_YES);
                if(bIndeterminate){
                    m_bSpanCells = 2;
                    break;
                }
            }
        }
    }
    else {
        m_bSpanCells=FALSE;
        GetDlgItem(IDC_SPAN_CELLS)->EnableWindow(FALSE);
    }



    //For multistuff  indentation is check is done here
    m_bDefaultIndentation = TRUE;
    if(m_pFmt->GetUnits() == UNITS_METRIC){
        m_sUnitsR = _T("cm.");
        m_sUnitsL = _T("cm.");
        //Set indent
        bIndeterminate = false;
        m_fIndentLeft = TwipsToCm(m_pDefFmt->GetIndent(LEFT));
        m_fIndentRight = TwipsToCm(m_pDefFmt->GetIndent(RIGHT));
        for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
            CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(iIndex ==0){
                if(pFmt){
                    m_fIndentLeft = TwipsToCm(pFmt->GetIndent(LEFT));
                    m_fIndentRight = TwipsToCm(pFmt->GetIndent(RIGHT));
                }
                continue;
            }
            else {
                if(pFmt){
                    if(m_fIndentLeft != TwipsToCm(pFmt->GetIndent(LEFT))){
                        bIndeterminate = true;
                        break;
                    }
                    if(m_fIndentRight != TwipsToCm(pFmt->GetIndent(RIGHT))){
                        bIndeterminate = true;
                        break;
                    }
                }
            }
        }
        if(bIndeterminate){
            m_fIndentLeft=0;
            m_fIndentRight=0;
            m_bDefaultIndentation = 2;
            //Set the window text to blank
        }
        else {
            if(m_fIndentLeft != m_pDefFmt->GetIndent(LEFT) || m_fIndentRight != m_pDefFmt->GetIndent(RIGHT)){
                m_bDefaultIndentation = FALSE;
            }
            else {
                m_bDefaultIndentation = TRUE;
            }
        }

    }
    else {
        m_sUnitsR = _T("in.");
        m_sUnitsL = _T("in.");
        bIndeterminate = false;
        m_fIndentLeft = TwipsToInches(m_pDefFmt->GetIndent(LEFT));
        m_fIndentRight = TwipsToInches(m_pDefFmt->GetIndent(RIGHT));
        for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
            CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(iIndex ==0){
                if(pFmt){
                    m_fIndentLeft = TwipsToInches(pFmt->GetIndent(LEFT));
                    m_fIndentRight = TwipsToInches(pFmt->GetIndent(RIGHT));
                }
                continue;
            }
            else {
                if(pFmt){
                    if(m_fIndentLeft != TwipsToInches(pFmt->GetIndent(LEFT))){
                        bIndeterminate = true;
                        break;
                    }
                    if(m_fIndentRight != TwipsToInches(pFmt->GetIndent(RIGHT))){
                        bIndeterminate = true;
                        break;
                    }
                }
            }
        }
        if(bIndeterminate){
            m_fIndentLeft=0;
            m_fIndentRight=0;
            m_bDefaultIndentation = 2;
            //Set the window text to blank
        }
        else {
            if(m_fIndentLeft != m_pDefFmt->GetIndent(LEFT) || m_fIndentRight != m_pDefFmt->GetIndent(RIGHT)){
                m_bDefaultIndentation = FALSE;
            }
            else {
                m_bDefaultIndentation = TRUE;
            }
        }
    }

    // round to hundredths ...
    m_fIndentLeft=Round(m_fIndentLeft,2);
    m_fIndentRight=Round(m_fIndentRight,2);
    //    m_pFmt->SetIndent(LEFT,m_fIndentLeft);
    //    m_pFmt->SetIndent(RIGHT,m_fIndentRight);

    CDialog::OnInitDialog();
    SetWindowText(m_sTitle);
    //These are enabled only for Stub/spanner/colhead/caption
    GetDlgItem(IDC_EXTND_LINES)->EnableWindow(FALSE);
    GetDlgItem(IDC_EXTND_TXTCOLOR)->EnableWindow(FALSE);
    GetDlgItem(IDC_EXTND_FILLCOLOR)->EnableWindow(FALSE);
    GetDlgItem(IDC_EXTND_INDENT)->EnableWindow(FALSE);
    GetDlgItem(IDC_EXTND_TXTCOLOR)->EnableWindow(FALSE);
    GetDlgItem(IDC_DECIMALS)->EnableWindow(FALSE);
    GetDlgItem(IDC_EXTND_FONT)->EnableWindow(FALSE);
    GetDlgItem(IDC_HIDECOMP)->EnableWindow(FALSE);
    GetDlgItem(IDC_HIDEZERO)->EnableWindow(FALSE);
    GetDlgItem(IDC_SPAN_CELLS)->EnableWindow(FALSE);

    switch(m_eGridComp){
        case FMT_ID_STUBHEAD:
            GetDlgItem(IDC_EXTND_LINES)->EnableWindow(TRUE);
            break;
        case FMT_ID_CAPTION:
        case FMT_ID_AREA_CAPTION:
            GetDlgItem(IDC_HIDECOMP)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_LINES)->EnableWindow(TRUE);
            GetDlgItem(IDC_SPAN_CELLS)->EnableWindow(TRUE);
            break;
        case FMT_ID_SPANNER:
            //BMD wanted spanner hide
            GetDlgItem(IDC_HIDECOMP)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_LINES)->EnableWindow(TRUE);
            break;
        case FMT_ID_STUB:
            GetDlgItem(IDC_HIDEZERO)->EnableWindow(TRUE);
        case FMT_ID_COLHEAD:
            GetDlgItem(IDC_HIDECOMP)->EnableWindow(TRUE);
            GetDlgItem(IDC_CUSTOMTXT)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_LINES)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_TXTCOLOR)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_FILLCOLOR)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_INDENT)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_TXTCOLOR)->EnableWindow(TRUE);
            GetDlgItem(IDC_DECIMALS)->EnableWindow(TRUE);
            GetDlgItem(IDC_EXTND_FONT)->EnableWindow(TRUE);
            PrepareDecimalCtl();
            break;
        case FMT_ID_DATACELL:
            GetDlgItem(IDC_BRDRL)->EnableWindow(FALSE);
            GetDlgItem(IDC_BRDRR)->EnableWindow(FALSE);
            GetDlgItem(IDC_BRDRT)->EnableWindow(FALSE);
            GetDlgItem(IDC_BRDRB)->EnableWindow(FALSE);
            GetDlgItem(IDC_DECIMALS)->EnableWindow(TRUE);
            PrepareDecimalCtl();
            break;


        default:
            break;
    }
    //Custom text
    m_bCustom =m_pDefFmt->IsTextCustom();
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? m_bCustom=pFmt->IsTextCustom() :m_bCustom=m_pDefFmt->IsTextCustom();
            continue;
        }
        else {
            pFmt ? bIndeterminate = m_bCustom !=pFmt->IsTextCustom() : bIndeterminate = m_bCustom !=m_pDefFmt->IsTextCustom();
            if(bIndeterminate){
                m_bCustom =2;
                break;
            }
        }
    }

    //Line left
    LINE eLine = LINE_DEFAULT;
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? eLine=pFmt->GetLineLeft() :eLine=LINE_DEFAULT;
            continue;
        }
        else {
            if(pFmt){
                bIndeterminate = eLine !=pFmt->GetLineLeft();
            }
            else {
                bIndeterminate = eLine !=LINE_DEFAULT;
            }
            if(bIndeterminate){
                break;
            }
        }
    }
    PrepareLinesControls(m_pDefFmt->GetLineLeft(),eLine,&m_BorderL);
    bIndeterminate? m_BorderL.SetCurSel(-1) : m_BorderL.SetCurSel(m_BorderL.GetCurSel());

    //Line right
    eLine = LINE_DEFAULT;
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? eLine=pFmt->GetLineRight() :eLine=LINE_DEFAULT;
            continue;
        }
        else {
            if(pFmt){
                bIndeterminate = eLine !=pFmt->GetLineRight();
            }
            else {
                bIndeterminate = eLine !=LINE_DEFAULT;
            }
            if(bIndeterminate){
                break;
            }
        }
    }
    PrepareLinesControls(m_pDefFmt->GetLineRight(),eLine,&m_BorderR);
    bIndeterminate? m_BorderR.SetCurSel(-1) : m_BorderR.SetCurSel(m_BorderR.GetCurSel());

     //Line Top
    eLine = LINE_DEFAULT;
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? eLine=pFmt->GetLineTop() :eLine=LINE_DEFAULT;
            continue;
        }
        else {
            if(pFmt){
                bIndeterminate = eLine !=pFmt->GetLineTop();
            }
            else {
                bIndeterminate = eLine !=LINE_DEFAULT;
            }
            if(bIndeterminate){
                break;
            }
        }
    }
    PrepareLinesControls(m_pDefFmt->GetLineTop(),eLine,&m_BorderT);
    bIndeterminate? m_BorderT.SetCurSel(-1) : m_BorderT.SetCurSel(m_BorderT.GetCurSel());

    //Line Bottom
    eLine = LINE_DEFAULT;
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? eLine=pFmt->GetLineBottom() :eLine=LINE_DEFAULT;
            continue;
        }
        else {
            if(pFmt){
                bIndeterminate = eLine !=pFmt->GetLineBottom();
            }
            else {
                bIndeterminate = eLine !=LINE_DEFAULT;
            }
            if(bIndeterminate){
                break;
            }
        }
    }
    PrepareLinesControls(m_pDefFmt->GetLineBottom(),eLine,&m_BorderB);
    bIndeterminate? m_BorderB.SetCurSel(-1) : m_BorderB.SetCurSel(m_BorderB.GetCurSel());


    //Horizontal Alignment
    TAB_HALIGN eHAlign = HALIGN_DEFAULT;
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? eHAlign=pFmt->GetHorzAlign() :eHAlign=HALIGN_DEFAULT;
            continue;
        }
       else {
            if(pFmt){
               bIndeterminate = eHAlign !=pFmt->GetHorzAlign();
            }
            else {
                bIndeterminate = eHAlign !=HALIGN_DEFAULT;
            }
            if(bIndeterminate){
                break;
            }
        }
    }
    PrepareAlignControls(m_pDefFmt->GetHorzAlign(),eHAlign,&m_AlignH);
    bIndeterminate? m_AlignH.SetCurSel(-1) : m_AlignH.SetCurSel(m_AlignH.GetCurSel());
    //Vertical Align
    TAB_VALIGN eVAlign = VALIGN_DEFAULT;
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? eVAlign=pFmt->GetVertAlign() :eVAlign=VALIGN_DEFAULT;
            continue;
        }
       else {
            if(pFmt){
                bIndeterminate = eVAlign !=pFmt->GetVertAlign() ;
            }
            else {
                bIndeterminate = eVAlign !=VALIGN_DEFAULT;
            }
            if(bIndeterminate){
                break;
            }
        }
    }
    PrepareAlignControls(m_pDefFmt->GetVertAlign(),eVAlign,&m_AlignV);
    bIndeterminate? m_AlignV.SetCurSel(-1) : m_AlignV.SetCurSel(m_AlignV.GetCurSel());


    //Text Color
    m_bDefTxtColor = true;
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? m_bDefTxtColor=pFmt->GetTextColor().m_bUseDefault :m_bDefTxtColor=true;
            continue;
        }
        else {
            pFmt ? bIndeterminate = m_bDefTxtColor !=pFmt->GetTextColor().m_bUseDefault : bIndeterminate = m_bDefTxtColor !=true;
            if(bIndeterminate){
                m_bDefTxtColor = false;
                break;
            }
        }
    }
    if(bIndeterminate){
        m_colorText = BOX_INDETERMINATE;
    }
    else if(m_bDefTxtColor){
        m_colorText =  m_pDefFmt->GetTextColor().m_rgb;
    }
    else {
        m_colorText =  m_pDefFmt->GetTextColor().m_rgb;
        bIndeterminate = false;
        for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
            CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(iIndex ==0){
                pFmt ? m_colorText=pFmt->GetTextColor().m_rgb :m_colorText=m_pDefFmt->GetTextColor().m_rgb;
                continue;
            }
            else {
                pFmt ? bIndeterminate = m_colorText !=pFmt->GetTextColor().m_rgb : bIndeterminate = m_colorText !=m_pDefFmt->GetTextColor().m_rgb;
                if(bIndeterminate){
                    m_colorText = BOX_INDETERMINATE;
                    break;
                }
            }
        }
    }
    //Fill Color
    m_bDefFillColor =true;
    bIndeterminate = false;
    for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
        CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(iIndex ==0){
            pFmt ? m_bDefFillColor=pFmt->GetFillColor().m_bUseDefault :m_bDefFillColor=m_pDefFmt->GetFillColor().m_bUseDefault;
            continue;
        }
        else {
            pFmt ? bIndeterminate = m_bDefFillColor !=pFmt->GetFillColor().m_bUseDefault : bIndeterminate = m_bDefFillColor !=m_pDefFmt->GetFillColor().m_bUseDefault;
            if(bIndeterminate){
                m_bDefFillColor = false;
                break;
            }
        }
    }
    if(bIndeterminate){
        m_colorFill = BOX_INDETERMINATE;
    }
    else if(m_bDefFillColor){
        m_colorFill =  m_pDefFmt->GetFillColor().m_rgb;
    }
    else {
        m_colorFill =  m_pDefFmt->GetFillColor().m_rgb;
        bIndeterminate = false;
        for(int iIndex =0 ;iIndex <  m_pArrTblBase->GetSize(); iIndex++){
            CTblBase* pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(iIndex ==0){
                pFmt ? m_colorFill=pFmt->GetFillColor().m_rgb :m_colorFill=m_pDefFmt->GetFillColor().m_rgb;
                continue;
            }
            else {
                pFmt ? bIndeterminate = m_colorFill !=pFmt->GetFillColor().m_rgb : bIndeterminate = m_colorFill !=m_pDefFmt->GetFillColor().m_rgb;
                if(bIndeterminate){
                    m_colorFill = BOX_INDETERMINATE;
                    break;
                }
            }
        }
    }

    // TODO:  Add extra initialization here

    UpdateFontDescription();

    // grey out indentation if defaults are being used ...
    if ((m_pFmt->GetIndent(LEFT)==INDENT_DEFAULT || m_pFmt->GetIndent(RIGHT)==INDENT_DEFAULT) && m_pFmt->GetIndent(LEFT)!=m_pFmt->GetIndent(RIGHT)) {
        AfxMessageBox(_T("Error: one indentation is default, but both are not; setting both to default"));
        m_pFmt->SetIndent(LEFT, INDENT_DEFAULT);
        m_pFmt->SetIndent(RIGHT, INDENT_DEFAULT);
    }

    //    m_bDefaultIndentation=(m_pFmt->GetIndent(LEFT)==INDENT_DEFAULT);
    GetDlgItem(IDC_STATIC_LEFT)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_STATIC_RIGHT)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_UNITSL)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_UNITSR)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_INDENTL)->EnableWindow(!m_bDefaultIndentation);
    GetDlgItem(IDC_INDENTR)->EnableWindow(!m_bDefaultIndentation);

    if(m_bDisableHide){
        GetDlgItem(IDC_HIDECOMP)->EnableWindow(FALSE);
    }
    if(m_bDisableZeroHide){
        GetDlgItem(IDC_HIDEZERO)->EnableWindow(FALSE);
    }
    if (!m_bShowDefaults) {
        GetDlgItem(IDC_DEFAULT_INDENTATION)->ShowWindow(SW_HIDE);
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::Switch3WayOff()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::Switch3WayOff()
{
    ((CButton*)GetDlgItem(IDC_HIDECOMP))->SetButtonStyle(BS_AUTOCHECKBOX);
    ((CButton*)GetDlgItem(IDC_CUSTOMTXT))->SetButtonStyle(BS_AUTOCHECKBOX);
    ((CButton*)GetDlgItem(IDC_EXTND_FONT))->SetButtonStyle(BS_AUTOCHECKBOX);
    ((CButton*)GetDlgItem(IDC_EXTND_LINES))->SetButtonStyle(BS_AUTOCHECKBOX);
    ((CButton*)GetDlgItem(IDC_EXTND_TXTCOLOR))->SetButtonStyle(BS_AUTOCHECKBOX);
    ((CButton*)GetDlgItem(IDC_EXTND_FILLCOLOR))->SetButtonStyle(BS_AUTOCHECKBOX);
    ((CButton*)GetDlgItem(IDC_EXTND_INDENT))->SetButtonStyle(BS_AUTOCHECKBOX);
    ((CButton*)GetDlgItem(IDC_EXTND_FILLCOLOR))->SetButtonStyle(BS_AUTOCHECKBOX);
    ((CButton*)GetDlgItem(IDC_DEFAULT_INDENTATION))->SetButtonStyle(BS_AUTOCHECKBOX);
    ((CButton*)GetDlgItem(IDC_SPAN_CELLS))->SetButtonStyle(BS_AUTOCHECKBOX);
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::UpdateMultiFmtsOnOk()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::UpdateMultiFmtsOnOk()
{
    {
        if (m_eGridComp != FMT_ID_DATACELL) {
            SetLines();

        }
        //Horiz Align
        if(m_AlignH.GetCurSel() != -1){
            SetHorzAlign((TAB_HALIGN)m_AlignH.GetItemData(m_AlignH.GetCurSel()));
        }
        //Vert Align
        if(m_AlignV.GetCurSel() != -1){
            SetVertAlign((TAB_VALIGN)m_AlignV.GetItemData(m_AlignV.GetCurSel()));
        }
        //custom
        if(m_bCustom != 2) {//not indeterminate state
            CUSTOM custom;
            m_bCustom ? custom.m_bIsCustomized = true  : custom.m_bIsCustomized = false;
            if(!custom.m_bIsCustomized ){
                custom.m_sCustomText =_T("");
            }
            SetCustom(custom);
        }
        //Decimals
        if(m_decimals.GetCurSel() != -1 && m_decimals.IsWindowEnabled() ){
            NUM_DECIMALS eNumDecimals = (NUM_DECIMALS)m_decimals.GetItemData(m_decimals.GetCurSel());
            SetNumDecimals(eNumDecimals);
        }
        switch(m_eGridComp){
        case FMT_ID_TITLE:
        case FMT_ID_SUBTITLE:
        case FMT_ID_PAGENOTE:
        case FMT_ID_ENDNOTE:
        case FMT_ID_STUBHEAD:
        case FMT_ID_AREA_CAPTION:
            ASSERT(FALSE);//cannot have multiple selections of this type
            break;
        case FMT_ID_CAPTION:
            //Fall into in the case condition to set the rest of the stuff and then break
            if(m_bSpanCells != 2){// not indeterminate
                SetSpanCells(m_bSpanCells ? true : false);
            }
        case FMT_ID_SPANNER:
            if(m_bExtendLines != 2){
                SetLinesExtend();
            }
            //BMD wanted caption /spanner show hide mode
            if(m_bHide != 2){//not indeterminate
                (m_bHide && !m_bDisableHide) ? SetHidden(HIDDEN_YES):SetHidden(HIDDEN_NO);
            }
            break;
        case FMT_ID_STUB:
        case FMT_ID_COLHEAD:
            if(m_bHide != 2){//not indeterminate
                (m_bHide && !m_bDisableHide) ? SetHidden(HIDDEN_YES):SetHidden(HIDDEN_NO);
            }
            if(m_bHideZero != 2) {
                (m_bHideZero && !m_bDisableZeroHide) ? SetZeroHidden(true):SetZeroHidden(false);

            }
            if(m_bExtendFont != 2){
                SetFontExtends();
            }
            if(m_bExtendLines != 2){
                SetLinesExtend();
            }
            if(m_bExtendTxtColor != 2){
                SetTextColorExtends();
            }
            if(m_bExtendFillColor != 2){
                SetFillColorExtends();
            }
            if(m_bExtendIndent != 2){
                SetIndentationExtends();
            }
            break;
        case FMT_ID_DATACELL:
            break;
        default:break;
        }


        //do if not using default
        FMT_COLOR fmtTxtColor;
        //for now
        //  m_bDefTxtColor =false;
        fmtTxtColor.m_bUseDefault = m_bDefTxtColor;
        if(m_colorText != BOX_INDETERMINATE) {
            fmtTxtColor.m_rgb = m_colorText;
            SetTextColor(fmtTxtColor);
        }
       //do if not using default
        FMT_COLOR fmtFillColor;
        //for now
        //  m_bDefFillColor =false;
        fmtFillColor.m_bUseDefault = m_bDefFillColor;
        if(m_colorFill != BOX_INDETERMINATE) {
            fmtFillColor.m_rgb = m_colorFill;
            SetFillColor(fmtFillColor);
        }
        if (m_bDefaultIndentation != 2) {//Set Indents if it is not indeterminate
            SetIndents();
        }
        if(m_iSetMultipleFonts!=-1){
            SetMultipleFonts();
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  CFmt* CCompFmtDlg::CreateNewFmt(CTblBase* pTblBase)
//
/////////////////////////////////////////////////////////////////////////////////
CFmt* CCompFmtDlg::CreateNewFmt(CTblBase* pTblBase)
{
    ASSERT(pTblBase);
    ASSERT(!pTblBase->GetFmt());

    ASSERT(m_pTabView);


    CDataCellFmt* pDataCellFmt = new CDataCellFmt();
    CTabulateDoc* pDoc = DYNAMIC_DOWNCAST(CTabulateDoc, m_pTabView->GetDocument());
    ASSERT(pDoc);
    CFmtReg* pFmtReg  = pDoc->GetTableSpec()->GetFmtRegPtr();


    pDataCellFmt->Init(); //call for the decimals initialisation
    m_pTabView->MakeDefFmt4Dlg(pDataCellFmt,m_pDefFmt);

    pDoc->SetModifiedFlag();
    pFmtReg->AddFmt(pDataCellFmt);
    pTblBase->SetFmt(pDataCellFmt);

    return pDataCellFmt;
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetLines()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetLines()
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    int iIndex =0;
    LINE eLine;
    //Left
    if(m_BorderL.GetCurSel() != -1){
        eLine = (LINE)m_BorderL.GetItemData(m_BorderL.GetCurSel());
        for (iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
            pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(pFmt){
                pFmt->SetLineLeft(eLine);
            }
            else if(eLine != LINE_DEFAULT) {
                pFmt = CreateNewFmt(pTblBase);
                pFmt->SetLineLeft(eLine);
            }
        }
    }
    //Right
    if(m_BorderR.GetCurSel() != -1){
        eLine = (LINE)m_BorderR.GetItemData(m_BorderR.GetCurSel());
        for (iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
            pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(pFmt){
                pFmt->SetLineRight(eLine);
            }
            else if(eLine != LINE_DEFAULT) {
                pFmt = CreateNewFmt(pTblBase);
                pFmt->SetLineRight(eLine);
            }
        }
    }

    //Top
    if(m_BorderT.GetCurSel() != -1){
        eLine = (LINE)m_BorderT.GetItemData(m_BorderT.GetCurSel());
        for (iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
            pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(pFmt){
                pFmt->SetLineTop(eLine);
            }
            else if(eLine != LINE_DEFAULT) {
                pFmt = CreateNewFmt(pTblBase);
                pFmt->SetLineTop(eLine);
            }
        }
    }
    //Bottom
     //Top
    if(m_BorderB.GetCurSel() != -1){
        eLine = (LINE)m_BorderB.GetItemData(m_BorderB.GetCurSel());
        for (iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
            pTblBase = m_pArrTblBase->GetAt(iIndex);
            CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
            if(pFmt){
                pFmt->SetLineBottom(eLine);
            }
            else if(eLine != LINE_DEFAULT) {
                pFmt = CreateNewFmt(pTblBase);
                pFmt->SetLineBottom(eLine);
            }
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetHorzAlign(TAB_HALIGN eHAlign)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetHorzAlign(TAB_HALIGN eHAlign)
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetHorzAlign(eHAlign);
        }
        else if(eHAlign != HALIGN_DEFAULT) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetHorzAlign(eHAlign);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetVertAlign(TAB_VALIGN eVAlign)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetVertAlign(TAB_VALIGN eVAlign)
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetVertAlign(eVAlign);
        }
        else if(eVAlign != VALIGN_DEFAULT) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetVertAlign(eVAlign);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetCustom(CUSTOM custom)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetCustom(CUSTOM custom)
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            if (pFmt->GetCustom().m_bIsCustomized != custom.m_bIsCustomized) {
                pFmt->SetCustom(custom);
            }
        }
        else if(custom.m_bIsCustomized) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetCustom(custom);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetFillColor(FMT_COLOR fmtColor)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetFillColor(FMT_COLOR fmtColor)
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetFillColor(fmtColor);
        }
        else if(!fmtColor.m_bUseDefault) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetFillColor(fmtColor);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetTextColor(FMT_COLOR fmtColor)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetTextColor(FMT_COLOR fmtColor)
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetTextColor(fmtColor);
        }
        else if(!fmtColor.m_bUseDefault) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetTextColor(fmtColor);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetNumDecimals(NUM_DECIMALS eNumDecimals)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetNumDecimals(NUM_DECIMALS eNumDecimals)
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CDataCellFmt* pFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetNumDecimals(eNumDecimals);
        }
        else if(eNumDecimals != NUM_DECIMALS_DEFAULT) {
            CDataCellFmt* pFmt = DYNAMIC_DOWNCAST(CDataCellFmt,CreateNewFmt(pTblBase));
            ASSERT(pFmt);
            pFmt->SetNumDecimals(eNumDecimals);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetHidden(HIDDEN eHide)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetHidden(HIDDEN eHide)
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetHidden(eHide);
        }
        else if(eHide != m_pDefFmt->GetHidden()) {
            pFmt = CreateNewFmt(pTblBase);
            ASSERT(pFmt);
            pFmt->SetHidden(eHide);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetHidden(HIDDEN bHide)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetZeroHidden(bool bHide)
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    CDataCellFmt* pDefDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,m_pDefFmt);
    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CDataCellFmt* pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pTblBase->GetFmt());
        if(pDataCellFmt){
            pDataCellFmt->SetZeroHidden(bHide);
        }
        else if(pDefDataCellFmt && bHide != pDefDataCellFmt->GetZeroHidden()) {
            CFmt* pFmt = CreateNewFmt(pTblBase);
            CDataCellFmt* pDataCellFmt = DYNAMIC_DOWNCAST(CDataCellFmt,pFmt);
            ASSERT(pDataCellFmt);
            pDataCellFmt->SetZeroHidden(bHide);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetSpanCells(bool bSpanCells)
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetSpanCells(bool bSpanCells)
{
    ASSERT(m_pArrTblBase);
    CTblBase* pTblBase = NULL;
    SPAN_CELLS eDefSpanCells = m_pDefFmt->GetSpanCells();
    SPAN_CELLS eSpanCells = SPAN_CELLS_DEFAULT;
    m_bSpanCells  ? eSpanCells = SPAN_CELLS_YES :eSpanCells =SPAN_CELLS_NO;

    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetSpanCells(eSpanCells);
        }
        else if(eSpanCells != eDefSpanCells) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetSpanCells(eSpanCells);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetLinesExtend()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetLinesExtend()
{
    ASSERT(m_pArrTblBase);
    ASSERT(m_bExtendLines != 2); //not indeterminate
    CTblBase* pTblBase = NULL;
    bool bDefExtends = m_pDefFmt->GetLinesExtend();

    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetLinesExtend(m_bExtendLines ? true : false);
        }
        else if(bDefExtends != m_bExtendLines) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetLinesExtend(m_bExtendLines ? true : false);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetFontExtends()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetFontExtends()
{
    ASSERT(m_pArrTblBase);
    ASSERT(m_bExtendFont != 2); //not indeterminate
    CTblBase* pTblBase = NULL;
    bool bDefExtends = m_pDefFmt->GetFontExtends();

    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetFontExtends(m_bExtendFont ? true : false);
        }
        else if(bDefExtends != m_bExtendFont) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetFontExtends(m_bExtendFont ? true : false);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetMultipleFonts()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetMultipleFonts()
{
    ASSERT(m_pArrTblBase);
    ASSERT( m_iSetMultipleFonts != -1); //not indeterminate
    CTblBase* pTblBase = NULL;

    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetFont(m_pFmt->GetFont());
        }
        else if(!m_pFmt->GetFont()) { //if it is not default font
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetFont(m_pFmt->GetFont());
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetIndents()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetIndents()
{
    ASSERT(m_pArrTblBase);
    ASSERT(m_bDefaultIndentation != 2); //not indeterminate
    CTblBase* pTblBase = NULL;

    bool m_bMetric = m_pDefFmt->GetUnits() == UNITS_METRIC;
    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            if(!m_bDefaultIndentation){
              m_bMetric? pFmt->SetIndent(LEFT,CmToTwips(m_fIndentLeft)) :pFmt->SetIndent(LEFT,InchesToTwips(m_fIndentLeft)) ;
              m_bMetric? pFmt->SetIndent(RIGHT,CmToTwips(m_fIndentRight)) : pFmt->SetIndent(RIGHT,InchesToTwips(m_fIndentRight));
            }
            else {
              pFmt->SetIndent(LEFT,INDENT_DEFAULT);
              pFmt->SetIndent(RIGHT,INDENT_DEFAULT);
            }
        }
        else if(!m_bDefaultIndentation) { //if it is not default indentation
            pFmt = CreateNewFmt(pTblBase);
            m_bMetric? pFmt->SetIndent(LEFT,CmToTwips(m_fIndentLeft)) :pFmt->SetIndent(LEFT,InchesToTwips(m_fIndentLeft)) ;
            m_bMetric? pFmt->SetIndent(RIGHT,CmToTwips(m_fIndentRight)) : pFmt->SetIndent(RIGHT,InchesToTwips(m_fIndentRight));

        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetTextColorExtends()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetTextColorExtends()
{
    ASSERT(m_pArrTblBase);
    ASSERT(m_bExtendTxtColor != 2); //not indeterminate
    CTblBase* pTblBase = NULL;
    bool bDefExtends = m_pDefFmt->GetTextColorExtends();

    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetTextColorExtends(m_bExtendTxtColor ? true : false);
        }
        else if(bDefExtends != m_bExtendTxtColor) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetTextColorExtends(m_bExtendTxtColor ? true : false);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetFillColorExtends()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetFillColorExtends()
{
    ASSERT(m_pArrTblBase);
    ASSERT(m_bExtendFillColor != 2); //not indeterminate
    CTblBase* pTblBase = NULL;
    bool bDefExtends = m_pDefFmt->GetFillColorExtends();

    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetFillColorExtends(m_bExtendFillColor ? true : false);
        }
        else if(bDefExtends != m_bExtendFillColor) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetFillColorExtends(m_bExtendFillColor ? true : false);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////
//
//  void CCompFmtDlg::SetIndentationExtends()
//
/////////////////////////////////////////////////////////////////////////////////
void CCompFmtDlg::SetIndentationExtends()
{
    ASSERT(m_pArrTblBase);
    ASSERT(m_bExtendIndent != 2); //not indeterminate
    CTblBase* pTblBase = NULL;
    bool bDefExtends = m_pDefFmt->GetIndentationExtends();

    for (int iIndex =0; iIndex < m_pArrTblBase->GetSize();iIndex++){
        pTblBase = m_pArrTblBase->GetAt(iIndex);
        CFmt* pFmt = DYNAMIC_DOWNCAST(CFmt,pTblBase->GetFmt());
        if(pFmt){
            pFmt->SetIndentationExtends(m_bExtendIndent ? true : false);
        }
        else if(bDefExtends != m_bExtendFillColor) {
            pFmt = CreateNewFmt(pTblBase);
            pFmt->SetIndentationExtends(m_bExtendIndent ? true : false);
        }
    }
}
