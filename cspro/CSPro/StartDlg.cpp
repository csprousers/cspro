// StartDlg.cpp : implementation file
//

#include "StdAfx.h"
#include "StartDlg.h"


// CStartDlg dialog

IMPLEMENT_DYNAMIC(CStartDlg, CDialog)
CStartDlg::CStartDlg(CWnd* pParent /*=NULL*/)
    : CDialog(CStartDlg::IDD, pParent)
    , m_iChoice(1)
    , m_iSelection(0)
{
}

CStartDlg::~CStartDlg()
{
}

void CStartDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Radio(pDX, IDC_CREATE_NEW, m_iChoice);
}

BEGIN_MESSAGE_MAP(CStartDlg, CDialog)
    ON_WM_PAINT()
    ON_NOTIFY(NM_DBLCLK, IDC_FILE_LIST, OnNMDblclkFileList)
    ON_BN_SETFOCUS(IDC_OPEN_EXISTING, OnBnSetfocusOpenExisting)
    ON_BN_SETFOCUS(IDC_CREATE_NEW, OnBnSetfocusCreateNew)
    ON_NOTIFY(NM_CLICK, IDC_FILE_LIST, OnNMClickFileList)
    ON_BN_DOUBLECLICKED(IDC_CREATE_NEW, OnBnDoubleclickedCreateNew)
END_MESSAGE_MAP()


// CStartDlg message handlers

BOOL CStartDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    m_hIcon1 = AfxGetApp()->LoadIcon(IDI_FILE_NEW);
    m_hIcon2 = AfxGetApp()->LoadIcon(IDI_FILE_OPEN);

    // GHM 20120606 added ILC_COLOR32 for alex's new icons
    m_cImageList.Create(16, 16, ILC_COLOR32, 0, 4);
    m_cImageList.SetBkColor((RGB(255,255,255)));
    HICON icon0 = AfxGetApp()->LoadIcon(IDI_DCF_FILE);
    HICON icon1 = AfxGetApp()->LoadIcon(IDI_ENT_FILE);
    HICON icon2 = AfxGetApp()->LoadIcon(IDI_BCH_FILE);
    HICON icon3 = AfxGetApp()->LoadIcon(IDI_XTB_FILE);
    HICON icon4 = AfxGetApp()->LoadIcon(IDI_FRM_FILE);

    m_cImageList.Add(icon0);
    m_cImageList.Add(icon1);
    m_cImageList.Add(icon2);
    m_cImageList.Add(icon3);
    m_cImageList.Add(icon4);

    CListCtrl* pFileList = (CListCtrl*) GetDlgItem(IDC_FILE_LIST);
    pFileList->InsertColumn(0, _T(""));
    pFileList->SetImageList(&m_cImageList, LVSIL_SMALL);
    pFileList->InsertItem(0, _T("... other files           "), -1);
    pFileList->SetItemState(0, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);

    CString sTemp, sFile, sExt;
    int iIcon;
    for (int i = 1 ; i <= 16 ; i++) {
        sTemp.Format(_T("File%d"),i);
        sFile = AfxGetApp()->GetProfileString(_T("Recent File List"), sTemp, _T("?"));
        if (sFile == _T("?")) {
            break;
        }
        else {
            sExt = PathFindExtension(sFile.GetBuffer(_MAX_PATH));
            sFile.ReleaseBuffer();
            if (sExt.CompareNoCase(FileExtensions::WithDot::Dictionary) == 0) {
                iIcon = 0;
            }
            else if (sExt.CompareNoCase(FileExtensions::WithDot::EntryApplication) == 0) {
                iIcon = 1;
            }
            else if (sExt.CompareNoCase(FileExtensions::WithDot::BatchApplication) == 0) {
                iIcon = 2;
            }
            else if (sExt.CompareNoCase(FileExtensions::WithDot::TabulationApplication) == 0) {
                iIcon = 3;
            }
            else if (sExt.CompareNoCase(_T(".frm")) == 0) {
                iIcon = 4;
            }
            else {
                iIcon = -1;
            }
            pFileList->InsertItem(i, sFile, iIcon);
            pFileList->SetItemState(0, 0, LVIS_FOCUSED | LVIS_SELECTED);
            pFileList->SetItemState(1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        }
    }
    pFileList->SetColumnWidth(0,LVSCW_AUTOSIZE);
    return TRUE;
}

void CStartDlg::OnPaint()
{
    CPaintDC dc(this); // device context for painting
    if (!IsIconic()) {
        // Insert New Icon
        CWnd* pIcon1 = GetDlgItem(IDC_FILE_NEW);
        ASSERT_VALID(pIcon1);
        CPaintDC dc1(pIcon1);
        dc1.DrawIcon(0, 0, m_hIcon1);
        // Insert Open Icon
        CWnd* pIcon2 = GetDlgItem(IDC_FILE_OPEN);
        ASSERT_VALID(pIcon2);
        CPaintDC dc2(pIcon2);
        dc2.DrawIcon(0, 0, m_hIcon2);
    }
}
void CStartDlg::OnBnSetfocusCreateNew()
{
    m_iChoice = 0;
    UpdateData(FALSE);
    CListCtrl* pFileList = (CListCtrl*) GetDlgItem(IDC_FILE_LIST);
    pFileList->EnableWindow(FALSE);
}

void CStartDlg::OnBnSetfocusOpenExisting()
{
    m_iChoice = 1;
    UpdateData(FALSE);
    CListCtrl* pFileList = (CListCtrl*) GetDlgItem(IDC_FILE_LIST);
    pFileList->EnableWindow(TRUE);
//  pFileList->SetFocus();
}

void CStartDlg::OnNMClickFileList(NMHDR */*pNMHDR*/, LRESULT *pResult)
{
    m_iChoice = 1;
    UpdateData(FALSE);
    *pResult = 0;
}

void CStartDlg::OnNMDblclkFileList(NMHDR */*pNMHDR*/, LRESULT *pResult)
{
    m_iChoice = 1;
    UpdateData(FALSE);
    OnOK();
    *pResult = 0;
}

void CStartDlg::OnOK()
{
    m_iSelection = 0;
    CListCtrl* pFileList = (CListCtrl*) GetDlgItem(IDC_FILE_LIST);
    for (int i = 0 ; i < pFileList->GetItemCount() ; i++) {
        if (pFileList->GetItemState(i, LVIS_SELECTED)) {
            m_iSelection = i;
            break;
        }
    }
    CDialog::OnOK();
}

void CStartDlg::OnBnDoubleclickedCreateNew()
{
    m_iChoice = 0;
    UpdateData(FALSE);
    OnOK();
}
