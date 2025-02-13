#include "StdAfx.h"
#include "AplFileAssociationsDlg.h"
#include "NewFileCreator.h"
#include <zToolsO/DirectoryLister.h>


/////////////////////////////////////////////////////////////////////////////
//
//                               CAplFileAssociationsDlg::CAplFileAssociationsDlg
//
/////////////////////////////////////////////////////////////////////////////

CAplFileAssociationsDlg::CAplFileAssociationsDlg(CWnd* pParent /*=NULL*/)
    :   CDialog(CAplFileAssociationsDlg::IDD, pParent),
        m_workingStorageType(WorkingStorageType::Editable),
        m_bWorkingStorage(FALSE),
        m_nextSuggestedDictionaryIndex(0)
{
}


void CAplFileAssociationsDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CAplFileAssociationsDlg)
    //}}AFX_DATA_MAP
    DDX_Check(pDX, IDC_WORKING, m_bWorkingStorage);
}


/////////////////////////////////////////////////////////////////////////////
// CAplFileAssociationsDlg message handlers

BOOL CAplFileAssociationsDlg::OnInitDialog()
{
    BOOL bRet = CDialog::OnInitDialog();
    if(!bRet) {
        return bRet;
    }
    SetWindowText(m_sTitle);

    CWnd* pWSCWnd = GetDlgItem(IDC_WORKING);
    if (m_workingStorageType == WorkingStorageType::Hidden) {
        pWSCWnd->ShowWindow(SW_HIDE);
    }
    else {
        CString wsd_filename = ( m_workingStorageType == WorkingStorageType::ReadOnly ) ?
            m_sWSDName : WS2CS(NewFileCreator::GetDefaultWorkingStorageDictionaryFilename(m_sAppName));
        pWSCWnd->SetWindowText(_T("Working Storage Dictionary - ") + wsd_filename);
        if (m_workingStorageType == WorkingStorageType::ReadOnly) {
            pWSCWnd->EnableWindow(FALSE);
        }
    }

    // Set Up File Association Grid
    CWnd* pWnd = GetDlgItem(IDC_NEWGRID);
    pWnd->GetClientRect(&m_Rect);

    m_aplFileAssociationsGrid.m_iCols = 1 ;
    m_aplFileAssociationsGrid.m_iRows = (int)m_fileAssociations.size();

    pWnd->SetFocus();
    m_aplFileAssociationsGrid.AttachGrid(this,IDC_NEWGRID);
    SetGridData();

    //Show the grid
    m_aplFileAssociationsGrid.SetFocus();
    m_aplFileAssociationsGrid.ShowWindow(SW_SHOW);
    m_aplFileAssociationsGrid.GotoCell(0,0);
    m_aplFileAssociationsGrid.StartEdit();
    m_aplFileAssociationsGrid.m_pPifEdit.SetFocus();
    m_aplFileAssociationsGrid.m_pPifEdit.SetCaretPos(CPoint(0,0));

    return bRet;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}


void CAplFileAssociationsDlg::SetGridData()
{
    int descriptionColumnWidth = m_Rect.Width() / 5; // 20120510 changed from 4 to 5 because of enlarging the window
    m_aplFileAssociationsGrid.SetColWidth(-1, descriptionColumnWidth);
    m_aplFileAssociationsGrid.SetColWidth(0, m_Rect.Width() - descriptionColumnWidth - GetSystemMetrics(SM_CXVSCROLL));

    for( int i = 0 ; i < (int)m_fileAssociations.size(); ++i )
    {
        m_aplFileAssociationsGrid.QuickSetText(-1, i, m_fileAssociations[i].GetDescription());
        m_aplFileAssociationsGrid.QuickSetAlignment(-1, i, UG_ALIGNLEFT);
        m_aplFileAssociationsGrid.QuickSetCellType(0, i, m_aplFileAssociationsGrid.m_iEllipsisIndex);
        m_aplFileAssociationsGrid.QuickSetText(0, i, m_fileAssociations[i].GetNewFilename());
        m_aplFileAssociationsGrid.QuickSetAlignment(0, i, UG_ALIGNLEFT);
    }
}


void CAplFileAssociationsDlg::OnOK()
{
    for( size_t i = 0 ; i < m_fileAssociations.size(); ++i )
    {
        FileAssociation& file_association = m_fileAssociations[i];

        if( !file_association.IsRequired() )
            continue;

        CIMSAString sTemp;
        m_aplFileAssociationsGrid.QuickGetText(0, (int)i, &sTemp);

        if( SO::IsBlank(sTemp) )
        {
            AfxMessageBox(file_association.GetDescription() + _T(" is required!"));
            return;
        }
    }

    for( size_t i = 0 ; i < m_fileAssociations.size(); ++i )
    {
        CString new_filename;
        m_aplFileAssociationsGrid.QuickGetText(0, (int)i, &new_filename);
        m_fileAssociations[i].SetNewFilename(new_filename);
    }

    CDialog::OnOK();
}


const FileAssociation* CAplFileAssociationsDlg::FindFileAssoc(LPCTSTR sFilePathOrig) const
{
    int i = FindFileAssocIndex(sFilePathOrig);
    return  ( i == NONE ) ? nullptr : &m_fileAssociations[i];
}

int CAplFileAssociationsDlg::FindFileAssocIndex(LPCTSTR sFilePathOrig) const
{
    for( size_t i = 0; i < m_fileAssociations.size(); ++i )
    {
        if( m_fileAssociations[i].GetOriginalFilename().CompareNoCase(sFilePathOrig) == 0 )
            return (int)i;
    }

    return NONE;
}


BOOL CAplFileAssociationsDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_KEYDOWN) {
        bool bCtrl = GetKeyState(VK_CONTROL) < 0;
        //bool bT = (pMsg->wParam == _T('m') || pMsg->wParam == _T('M'));
        //if (bCtrl && bT) {
        if( bCtrl )
        {
            // 20110805 shortcuts so that the user doesn't have to click on the ellipses
            if( pMsg->wParam >= _T('1') && pMsg->wParam <= _T('9') )
            {
                int rowNum = pMsg->wParam - _T('1');

                if( rowNum < (int)m_fileAssociations.size() )
                {
                    GetDlgItem(IDOK)->SetFocus(); // there was a problem if the grid cell to be changed had focus
                    m_aplFileAssociationsGrid.ForceButtonClick(rowNum);
                    m_aplFileAssociationsGrid.RedrawCell(0,rowNum);
                    return TRUE;
                }
            }

            // 20120527, control + space will cycle through possible dictionaries if the Input Dictionary line is blank
            else if( pMsg->wParam == VK_SPACE ) 
            {
                if( m_suggestedDictionaries.empty() )
                {
                    EstablishSuggestedDictionaries();
                    ASSERT(m_nextSuggestedDictionaryIndex == 0 && !m_suggestedDictionaries.empty());
                }

                else if( ++m_nextSuggestedDictionaryIndex >= m_suggestedDictionaries.size() )
                {
                    m_nextSuggestedDictionaryIndex = 0;
                }

                GetDlgItem(IDOK)->SetFocus(); // change the focus (see above on my 20110805 work)
                m_aplFileAssociationsGrid.QuickSetText(0, 0, m_suggestedDictionaries[m_nextSuggestedDictionaryIndex].c_str());
                m_aplFileAssociationsGrid.RedrawCell(0, 0);

                return TRUE;
            }
        }
    }

    return CDialog::PreTranslateMessage(pMsg);
}



void CAplFileAssociationsDlg::EstablishSuggestedDictionaries() // 20120527
{
    // we will suggest:
    //  1) a dictionary named after the application (the default behavior in versions prior to 5.0)
    //  2) any dictionary that is in the folder where the application will be created

    std::wstring original_suggestion = PortableFunctions::PathRemoveFileExtension(m_sAppName) + FileExtensions::WithDot::Dictionary;
    m_suggestedDictionaries.emplace_back(original_suggestion);

    for( const std::wstring& dictionary_filename : DirectoryLister().SetNameFilter(FileExtensions::Wildcard::Dictionary)
                                                                    .GetPaths(PortableFunctions::PathGetDirectory(m_sAppName)) )
    {
        if( !SO::EqualsNoCase(dictionary_filename, original_suggestion) )
            m_suggestedDictionaries.emplace_back(dictionary_filename);
    }
}
