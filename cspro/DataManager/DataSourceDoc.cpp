#include "StdAfx.h"
#include "DataSourceDoc.h"
#include "CaseListingView.h"
#include "DataSourceFrame.h"
#include "DataSourceSettings.h"
#include <zInterfaceF/DictionaryReconcileDlg.h>


IMPLEMENT_DYNCREATE(DataSourceDoc, CDocument)

BEGIN_MESSAGE_MAP(DataSourceDoc, CDocument)
END_MESSAGE_MAP()


DataSourceDoc::DataSourceDoc()
{
}


DataSourceDoc::~DataSourceDoc()
{
}


DataSourceFrame& DataSourceDoc::GetFrame()
{
    return *assert_cast<DataSourceFrame*>(GetCaseListingView().GetParentFrame());
}


CaseListingView& DataSourceDoc::GetCaseListingView()
{
    POSITION pos = GetFirstViewPosition();
    ASSERT(pos != nullptr);

    return assert_cast<CaseListingView&>(*GetNextView(pos));
}


UINT DataSourceDoc::GetDefaultPageCommandId()
{
    // always show the data summary as the initial page
    return ID_VIEW_DATA_SUMMARY;
}


void DataSourceDoc::SetPathName(LPCTSTR /*lpszPathName*/, const BOOL bAddToMRU/* = TRUE*/)
{
    // because lpszPathName is a ConnectionString and can have non-path characters,
    // we override this method, called from CMultiDocTemplate::OpenDocumentFile,
    // to set a valid file path
    ASSERT(m_connectionString.IsDefined());

    if( m_connectionString.HasFilePath() )
        __super::SetPathName(TC::ToWide(m_connectionString.GetFilePath()).c_str(), bAddToMRU);

    SetTitle(TC::ToWide(FormatText("%s (%s)", m_connectionString.GetName(DataRepositoryNameType::Concise).c_str(),
                                              m_dictionary->GetName().c_str())).c_str());
}


BOOL DataSourceDoc::OnOpenDocument(LPCTSTR lpszPathName)
{
    CMainFrame* const main_frame = assert_cast<CMainFrame*>(AfxGetMainWnd());

    m_connectionString = main_frame->GetConnectionStringFileSimulator().GetConnectionString(lpszPathName);

    m_dataSourceSettings = main_frame->GetSettings().GetSettings<DataSourceSettings>(m_connectionString);

    try
    {
        if( m_connectionString.HasFilePath() &&
            !PortableFunctions::FileIsRegular(m_connectionString.GetFilePath()) )
        {
            throw FileIO::Exception::FileNotFound(m_connectionString.GetFilePath());
        }

        if( !LoadDictionary() )
            return FALSE;

        ASSERT(m_dictionarySource != nullptr && m_dictionary != nullptr);

        // when a data source is opened without using the embedded dictionary, confirm that the dictionary matches
        if( !m_dictionarySource->UsingEmbeddedDictionary() &&
            !DictionaryReconcileDlg::DictionaryChangesIfAnyAreOk(m_connectionString, *m_dictionary) )
        {
            return FALSE;
        }

        // restore the previously selected language setting
        const std::optional<size_t> language_index =
            !m_dataSourceSettings->GetLanguageName().empty() ? m_dictionary->IsLanguageDefined(m_dataSourceSettings->GetLanguageName()) :
                                                               std::nullopt;

        if( language_index > size_t(0) )
            m_dictionary->SetCurrentLanguage(*language_index);

        // set the case access and open the data repository
        m_caseAccess = CaseAccess::CreateAndInitializeFullCaseAccess(*m_dictionary);

        m_dataRepository = DataRepository::CreateAndOpen(m_caseAccess, m_connectionString, DataRepositoryAccess::ReadOnly,
                                                                                           DataRepositoryOpenFlag::OpenMustExist);

        return RunPostOpenTasks();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
        return FALSE;
    }
}


bool DataSourceDoc::LoadDictionary()
{
    ASSERT(m_dictionary == nullptr && m_dataSourceSettings != nullptr && m_dictionarySource == nullptr);
    std::optional<bool> load_success;

    auto load = [&](std::unique_ptr<DictionarySource> dictionary_source)
    {
        try
        {
            m_dictionary = dictionary_source->GetAssociatedDictionary();

            if( m_dictionary != nullptr )
            {
                m_dictionarySource = std::move(dictionary_source);

                // save this dictionary information for the next time the data file is opened
                if( !m_dictionarySource->UsingEmbeddedDictionary() )
                    m_dataSourceSettings->SetDictionaryFilePath(m_dictionarySource->GetDictionaryFilePath());

                load_success = true;
            }
        }

        catch( const DataRepositoryException::EncryptionError& )
        {
            // when trying to open an Encrypted CSPro DB file, don't query about a dictionary if
            // the embedded one couldn't be opened (because the user didn't know the password)
            load_success = false;
        }

        return load_success.has_value();
    };

    // first try to load the dictionary using an associated or embedded dictionary
    if( load(std::make_unique<DictionarySource>(m_connectionString)) )
        return *load_success;

    // use a previously associated dictionary when possible
    if( m_dataSourceSettings->HasUsableDictionaryFilePath() )
    {
        const std::string message = FormatText("To open '%s' you must specify a dictionary. "
                                               "Do you want to use the dictionary '%s' that was previously associated with this data source?",
                                               m_connectionString.GetName(DataRepositoryNameType::Concise).c_str(),
                                               PortableFunctions::PathGetFilename(m_dataSourceSettings->GetDictionaryFilePath()).c_str());

        const int response = AfxMessageBox(message, MB_YESNOCANCEL);

        if( response == IDCANCEL )
        {
            return false;
        }

        else if( response == IDYES )
        {
            load(std::make_unique<DictionarySource>(m_dataSourceSettings->GetDictionaryFilePath()));
            return load_success.value_or(false);
        }
    }

    // query for a dictionary to use
    OpenFileDlg open_file_dlg(0, nullptr, nullptr, FileFilters::Dictionary);
    open_file_dlg.SetTitle("Select the dictionary that describes " + m_connectionString.GetName(DataRepositoryNameType::Concise));

    if( m_connectionString.HasFilePath() )
        open_file_dlg.SetInitialDirectory(PortableFunctions::PathGetDirectory(m_connectionString.GetFilePath()));

    if( open_file_dlg.DoModal() == IDOK )
    {
        load(std::make_unique<DictionarySource>(open_file_dlg.GetFilePath()));
        return load_success.value_or(false);
    }

    return false;
}


void DataSourceDoc::OnCloseDocument()
{
    try
    {
        if( m_dataRepository != nullptr )
            m_dataRepository->Close();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }

    __super::OnCloseDocument();
}
