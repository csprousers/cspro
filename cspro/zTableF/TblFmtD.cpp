// TblFmtD.cpp : implementation file
//

#include "StdAfx.h"
#include "TblFmtD.h"


// CTblFmtDlg dialog

IMPLEMENT_DYNAMIC(CTblFmtDlg, CDialog)
CTblFmtDlg::CTblFmtDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CTblFmtDlg::IDD, pParent)
    , m_bIncludeSubTitle(FALSE)
    , m_bIncludePageNote(FALSE)
    , m_bIncludeEndNote(FALSE)
    , m_bShowDefaults(TRUE)
{
    m_bViewer = false;
}

CTblFmtDlg::~CTblFmtDlg()
{
}

void CTblFmtDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Check(pDX, IDC_INCL_SUBTITLE, m_bIncludeSubTitle);
    DDX_Check(pDX, IDC_INCL_PGNOTE, m_bIncludePageNote);
    DDX_Check(pDX, IDC_INCL_FOOTNOTE, m_bIncludeEndNote);
    DDX_Control(pDX, IDC_LSTUB_LEADERING, m_LStubLeadering);
    DDX_Control(pDX, IDC_RSTUB_LEADERING, m_RStubLeadering);
    DDX_Control(pDX, IDC_FRQ_RDRBRKS, m_FrqRdrBreaks);
    DDX_Control(pDX, IDC_BRDRL, m_BorderL);
    DDX_Control(pDX, IDC_BRDRR, m_BorderR);
    DDX_Control(pDX, IDC_BRDRT, m_BorderT);
    DDX_Control(pDX, IDC_BRDRB, m_BorderB);
}


BEGIN_MESSAGE_MAP(CTblFmtDlg, CDialog)
    ON_BN_CLICKED(IDC_RESET, OnBnClickedReset)
END_MESSAGE_MAP()


// CTblFmtDlg message handlers

BOOL CTblFmtDlg::OnInitDialog()
{
    m_bIncludeSubTitle = m_pFmt->HasSubTitle();
    m_bIncludePageNote = m_pFmt->HasPageNote();
    m_bIncludeEndNote = m_pFmt->HasEndNote();
    CDialog::OnInitDialog();

    PrepareBordersControls(m_pDefFmt->GetBorderLeft(),m_pFmt->GetBorderLeft(),&m_BorderL);
    PrepareBordersControls(m_pDefFmt->GetBorderRight(),m_pFmt->GetBorderRight(),&m_BorderR);
    PrepareBordersControls(m_pDefFmt->GetBorderTop(),m_pFmt->GetBorderTop(),&m_BorderT);
    PrepareBordersControls(m_pDefFmt->GetBorderBottom(),m_pFmt->GetBorderBottom(),&m_BorderB);

    PrepareLeadering(m_pDefFmt->GetLeadering(LEFT), m_pFmt->GetLeadering(LEFT) , &m_LStubLeadering);
    PrepareLeadering(m_pDefFmt->GetLeadering(RIGHT), m_pFmt->GetLeadering(RIGHT) ,&m_RStubLeadering);

    if(m_bViewer){
        m_FrqRdrBreaks.EnableWindow(FALSE);
    }
    else {
        PrepareRdrBrkCtl();
    }
    return TRUE;  // return TRUE unless you set the focus to a control
    // EXCEPTION: OCX Property Pages should return FALSE
}

void CTblFmtDlg::OnBnClickedReset()
{
    //reset borders
    m_pFmt->SetBorderTop(LINE_DEFAULT);
    m_pFmt->SetBorderBottom(LINE_DEFAULT);
    m_pFmt->SetBorderLeft(LINE_DEFAULT);
    m_pFmt->SetBorderRight(LINE_DEFAULT);

    // include
    m_pFmt->SetIncludeEndNote(m_pDefFmt->HasEndNote());
    m_pFmt->SetIncludePageNote(m_pDefFmt->HasPageNote());
    m_pFmt->SetIncludeSubTitle(m_pDefFmt->HasSubTitle());

    m_bIncludeEndNote  = m_pDefFmt->HasEndNote();
    m_bIncludePageNote  = m_pDefFmt->HasPageNote();
    m_bIncludeSubTitle = m_pDefFmt->HasSubTitle();

    m_pFmt->SetReaderBreak(READER_BREAK_DEFAULT);

    m_pFmt->SetLeadering(LEFT,LEADERING_DEFAULT);
    m_pFmt->SetLeadering(RIGHT,LEADERING_DEFAULT);

//Now let the controls reflect the defaults
    m_BorderL.SetCurSel(0);
    m_BorderR.SetCurSel(0);
    m_BorderT.SetCurSel(0);
    m_BorderB.SetCurSel(0);

    m_LStubLeadering.SetCurSel(0);
    m_RStubLeadering.SetCurSel(0);
    m_FrqRdrBreaks.SetCurSel(0);

    UpdateData(FALSE);

}

void CTblFmtDlg::OnOK()
{
    // TODO: Add your specialized code here and/or call the base class
    CDialog::OnOK();
    m_pFmt->SetIncludeSubTitle(m_bIncludeSubTitle? true : false);
    m_pFmt->SetIncludePageNote(m_bIncludePageNote ? true : false);
    m_pFmt->SetIncludeEndNote(m_bIncludeEndNote ? true : false);

    m_pFmt->SetBorderLeft((LINE)m_BorderL.GetItemData(m_BorderL.GetCurSel()));
    m_pFmt->SetBorderRight((LINE)m_BorderR.GetItemData(m_BorderR.GetCurSel()));
    m_pFmt->SetBorderTop((LINE)m_BorderT.GetItemData(m_BorderT.GetCurSel()));
    m_pFmt->SetBorderBottom((LINE)m_BorderB.GetItemData(m_BorderB.GetCurSel()));

    m_pFmt->SetLeadering(LEFT,(LEADERING)m_LStubLeadering.GetItemData(m_LStubLeadering.GetCurSel()));
    m_pFmt->SetLeadering(RIGHT,(LEADERING)m_RStubLeadering.GetItemData(m_RStubLeadering.GetCurSel()));

    m_pFmt->SetReaderBreak((READER_BREAK)m_FrqRdrBreaks.GetItemData(m_FrqRdrBreaks.GetCurSel()));
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblFmtDlg::PrepareLinesControls(LINE eLineDef, , LINE eLine ,CComboBox* pComboBox)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblFmtDlg::PrepareBordersControls(LINE eLineDef,LINE eLine ,CComboBox* pComboBox)
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

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblFmtDlg::PrepareLeadering(LEADERING eLeaderingDef, LEADERING eLeadering , CComboBox* pComboBox)
//
/////////////////////////////////////////////////////////////////////////////////
void CTblFmtDlg::PrepareLeadering(LEADERING eLeaderingDef, LEADERING eLeadering , CComboBox* pComboBox)
{
    if (m_bShowDefaults) {
        switch(eLeaderingDef){
            case LEADERING_DEFAULT:
                ASSERT(FALSE);
                break;
            case LEADERING_NONE:
                pComboBox->AddString(_T("Default (None)"));
                break;
            case LEADERING_DOT:
                pComboBox->AddString(_T("Default (...)"));
                break;
            case LEADERING_DOT_SPACE:
                pComboBox->AddString(_T("Default (. .)"));
                break;
            case  LEADERING_DASH:
                pComboBox->AddString(_T("Default (--)"));
                break;
            case LEADERING_DASH_SPACE:
                pComboBox->AddString(_T("Default (- -)"));
                break;
            default:
                ASSERT(FALSE);
                break;
        }
        pComboBox->SetItemData(0,LEADERING_DEFAULT);
    }

     pComboBox->SetItemData(pComboBox->AddString(_T("None")),LEADERING_NONE);

     pComboBox->SetItemData(pComboBox->AddString(_T(".......")),LEADERING_DOT);

     pComboBox->SetItemData(pComboBox->AddString(_T(". . . .")),LEADERING_DOT_SPACE);

     pComboBox->SetItemData(pComboBox->AddString(_T("------")),LEADERING_DASH);

     pComboBox->SetItemData(pComboBox->AddString(_T("- - - - -")),LEADERING_DASH_SPACE);


    switch(eLeadering){
        case LEADERING_DEFAULT:
            ASSERT(m_bShowDefaults);
            pComboBox->SetCurSel(0);
            break;
        case LEADERING_NONE:
            pComboBox->SetCurSel(m_bShowDefaults ? 1 : 0);
            break;
        case LEADERING_DOT:
            pComboBox->SetCurSel(m_bShowDefaults ? 2 : 1);
            break;
        case LEADERING_DOT_SPACE:
            pComboBox->SetCurSel(m_bShowDefaults ? 3 : 2);
            break;
        case LEADERING_DASH:
            pComboBox->SetCurSel(m_bShowDefaults ? 4 : 3);
            break;
        case LEADERING_DASH_SPACE:
            pComboBox->SetCurSel(m_bShowDefaults ? 5 : 4);
            break;
        default:
            ASSERT(FALSE);
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////////
//
//  void CTblFmtDlg::PrepareRdrBrkCtl()
//
/////////////////////////////////////////////////////////////////////////////////
void CTblFmtDlg::PrepareRdrBrkCtl()
{
    ASSERT(m_pDefFmt);
    ASSERT(m_pFmt);

    if (m_bShowDefaults) {
        CIMSAString sReaderBreak;
        int iDefRdrBrk = -1;
        switch(m_pDefFmt->GetReaderBreak()){
            case READER_BREAK_DEFAULT:
                ASSERT(FALSE);
                break;
            case READER_BREAK_NONE:
                sReaderBreak = _T(" (None)");
                iDefRdrBrk=0;
                break;
            case READER_BREAK_ONE:
                sReaderBreak =_T(" (1)");
                iDefRdrBrk=1;
                break;
            case READER_BREAK_TWO:
                sReaderBreak =_T(" (2)");
                iDefRdrBrk=2;
                break;
            case READER_BREAK_THREE:
                sReaderBreak =_T(" (3)");
                iDefRdrBrk=3;
                break;
            case READER_BREAK_FOUR:
                sReaderBreak =_T(" (4)");
                iDefRdrBrk=4;
                break;
            case READER_BREAK_FIVE:
                sReaderBreak =_T(" (5)");
                iDefRdrBrk=5;
                break;
            case READER_BREAK_SIX:
                sReaderBreak =_T(" (6)");
                iDefRdrBrk=6;
                break;
            case READER_BREAK_SEVEN:
                sReaderBreak =_T(" (7)");
                iDefRdrBrk=7;
                break;
            case READER_BREAK_EIGHT:
                sReaderBreak =_T(" (8)");
                iDefRdrBrk=8;
                break;
            case READER_BREAK_NINE:
                sReaderBreak =_T(" (9)");
                iDefRdrBrk=9;
                break;
            case READER_BREAK_TEN:
                sReaderBreak =_T(" (10)");
                iDefRdrBrk=10;
                break;
            default:
                ASSERT(FALSE);
                break;
        }
        CIMSAString sDef = _T("Default ")+ sReaderBreak;
        m_FrqRdrBreaks.AddString(sDef);
        m_FrqRdrBreaks.SetItemData(iDefRdrBrk,READER_BREAK_DEFAULT);
    }

    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("None")),READER_BREAK_NONE);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("1")),READER_BREAK_ONE);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("2")),READER_BREAK_TWO);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("3")),READER_BREAK_THREE);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("4")),READER_BREAK_FOUR);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("5")),READER_BREAK_FIVE);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("6")),READER_BREAK_SIX);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("7")),READER_BREAK_SEVEN);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("8")),READER_BREAK_EIGHT);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("9")),READER_BREAK_NINE);
    m_FrqRdrBreaks.SetItemData(m_FrqRdrBreaks.AddString(_T("10")),READER_BREAK_TEN);

    switch(m_pFmt->GetReaderBreak()){
        case READER_BREAK_DEFAULT:
            ASSERT(m_bShowDefaults);
            m_FrqRdrBreaks.SetCurSel(0);
            break;
        case READER_BREAK_NONE:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 1 : 0);
            break;
        case READER_BREAK_ONE:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 2 : 1);
            break;
        case READER_BREAK_TWO:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 3 : 2);
            break;
        case READER_BREAK_THREE:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 4 : 3);
            break;
        case READER_BREAK_FOUR:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 5 : 4);
            break;
        case READER_BREAK_FIVE:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 6 : 5);
            break;
        case READER_BREAK_SIX:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 7 : 6);
            break;
        case READER_BREAK_SEVEN:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 8 : 7);
            break;
        case READER_BREAK_EIGHT:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 9 : 8);
            break;
        case READER_BREAK_NINE:
           m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 10 : 9);
            break;
        case READER_BREAK_TEN:
            m_FrqRdrBreaks.SetCurSel(m_bShowDefaults ? 11 : 10);
            break;
        default:
            ASSERT(FALSE);
            break;
    }
}
