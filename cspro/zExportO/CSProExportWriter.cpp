#include "stdafx.h"
#include "CSProExportWriter.h"
#include <zDataO/DictionarySource.h>


CSProExportWriter::CSProExportWriter(std::shared_ptr<const CaseAccess> case_access, const ConnectionString& connection_string)
    :   m_caseAccess(std::move(case_access))
{
    // if only writing a single record, create a special dictionary and case access for that record
    const std::string* const single_record_to_export_name = connection_string.GetProperty(CSProperty::record);

    if( single_record_to_export_name != nullptr )
        SetupSingleRecordCaseAccess(*single_record_to_export_name);


    // open the data repository
    const ConnectionString modified_connection_string = GetDataConnectionString(connection_string);

    if( DataRepositoryHelpers::IsTypeExportWriter(modified_connection_string.GetType()) )
        throw CSProException("You cannot export to CSPro format when specifying an export-style type.");

    m_dataRepository = DataRepository::CreateAndOpen(m_caseAccess, modified_connection_string,
                                                     DataRepositoryAccess::BatchOutput, DataRepositoryOpenFlag::CreateNew);


    // store the dictionary file path (to be written in Close)
    m_dictionaryFilePath = GetDictionaryFilePath(modified_connection_string);
    ASSERT(m_dictionaryFilePath == GetDictionaryFilePath(connection_string));
}


CSProExportWriter::~CSProExportWriter()
{
    Close();
}


ConnectionString CSProExportWriter::GetDataConnectionString(const ConnectionString& connection_string)
{
    // construct a connection string with the CSProExport type property removed
    ConnectionString modified_connection_string(connection_string.GetFilePath());

    // add any properties from the original connection string
    for( const auto& [attribute, value] : connection_string.GetProperties() )
        modified_connection_string.SetProperty(attribute, value);

    return modified_connection_string;
}


std::string CSProExportWriter::GetDictionaryFilePath(const ConnectionString& connection_string)
{
    std::string dictionary_file_path = DictionarySource::GetDictionaryPathOverride(connection_string);

    if( !dictionary_file_path.empty() )
        return dictionary_file_path;

    const std::string* const pre80_dictionary_path_override = connection_string.GetProperty("dictionary");

    if( pre80_dictionary_path_override != nullptr )
        return MakeFullPath(GetWorkingDirectory(connection_string.GetFilePath()), *pre80_dictionary_path_override);

    return PortableFunctions::PathAppendFileExtension(connection_string.GetFilePath(), FileExtensions::Dictionary);
}


void CSProExportWriter::Close()
{
    if( m_dataRepository == nullptr )
        return;

    try
    {
        m_dataRepository->Close();

        // save a copy of the dictionary
        m_caseAccess->GetDataDict().Save(m_dictionaryFilePath, false);
    }

    catch( const CSProException& exception )
    {
        if( m_caseAccess->GetCaseConstructionReporter() != nullptr )
            m_caseAccess->GetCaseConstructionReporter()->IssueMessage(MessageType::Error, 10105, exception.what());
    }

    m_dataRepository.reset();
}


void CSProExportWriter::WriteCase(const Case& data_case)
{
    if( m_singleRecordCaseDetails != nullptr )
    {
        WriteSingleRecordCase(data_case);
    }

    else
    {
        m_dataRepository->WriteCase(const_cast<Case&>(data_case));
    }
}


void CSProExportWriter::SetupSingleRecordCaseAccess(const std::string& record_name)
{
    // if only writing a single record, find it and then create dictionary/CaseAccess objects for only that record
    const CaseMetadata& source_case_metadata = m_caseAccess->GetCaseMetadata();
    const CDataDict& source_dictionary = source_case_metadata.GetDictionary();

    const CaseRecordMetadata* source_case_record_metadata = source_case_metadata.FindCaseRecordMetadata(record_name);

    if( source_case_record_metadata == nullptr )
    {
        throw CSProException("'%s' is not a record in the dictionary '%s'",
                             record_name.c_str(), source_dictionary.GetName().c_str());
    }

    if( !MultipleLevelExportSupported && source_case_record_metadata->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber() > 0 )
    {
        // this exporter certainly could be setup to export records from levels 2+, but it just has not been implemented
        throw CSProException("The CSPro Export format cannot export only '%s' because it is not on the dictionary's first level",
                             record_name.c_str());
    }

    // if the source dictionary only contains this record, then we do not need need to create a
    // special single record dictionary
    if( source_dictionary.GetNumRecords() == 1 )
        return;

    m_singleRecordCaseDetails = std::make_unique<SingleRecordCaseDetails>(SingleRecordCaseDetails
        {
            source_case_record_metadata->GetRecordIndex(),
            source_dictionary,
            nullptr,
            nullptr,
            nullptr
        });

    // remove all levels but the first
    while( m_singleRecordCaseDetails->dictionary.GetNumLevels() > 1 )
        m_singleRecordCaseDetails->dictionary.RemoveLevel(1);

    // remove all records but the one to export
    DictLevel& dict_level = m_singleRecordCaseDetails->dictionary.GetLevel(0);

    for( int r = dict_level.GetNumRecords() - 1; r >= 0; --r )
    {
        if( !SO::EqualsNoCase(dict_level.GetRecord(r)->GetName(), record_name) )
            dict_level.RemoveRecordAt(r);
    }

    ASSERT(dict_level.GetNumRecords() == 1);

    m_singleRecordCaseDetails->dictionary.UpdatePointers();

    // modify the case access to this single record dictionary
    std::shared_ptr<CaseConstructionReporter> source_case_construction_reporter = m_caseAccess->GetSharedCaseConstructionReporter();
    m_caseAccess = CaseAccess::CreateAndInitializeFullCaseAccess(m_singleRecordCaseDetails->dictionary);

    // create a case
    m_singleRecordCaseDetails->data_case = m_caseAccess->CreateCase();
    m_singleRecordCaseDetails->data_case->SetCaseConstructionReporter(std::move(source_case_construction_reporter));

    m_singleRecordCaseDetails->id_case_record = &m_singleRecordCaseDetails->data_case->GetRootCaseLevel().GetIdCaseRecord();
    m_singleRecordCaseDetails->single_record_case_record = &m_singleRecordCaseDetails->data_case->GetRootCaseLevel().GetCaseRecord(0);

    ASSERT(SO::EqualsNoCase(record_name, m_singleRecordCaseDetails->single_record_case_record->GetCaseRecordMetadata().GetDictRecord().GetName()));
}


void CSProExportWriter::WriteSingleRecordCase(const Case& data_case)
{
    ASSERT(m_singleRecordCaseDetails != nullptr);

    const CaseLevel& source_root_case_level = data_case.GetRootCaseLevel();
    const CaseRecord& source_case_record = source_root_case_level.GetCaseRecord(m_singleRecordCaseDetails->source_case_record_number);

    // quit if there are no occurrences of this record
    if( source_case_record.GetNumberOccurrences() == 0 )
        return;

    // otherwise copy the ID...
    m_singleRecordCaseDetails->data_case->Reset();

    m_singleRecordCaseDetails->id_case_record->CopyValues(source_root_case_level.GetIdCaseRecord(), 0);

    // ...and then each of the occurrences
    m_singleRecordCaseDetails->single_record_case_record->SetNumberOccurrences(source_case_record.GetNumberOccurrences());

    for( size_t record_occurrence = 0; record_occurrence < source_case_record.GetNumberOccurrences(); ++record_occurrence )
        m_singleRecordCaseDetails->single_record_case_record->CopyValues(source_case_record, record_occurrence);

    m_dataRepository->WriteCase(*m_singleRecordCaseDetails->data_case);
}
