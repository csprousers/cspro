#include "Stdafx.h"
#include "DataRepository.h"
#include "DataViewerSettings.h"
#include <zDataO/ISyncableDataRepository.h>
#include <zDataO/SyncHistoryEntry.h>


CSPro::Data::DataRepository::DataRepository()
    :   m_caseAccess(new std::shared_ptr<CaseAccess>),
        m_pNativeRepo(nullptr),
        m_dictionary(nullptr)
{
}


void CSPro::Data::DataRepository::CreateAndOpen(CSPro::Dictionary::DataDictionary^ dictionary,
                                                CSPro::Util::ConnectionString^ connection_string,
                                                DataRepositoryAccess eAccess, DataRepositoryOpenFlag eOpenFlag)
{
    ASSERT(dictionary != nullptr);

    try
    {
        m_dictionary = dictionary;
        *m_caseAccess = CaseAccess::CreateAndInitializeFullCaseAccess(*m_dictionary->GetNativePointer());
        m_pNativeRepo = ::DataRepository::CreateAndOpen(*m_caseAccess, connection_string->GetNativeConnectionString(), eAccess, eOpenFlag).release();
    }

    catch( const DataRepositoryException::Error& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
    }
}


void CSPro::Data::DataRepository::OpenForReading(CSPro::Dictionary::DataDictionary^ dictionary, CSPro::Util::ConnectionString^ connection_string)
{
    CreateAndOpen(dictionary, connection_string, DataRepositoryAccess::ReadOnly, DataRepositoryOpenFlag::OpenMustExist);
}


void CSPro::Data::DataRepository::OpenForReadingAndWriting(CSPro::Dictionary::DataDictionary^ dictionary, CSPro::Util::ConnectionString^ connection_string)
{
    CreateAndOpen(dictionary, connection_string, DataRepositoryAccess::ReadWrite, DataRepositoryOpenFlag::OpenMustExist);
}


void CSPro::Data::DataRepository::CreateForReadingAndWriting(CSPro::Dictionary::DataDictionary^ dictionary, CSPro::Util::ConnectionString^ connection_string)
{
    CreateAndOpen(dictionary, connection_string, DataRepositoryAccess::ReadWrite, DataRepositoryOpenFlag::CreateNew);
}


void CSPro::Data::DataRepository::Close()
{
    m_pNativeRepo->Close();
}


System::String^ CSPro::Data::DataRepository::ConciseName::get()
{
    return gcnew System::String(m_pNativeRepo->GetName(DataRepositoryNameType::Concise));
}


int CSPro::Data::DataRepository::NumberCases::get()
{
    return m_pNativeRepo->GetNumberCases();
}


CSPro::Dictionary::DataDictionary^ CSPro::Data::DataRepository::Dictionary::get()
{
    return m_dictionary;
}


bool CSPro::Data::DataRepository::Syncable::get()
{
    return ( m_pNativeRepo->GetSyncableDataRepository() != nullptr );
}


DataRepository* CSPro::Data::DataRepository::GetNativePointer()
{
    return m_pNativeRepo;
}


namespace
{
    std::unique_ptr<CaseIteratorParameters> GetStartParameters(CSPro::Data::DataViewerSettings^ data_viewer_settings)
    {
        if( data_viewer_settings->KeyPrefix == nullptr )
        {
            return nullptr;
        }

        else
        {
            return std::make_unique<CaseIteratorParameters>(CaseIterationStartType::GreaterThanEquals, CString(), CString(data_viewer_settings->KeyPrefix));
        }
    }
}


int CSPro::Data::DataRepository::GetNumberCases(DataViewerSettings^ data_viewer_settings)
{
    std::unique_ptr<CaseIteratorParameters> start_parameters = GetStartParameters(data_viewer_settings);
    return m_pNativeRepo->GetNumberCases(data_viewer_settings->GetCaseIterationCaseStatus(), start_parameters.get());
}


System::Collections::Generic::List<CSPro::Data::CaseSummary>^ CSPro::Data::DataRepository::GetCaseSummaries(
    DataViewerSettings^ data_viewer_settings, int start_index, int number_case_summaries)
{
    auto case_summaries = gcnew System::Collections::Generic::List<CSPro::Data::CaseSummary>();

    std::unique_ptr<CaseIteratorParameters> start_parameters = GetStartParameters(data_viewer_settings);

    auto case_summary_iterator = m_pNativeRepo->CreateIterator(CaseIterationContent::CaseSummary,
        data_viewer_settings->GetCaseIterationCaseStatus(), data_viewer_settings->GetCaseIterationMethod(),
        data_viewer_settings->GetCaseIterationOrder(), start_parameters.get(),
        start_index, number_case_summaries);

    ::CaseSummary native_case_summary;

    while( case_summary_iterator->NextCaseSummary(native_case_summary) )
    {
        CSPro::Data::CaseSummary case_summary;

        case_summary.Key = gcnew System::String(native_case_summary.GetKey());
        case_summary.PositionInRepository = native_case_summary.GetPositionInRepository();
        case_summary.CaseLabel = gcnew System::String(native_case_summary.GetCaseLabelOrKey());
        case_summary.Deleted = native_case_summary.GetDeleted();
        case_summary.Verified = native_case_summary.GetVerified();
        case_summary.PartialSaveMode = (CSPro::Data::PartialSaveMode)native_case_summary.GetPartialSaveMode();

        case_summaries->Add(case_summary);
    }

    return case_summaries;
}


array<CSPro::Data::SyncHistoryEntry^>^ CSPro::Data::DataRepository::GetSyncHistory()
{
    ISyncableDataRepository* pSyncableRepo = m_pNativeRepo->GetSyncableDataRepository();
    if (pSyncableRepo == nullptr)
        return nullptr;
    auto nativeHistory = pSyncableRepo->GetSyncHistory();
    auto history = gcnew array<CSPro::Data::SyncHistoryEntry^>(nativeHistory.size());
    for (size_t i = 0; i < nativeHistory.size(); ++i) {
        history[i] = gcnew CSPro::Data::SyncHistoryEntry(nativeHistory[i]);
    }
    return history;
}


CSPro::Dictionary::DataDictionary^ CSPro::Data::DataRepository::GetEmbeddedDictionary(CSPro::Util::ConnectionString^ connection_string)
{
    std::unique_ptr<CDataDict> dictionary = DataRepositoryHelpers::GetEmbeddedDictionary(connection_string->GetNativeConnectionString());

    if( dictionary != nullptr )
        return gcnew CSPro::Dictionary::DataDictionary(dictionary.release(), true);

    return nullptr;
}
