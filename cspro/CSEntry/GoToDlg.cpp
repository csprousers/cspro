// GoToDlg.cpp : implementation file
//
#include "StdAfx.h"
#include "GoToDlg.h"
#include "MainFrm.h"
#include "Rundoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[]= __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CNumEdit

CNumEdit::CNumEdit()
{
}

CNumEdit::~CNumEdit()
{
}


BEGIN_MESSAGE_MAP(CNumEdit, CEdit)
    //{{AFX_MSG_MAP(CNumEdit)
    ON_WM_KEYDOWN()
    ON_WM_CHAR()
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNumEdit message handlers

void CNumEdit::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    CEdit::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CNumEdit::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if( ( _TCHAR(nChar) >= _T('0') && _TCHAR(nChar) <= '9' ) || nChar == VK_BACK_SPACE ) {
        CEdit::OnChar(nChar, nRepCnt, nFlags);
    }
    else {
        return;
    }
}

/////////////////////////////////////////////////////////////////////////////
// CGoToDlg dialog


CGoToDlg::CGoToDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CGoToDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CGoToDlg)
    m_sName = _T("");
//  m_iOcc = -1;
    //}}AFX_DATA_INIT
    m_pBase = NULL;
    m_iOcc = -1;
}


void CGoToDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGoToDlg)
    DDX_Text(pDX, IDC_NAME, m_sName);
    //}}AFX_DATA_MAP
}

/////////////////////////////////////////////////////////////////////////////
// CGoToDlg message handlers

void CGoToDlg::OnOK()
{

    if(!UpdateData(TRUE)) {
        return;
    }

    CIMSAString sText;
    m_numEdit.GetWindowText(sText);

    if(SO::IsBlank(sText) || sText.IsEmpty() ){
        m_iOcc = -1;
    }
    else {
        m_iOcc = _ttoi(sText);
    }

    if(m_iOcc < -1 ){
        AfxMessageBox(MGF::GetMessageText(MGF::GotoInvalidOccurrence).c_str());
        return;
    }
    m_sName.MakeUpper();
    CMainFrame* pFrame = (CMainFrame*) AfxGetMainWnd();
    CEntryrunDoc* pDoc = (CEntryrunDoc*)pFrame->GetActiveDocument();

    CDEFormFile* pFile = pDoc->GetCurFormFile();
    CDEForm* pForm = NULL;
    if(pFile->FindField(m_sName,&pForm,&m_pBase)) {

    }
    else {
        AfxMessageBox(MGF::GetMessageText(MGF::GotoItemNotFound).c_str());
        return;
    }
    CDEField* pField = DYNAMIC_DOWNCAST(CDEField,m_pBase);
    if(pField == NULL){
        AfxMessageBox(MGF::GetMessageText(MGF::GotoItemNotFound).c_str());
        return;
    }

    CDialog::OnOK();
}

BOOL CGoToDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    m_numEdit.SubclassDlgItem(IDC_OCC,this);
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}
