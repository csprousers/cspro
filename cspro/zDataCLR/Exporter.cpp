#include "Stdafx.h"
#include "Exporter.h"
#include "DataViewerCaseConstructionReporter.h"
#include <zExportO/ExportDefinitions.h>


CSPro::Data::Exporter::Exporter(CSPro::Data::DataRepository^ repository)
    :   m_nativeRepository(repository->GetNativePointer()),
        m_recordNames(new std::vector<std::wstring>),
        m_exportConnectionStrings(new std::vector<::ConnectionString>)
{
    // populate the record names
    const CDataDict& dictionary = m_nativeRepository->GetCaseAccess()->GetDataDict();

    for( const DictLevel& dict_level : dictionary.GetLevels() )
    {
        for( int r = 0; r < dict_level.GetNumRecords(); ++r )
            m_recordNames->emplace_back(CS2WS(dict_level.GetRecord(r)->GetName()));
    }
}


CSPro::Data::Exporter::!Exporter()
{
    delete m_recordNames;
    delete m_exportConnectionStrings;
}


void CSPro::Data::Exporter::Refresh(System::String^ base_filename, System::Collections::Generic::ICollection<ExporterExportType>^ exporter_export_types, const bool one_file_per_record)
{
    m_exportConnectionStrings->clear();

    const std::wstring wstring_base_filename = ToWS(base_filename);

    if( exporter_export_types->Contains(ExporterExportType::CommaDelimited) )
        AddExportConnectionString(wstring_base_filename, DataRepositoryType::CommaDelimited, one_file_per_record);

    if( exporter_export_types->Contains(ExporterExportType::Excel) )
        AddExportConnectionString(wstring_base_filename, DataRepositoryType::Excel, one_file_per_record);

    if( exporter_export_types->Contains(ExporterExportType::Json) )
        AddExportConnectionString(wstring_base_filename, DataRepositoryType::Json, one_file_per_record);

    if( exporter_export_types->Contains(ExporterExportType::R) )
        AddExportConnectionString(wstring_base_filename, DataRepositoryType::R, one_file_per_record);

    if( exporter_export_types->Contains(ExporterExportType::SAS) )
        AddExportConnectionString(wstring_base_filename, DataRepositoryType::SAS, one_file_per_record);

    if( exporter_export_types->Contains(ExporterExportType::SPSS) )
        AddExportConnectionString(wstring_base_filename, DataRepositoryType::SPSS, one_file_per_record);

    if( exporter_export_types->Contains(ExporterExportType::Stata) )
        AddExportConnectionString(wstring_base_filename, DataRepositoryType::Stata, one_file_per_record);
}


void CSPro::Data::Exporter::AddExportConnectionString(const std::wstring& base_filename, const DataRepositoryType data_repository_type, bool one_file_per_record)
{
    const TCHAR* export_extension;
    std::wstring data_repository_type_override;

    if( DataRepositoryHelpers::IsTypeExportWriter(data_repository_type) )
    {
        export_extension = ExportTypeDefaultExtension(data_repository_type);

        // if the export type doesn't support multiple records, then we must use one file per record
        if( !one_file_per_record && !ExportTypeSupportsMultipleRecords(data_repository_type) )
            one_file_per_record = true;
    }

    // if writing to a non-export writer, make sure that we write it as using CSProExportWriter
    else
    {
        data_repository_type_override = FormatTextCS2WS(_T("%s=%s"), ConnectionStringDataRepositoryPropertyType,
                                                                     DataRepositoryTypeNames[static_cast<size_t>(DataRepositoryType::CSProExport)]);

        export_extension = DataRepositoryTypeDefaultExtensions[static_cast<size_t>(data_repository_type)];
    }

    // when outputting multiple records, the record name is added to the filename
    if( one_file_per_record && m_recordNames->size() > 1 )
    {
        if( !data_repository_type_override.empty() )
            data_repository_type_override.insert(0, 1, '&');

        for( const std::wstring& record_name : *m_recordNames )
        {
            m_exportConnectionStrings->emplace_back(FormatTextCS2WS(_T("%s-%s.%s|%s=%s%s"),
                                                                    base_filename.c_str(), record_name.c_str(), export_extension,
                                                                    CSProperty::record, record_name.c_str(),
                                                                    data_repository_type_override.c_str()));
        }
    }

    else
    {
        if( !data_repository_type_override.empty() )
            data_repository_type_override.insert(0, 1, '|');

        m_exportConnectionStrings->emplace_back(FormatTextCS2WS(_T("%s.%s%s"),
                                                                base_filename.c_str(), export_extension,
                                                                data_repository_type_override.c_str()));
    }
}


System::String^ CSPro::Data::Exporter::GetOutputFilenames()
{
    std::wstring output_filenames;

    for( const ::ConnectionString& connection_string : *m_exportConnectionStrings )
    {
        for( const std::wstring& filename : GetExportFilenames(connection_string) )
            SO::AppendWithSeparator(output_filenames, filename, _T("\r\n"));
    }
    
    return FromWS(output_filenames);
}


System::String^ CSPro::Data::Exporter::Export(System::ComponentModel::BackgroundWorker^ background_worker)
{
    ASSERT(!m_exportConnectionStrings->empty());

    // code heavily borrowed from RepositoryConverter's Convert
    try
    {
        const std::shared_ptr<const CaseAccess> case_access = m_nativeRepository->GetSharedCaseAccess();

        // use a case construction reporter to keep track of any errors on conversion
        auto case_construction_reporter = std::make_shared<DataViewerCaseConstructionReporter>();
        const_cast<CaseAccess&>(*case_access).SetCaseConstructionReporter(case_construction_reporter);

        const std::unique_ptr<Case> data_case = case_access->CreateCase(true);

        // open the export data repositories
        std::vector<std::unique_ptr<::DataRepository>> export_data_repositories;

        for( const ConnectionString& connection_string : *m_exportConnectionStrings )
        {
            // don't export on top of the existing data source (which could occur with JSON data sources)
            if( m_nativeRepository->GetConnectionString().FilenameMatches(connection_string.GetFilename()) )
                continue;

            export_data_repositories.emplace_back(::DataRepository::CreateAndOpen(case_access,
                                                                                  connection_string,
                                                                                  DataRepositoryAccess::BatchOutput,
                                                                                  DataRepositoryOpenFlag::CreateNew));
        }

        // run the export
        background_worker->ReportProgress(-1, System::String::Format("Exporting {0}",
                                                                     FromWS(m_nativeRepository->GetName(DataRepositoryNameType::Concise))));

        const size_t total_cases = m_nativeRepository->GetNumberCases();
        size_t exported_cases = 0;

        constexpr size_t ProgressUpdateFrequency = 250;
        size_t cases_until_progress_update = ProgressUpdateFrequency;
        const double percent_multiplier = 100.0 / std::max(total_cases, static_cast<size_t>(1));

        const std::unique_ptr<CaseIterator> case_iterator = m_nativeRepository->CreateCaseIterator(::CaseIterationMethod::SequentialOrder,
                                                                                                   ::CaseIterationOrder::Ascending);

        while( case_iterator->NextCase(*data_case) )
        {
            for( auto& export_data_repository : export_data_repositories )
                export_data_repository->WriteCase(*data_case);

            ++exported_cases;

            if( --cases_until_progress_update == 0 )
            {
                // update progress bar
                background_worker->ReportProgress(static_cast<int>(exported_cases * percent_multiplier),
                                                  System::String::Format("Case {0} of {1} [{2}]",
                                                                         exported_cases,
                                                                         total_cases,
                                                                         FromWS(data_case->GetKey())));

                cases_until_progress_update = ProgressUpdateFrequency;
            }

            // if the operation is cancelled, delete the exported data
            if( background_worker->CancellationPending )
            {
                for( const std::unique_ptr<::DataRepository>& export_data_repository : export_data_repositories )
                    export_data_repository->DeleteRepository();

                return nullptr;
            }
        }

        // finalize the export
        for( const std::unique_ptr<::DataRepository>& export_data_repository : export_data_repositories )
            export_data_repository->Close();

        export_data_repositories.clear();

        background_worker->ReportProgress(100, gcnew System::String("Export complete"));

        std::wstring message = FormatTextCS2WS(_T("Successfully exported %d cases."), static_cast<int>(exported_cases));

        if( !case_construction_reporter->GetErrors().empty() )
        {
            message.append(_T("\n\nThere were errors during the conversion:"));

            for( const std::wstring& error : case_construction_reporter->GetErrors() )
            {
                message.append(_T("\n\n• "));
                message.append(error);
            }
        }

        return FromWS(message);
    }

    catch( const DataRepositoryException::Error& exception )
    {
        return FromWS(exception.GetErrorMessage());
    }
}
