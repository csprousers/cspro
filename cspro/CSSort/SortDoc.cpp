//***************************************************************************
//  File name: SortDoc.cpp
//
//  Description:
//       CSSort document implementation
//
//  History:    Date       Author   Comment
//              ---------------------------
//              11 Dec 00   bmd     Created for CSPro 2.1
//
//***************************************************************************
//  Functional specs:
//      Sorts file up to 2 GB by questionnaire
//      In record sort, duplicate keys remain in same order as original file
//      In questionnaire sort, duplicate ids are flaged and sort fails
//      If more than 1 record type or max record > 1 for one record type
//          only level 1 ids can be used as keys
//      otherwise
//          any items can be used as keys
//
// !!!!!!!! The sort was changed radically for CSPro 7.0, using a SQLite
//          database instead to handle the sorting
//***************************************************************************

#include "StdAfx.h"
#include "SortDoc.h"
#include "CSSort.h"
#include "SortView.h"
#include "TwoFldlg.h"
#include "TypeDlg.h"
#include <zUtilO/Filedlg.h>
#include <zSortO/Sorter.h>


IMPLEMENT_DYNCREATE(CSortDoc, CDocument)

BEGIN_MESSAGE_MAP(CSortDoc, CDocument)
    ON_COMMAND(ID_FILE_RUN, OnFileRun)
    ON_COMMAND(ID_FILE_SAVE, OnFileSave)
    ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE, OnUpdateFileSave)
    ON_UPDATE_COMMAND_UI(ID_FILE_SAVE_AS, OnUpdateFileSaveAs)
    ON_COMMAND(ID_OPTIONS_SORT_TYPE, OnOptionsSortType)
    ON_UPDATE_COMMAND_UI(ID_OPTIONS_SORT_TYPE, OnUpdateOptionsSortType)
END_MESSAGE_MAP()


CSortDoc::CSortDoc()
    :   m_bRetSave(false)
{
    m_pff.SetAppType(SORT_TYPE);
    m_pff.SetViewListing(ONERROR);
    m_pff.SetViewResultsFlag(false);
}


BOOL CSortDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    if( !CDocument::OnOpenDocument(lpszPathName) )
        return FALSE;

    CString extension = PortableFunctions::PathGetFileExtension<CString>(lpszPathName);

    if (extension.CompareNoCase(FileExtensions::Pff) == 0) {
        m_pff.SetPifFileName(lpszPathName);
        if (m_pff.LoadPifFile()) {
            RunBatchSort();
            assert_cast<CSortApp*>(AfxGetApp())->m_iReturnCode = 0;
        }
        else {
            assert_cast<CSortApp*>(AfxGetApp())->m_iReturnCode = 8;
        }
        return FALSE;
    }

    else if (extension.CompareNoCase(FileExtensions::SortSpec) == 0) {
        if (OpenSpecFile(lpszPathName)) {
            AfxGetApp()->WriteProfileString(_T("Settings"),_T("Last Open"), lpszPathName);
            CString csFileName = lpszPathName;
            m_pff.SetAppFName(csFileName);
            m_pff.SetListingFName(csFileName + FileExtensions::WithDot::Listing);
            CString csPFF = csFileName + FileExtensions::WithDot::Pff;
            m_pff.SetPifFileName(csPFF);
            CFileStatus status;
            if (CFile::GetStatus(csPFF, status)) {
                if (m_pff.LoadPifFile()) {
                    if (csFileName.CompareNoCase(m_pff.GetAppFName()) != 0) {
                        AfxMessageBox(FormatText(_T("Spec files in %s\ndoes not match %s"), csPFF.GetString(), csFileName.GetString()));
                        return FALSE;
                    }
                }
            }
        }
        else {
            return FALSE;
        }
    }
    else if (extension.CompareNoCase(FileExtensions::Dictionary) ==0) {
        m_pff.SetAppFName(_T(""));
        if (!OpenDictFile(lpszPathName)) {
            return FALSE;
        }
        AfxGetApp()->WriteProfileString(_T("Settings"),_T("Last Open"), lpszPathName);
        m_pff.SetListingFName(PortableFunctions::PathAppendToPath(PortableFunctions::PathGetDirectory<CString>(lpszPathName), _T("CSSort.lst")));
    }
    else {
        AfxMessageBox(_T("Invalid file type"));
        return FALSE;
    }
    return TRUE;
}


bool CSortDoc::OpenSpecFile(const TCHAR* filename)
{
    try
    {
        auto new_sort_spec = std::make_unique<SortSpec>();
        new_sort_spec->Load(filename, false);

        m_sortSpec = std::move(new_sort_spec);

        ConvertSortItemsSpecToSortDoc();

        return true;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }
}


bool CSortDoc::OpenDictFile(const TCHAR* filename)
{
    try
    {
        std::shared_ptr<const CDataDict> dictionary = CDataDict::InstantiateAndOpen(filename, false);

        m_sortSpec = std::make_shared<SortSpec>();
        m_sortSpec->SetDictionary(dictionary);

        ConvertSortItemsSpecToSortDoc();

        SetTitle(filename);

        return true;
    }

    catch( const CSProException& exception )
    {
		ErrorMessage::Display(exception);
        return false;
    }
}


void CSortDoc::OnUpdateOptionsSortType(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(( m_sortSpec != nullptr ));
}

void CSortDoc::OnOptionsSortType()
{
    CSortTypeDlg dlg(*m_sortSpec);

    if( dlg.DoModal() != IDOK )
        return;

    ConvertSortItemsSpecToSortDoc();

    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != NULL);
    CSortView* pView = (CSortView*) GetNextView(pos);
    pView->OnInitialUpdate();
}



BOOL CSortDoc::SaveModified() {

    // borrowed from CDocument::SaveModified() ; see doccore.cpp

    if (!IsModified()) {
        return TRUE;        // ok to continue
    }

    CString name = m_pff.GetAppFName();
    if (name.IsEmpty()) {
        VERIFY(name.LoadString(AFX_IDS_UNTITLED));
    }
    CString prompt;
    AfxFormatString1(prompt, AFX_IDP_ASK_TO_SAVE, name);
    switch (AfxMessageBox(prompt, MB_YESNOCANCEL, AFX_IDP_ASK_TO_SAVE))
    {
    case IDCANCEL:
        return FALSE;       // don't continue

    case IDYES:
        // If so, either Save or Update, as appropriate
        OnFileSave();
        return m_bRetSave;
        break;

    case IDNO:
        // If not saving changes, revert the document
        break;

    default:
        ASSERT(FALSE);
        break;
    }
    return TRUE;    // keep going
}



void CSortDoc::OnUpdateFileSave(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(( m_aKey.GetSize() > 0 ));
}

void CSortDoc::OnFileSave() {

    if (!IsModified()) {
        m_bRetSave = true;
        return;
    }
    if (m_pff.GetAppFName().IsEmpty()) {
        OnFileSaveAs();
        return;
    }
    SaveSpecFile();
    SetModifiedFlag(FALSE);
    m_bRetSave = true;
}


void CSortDoc::OnUpdateFileSaveAs(CCmdUI* pCmdUI)
{
    pCmdUI->Enable(( m_aKey.GetSize() > 0 ));
}

void CSortDoc::OnFileSaveAs() {
    CString csDictFileName = m_sortSpec->GetDictionary().GetFullFileName();

    CIMSAString csPath = m_pff.GetAppFName();         // BMD 14 Mar 2002
    if (SO::IsBlank(csPath)) {
        csPath = csDictFileName.Left(csDictFileName.ReverseFind('\\')) + _T("\\*.ssf");
    }
    CString csFilter;
//    csFilter.LoadString(IDS_SPEC_FILTER);
    csFilter = _T("Sort Specification Files (*.ssf)|*.ssf|All Files (*.*)|*.*||");
    CIMSAFileDialog dlgFile(FALSE, FileExtensions::SortSpec, csPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, csFilter);
    dlgFile.m_ofn.lpstrTitle = _T("Save Sort Specification File");
    if (dlgFile.DoModal() == IDOK) {
        m_pff.SetAppFName(dlgFile.GetPathName());
        m_pff.SetListingFName(dlgFile.GetPathName() + FileExtensions::WithDot::Listing);
        AfxGetMainWnd()->SetWindowText(CSortView::CreateWindowTitle(m_pff.GetAppFName(), csDictFileName));
        SaveSpecFile();
        SetModifiedFlag(FALSE);
        AfxGetApp()->AddToRecentFileList(dlgFile.GetPathName());

        m_bRetSave = true;
        return;
    }

    m_bRetSave = false;
}


void CSortDoc::SaveSpecFile()
{
    ConvertSortItemsSortDocToSpec();

    try
    {
        m_sortSpec->Save(CS2WS(m_pff.GetAppFName()));
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


CString CSortDoc::GetSpecFileName() const
{
    return PortableFunctions::PathGetFilename(m_pff.GetAppFName());
}


CString CSortDoc::GetDictFileName() const
{
    return ( m_sortSpec != nullptr ) ? PortableFunctions::PathGetFilename(m_sortSpec->GetDictionary().GetFullFileName()) :
                                       CString();
}


void CSortDoc::ConvertSortItemsSpecToSortDoc()
{
    ASSERT(m_sortSpec != nullptr);

    m_aItem.RemoveAll();
    m_aAvail.RemoveAll();
    m_aKey.RemoveAll();

    // add all of the sort items  
    for( const CDictItem* dict_item : m_sortSpec->GetPossibleSortableDictItems() )
    {
        m_aAvail.Add(m_aItem.GetCount());
        m_aItem.Add(SORTITEM { dict_item, SortSpec::SortOrder::Ascending });
    }

    // then modify the used ones
    for( const SortSpec::SortItem& used_sort_item : m_sortSpec->GetSortItems() )
    {
        for( int i = 0; i < m_aAvail.GetCount(); ++i )
        {
            if( m_aItem[m_aAvail[i]].dict_item == used_sort_item.dict_item )
            {
                m_aItem[m_aAvail[i]].order = used_sort_item.order;
                m_aKey.Add(m_aAvail[i]);
                m_aAvail.RemoveAt(i);
                --i;
            }
        }
    }
}


void CSortDoc::ConvertSortItemsSortDocToSpec()
{
    ASSERT(m_sortSpec != nullptr);

    std::vector<std::tuple<int, SortSpec::SortOrder>> sort_item_indices_and_orders;

    for( int i = 0; i < m_aKey.GetSize(); ++i )
        sort_item_indices_and_orders.emplace_back(m_aKey[i], m_aItem[m_aKey[i]].order);

    m_sortSpec->SetSortItems(sort_item_indices_and_orders);
}



void CSortDoc::OnFileRun()
{
    CTwoFileDialog dlg(m_pff, m_sortSpec->GetDictionary().GetFullFileName());

    if( dlg.DoModal() != IDOK )
        return;

    // save the PFF
    if( !m_pff.GetAppFName().IsEmpty() )
    {
        m_pff.SetPifFileName(m_pff.GetAppFName() + FileExtensions::WithDot::Pff);
        m_pff.Save();
    }

    try
    {
        ConvertSortItemsSortDocToSpec();

        switch( Sorter(m_sortSpec).Run(m_pff, false) )
        {
            case Sorter::RunSuccess::Success:
                AfxMessageBox(_T("Sort completed successfully."));
                break;

            case Sorter::RunSuccess::SuccessWithStructuralErrors:
                AfxMessageBox(_T("Sort completed successfully but with structure errors in resulting file."));
                break;

            case Sorter::RunSuccess::Errors:
                AfxMessageBox(_T("Sort had errors."));
                break;
        }
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void CSortDoc::RunBatchSort()
{
    try
    {
        if( m_pff.GetAppType() != SORT_TYPE )
            throw CSProException(_T("PFF file %s was not read correctly. Check the file for parameters invalid to CSSort."), m_pff.GetPifFileName().GetString());

        Sorter().Run(m_pff, true);

        m_pff.ExecuteOnExitPff();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
