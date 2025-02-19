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
    if( !__super::OnOpenDocument(lpszPathName) )
        return FALSE;

    try
    {
        const std::string file_path = TC::ToUtf8(lpszPathName);
        const std::string extension = PortableFunctions::PathGetFileExtension(file_path);

        if( SO::EqualsNoCase(extension, FileExtensions::Pff) )
        {
            m_pff.SetPifFileName(UTF8_TODO::GetCString(file_path));

            if( m_pff.LoadPifFile() )
            {
                RunBatchSort();
                assert_cast<CSortApp*>(AfxGetApp())->m_iReturnCode = 0;
            }

            else
            {
                assert_cast<CSortApp*>(AfxGetApp())->m_iReturnCode = 8;
            }

            return FALSE;
        }

        else if( SO::EqualsNoCase(extension, FileExtensions::SortSpec) )
        {
            if( !OpenSpecFile(file_path) )
                return FALSE;

            AfxGetApp()->WriteProfileString(L"Settings", L"Last Open", lpszPathName);

            m_pff.SetAppFName(UTF8_TODO::GetCString(file_path));
            m_pff.SetListingFName(UTF8_TODO::GetCString(PortableFunctions::PathAppendFileExtension(file_path, FileExtensions::Listing)));

            const std::string pff_file_path = PortableFunctions::PathAppendFileExtension(file_path, FileExtensions::Pff);
            m_pff.SetPifFileName(UTF8_TODO::GetCString(pff_file_path));

            if( PortableFunctions::FileIsRegular(pff_file_path) && m_pff.LoadPifFile() )
            {
                if( !SO::EqualsNoCase(file_path, m_pff.GetAppFName()) )
                    throw CSProException("Spec file in %s\ndoes not match %s", pff_file_path.c_str(), file_path.c_str());
            }
        }

        else if( SO::EqualsNoCase(extension, FileExtensions::Dictionary) )
        {
            m_pff.SetAppFName(CString());

            if( !OpenDictionary(file_path) )
                return FALSE;

            AfxGetApp()->WriteProfileString(L"Settings", L"Last Open", lpszPathName);

            m_pff.SetListingFName(UTF8_TODO::GetCString(PortableFunctions::PathReplaceFilename(file_path, "CSSort.lst")));
        }

        else
        {
            throw CSProException("Invalid file type.");
        }

        return TRUE;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }
}


bool CSortDoc::OpenSpecFile(const std::string& file_path)
{
    try
    {
        auto new_sort_spec = std::make_unique<SortSpec>();
        new_sort_spec->Load(file_path, false);

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


bool CSortDoc::OpenDictionary(const std::string& file_path)
{
    try
    {
        std::unique_ptr<const CDataDict> dictionary = CDataDict::InstantiateAndOpen(file_path, false);

        m_sortSpec = std::make_unique<SortSpec>();
        m_sortSpec->SetDictionary(std::move(dictionary));

        ConvertSortItemsSpecToSortDoc();

        SetTitle(TC::ToWide(file_path).c_str());

        return true;
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return false;
    }
}


void CSortDoc::OnUpdateOptionsSortType(CCmdUI* const pCmdUI)
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


BOOL CSortDoc::SaveModified()
{
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



void CSortDoc::OnUpdateFileSave(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(( m_aKey.GetSize() > 0 ));
}


void CSortDoc::OnFileSave()
{
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


void CSortDoc::OnUpdateFileSaveAs(CCmdUI* const pCmdUI)
{
    pCmdUI->Enable(( m_aKey.GetSize() > 0 ));
}


void CSortDoc::OnFileSaveAs()
{
    std::string file_path = UTF8_TODO::GetUtf8(m_pff.GetAppFName());

    // if no spec file path exists, base it on the dictionary's file path
    if( file_path.empty() )
        file_path = PortableFunctions::PathReplaceFileExtension(m_sortSpec->GetDictionary().GetFilePath(), FileExtensions::SortSpec);

    SaveFileDlg save_file_dlg(0, FileExtensions::SortSpec, file_path, L"Sort Specification Files (*.ssf)|*.ssf|All Files (*.*)|*.*||");
    save_file_dlg.SetTitle(L"Save Sort Specification File");

    if( save_file_dlg.DoModal() == IDOK )
    {
        m_pff.SetAppFName(UTF8_TODO::GetCString(save_file_dlg.GetFilePath()));
        m_pff.SetListingFName(UTF8_TODO::GetCString(PortableFunctions::PathAppendFileExtension(save_file_dlg.GetFilePath(), FileExtensions::Listing)));
        AfxGetMainWnd()->SetWindowText(TC::ToWide(CSortView::CreateWindowTitle(UTF8_TODO::GetUtf8(m_pff.GetAppFName()), m_sortSpec->GetDictionary().GetFilePath())).c_str());
        SaveSpecFile();
        SetModifiedFlag(FALSE);
        AfxGetApp()->AddToRecentFileList(TC::ToWide(save_file_dlg.GetFilePath()).c_str());
        m_bRetSave = true;
    }

    else
    {
        m_bRetSave = false;
    }

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


const std::string& CSortDoc::GetSpecFilePath() const
{
    return UTF8_TODO::Create_Reference(m_pff.GetAppFName());
}


const std::string& CSortDoc::GetDictionaryFilePath() const
{
    return ( m_sortSpec != nullptr ) ? m_sortSpec->GetDictionary().GetFilePath() :
                                       SO::Empty_string;
}


void CSortDoc::ConvertSortItemsSpecToSortDoc()
{
    ASSERT(m_sortSpec != nullptr);

    m_aItem.RemoveAll();
    m_aAvail.RemoveAll();
    m_aKey.RemoveAll();

    // add all of the sort items
    for( const CDictItem* const dict_item : m_sortSpec->GetPossibleSortableDictItems() )
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
    CTwoFileDialog dlg(m_pff, m_sortSpec->GetDictionary().GetFilePath());

    if( dlg.DoModal() != IDOK )
        return;

    // save the PFF
    if( !m_pff.GetAppFName().IsEmpty() )
    {
        m_pff.SetPifFileName(UTF8_TODO::GetCString(PortableFunctions::PathAppendFileExtension(UTF8_TODO::GetUtf8(m_pff.GetAppFName()), FileExtensions::Pff)));
        m_pff.Save();
    }

    try
    {
        ConvertSortItemsSortDocToSpec();

        const Sorter::RunSuccess run_success = Sorter(m_sortSpec).Run(m_pff, false);

        switch( run_success )
        {
            case Sorter::RunSuccess::Success:
                AfxMessageBox(L"Sort completed successfully.");
                break;

            case Sorter::RunSuccess::SuccessWithStructuralErrors:
                AfxMessageBox(L"Sort completed successfully but with structure errors in resulting data source.");
                break;

            case Sorter::RunSuccess::Errors:
                AfxMessageBox(L"Sort had errors.");
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
        {
            throw CSProException("PFF file %s was not read correctly. Check the file for parameters invalid to CSSort.",
                                 UTF8_TODO::GetUtf8(m_pff.GetPifFileName()).c_str());
        }

        Sorter().Run(m_pff, true);

        m_pff.ExecuteOnExitPff();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
