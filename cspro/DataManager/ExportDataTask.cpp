#include "StdAfx.h"
#include "ExportDataTask.h"
#include "ExportDataSettings.h"
#include "TaskRunner.h"


CREATE_JSON_KEY(formats)
CREATE_JSON_KEY(oneFilePerRecord)


// --------------------------------------------------------------------------
// ExportDataSettings
// --------------------------------------------------------------------------

ExportDataSettings ExportDataSettings::CreateFromJson(const JsonNode& json_node)
{
    return ExportDataSettings
    {
        json_node.GetArray(JK::formats).GetSet<DataRepositoryType>(),
        json_node.GetAbsolutePath(JK::path),
        json_node.Get<bool>(JK::oneFilePerRecord)
    };
}


void ExportDataSettings::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::formats, export_formats)
               .WritePath(JK::path, base_file_path)
               .Write(JK::oneFilePerRecord, one_file_per_record)
               .EndObject();
}



// --------------------------------------------------------------------------
// ExportDataTask
// --------------------------------------------------------------------------

ExportDataTask::ExportDataTask(std::vector<ConnectionString> export_connection_strings)
    :   m_exportConnectionStrings(std::move(export_connection_strings))
{
    ASSERT(!m_exportConnectionStrings.empty());
}


void ExportDataTask::Initialize()
{
    m_taskRunner->SetTitle("Exporting data...");

    m_taskRunner->LogText("Exporting cases to files:");

    for( const ConnectionString& connection_string : m_exportConnectionStrings )
    {
        m_taskRunner->LogText("   " + connection_string.GetName(DataRepositoryNameType::Full));

        m_exportDataRepositories.emplace_back(DataRepository::CreateAndOpen(m_caseAccess,
                                                                            connection_string,
                                                                            DataRepositoryAccess::BatchOutput,
                                                                            DataRepositoryOpenFlag::CreateNew));
    }

    m_taskRunner->LogText();
}


void ExportDataTask::ProcessCase(Case& data_case)
{
    for( const std::unique_ptr<DataRepository>& data_repository : m_exportDataRepositories )
        data_repository->WriteCase(data_case);
}


void ExportDataTask::Finalize(const Result result)
{
    if( result == Result::Complete )
    {
        const int cases_processed = GetCasesProcessed();
        const int files_created = m_exportDataRepositories.size();

        m_taskRunner->LogText("Process summary:");
        m_taskRunner->LogText("    Cases processed: %d", cases_processed);

        m_taskRunner->LogText();
        m_taskRunner->LogText("Successfully exported %d case%s to %d file%s.", cases_processed, PluralizeWord(cases_processed),
                                                                               files_created, PluralizeWord(files_created));
    }

    else
    {
        m_taskRunner->LogText();

        // delete any partially saved files
        for( const std::unique_ptr<DataRepository>& data_repository : m_exportDataRepositories )
        {
            m_taskRunner->LogText("Deleting the file: " + data_repository->GetName(DataRepositoryNameType::Full));
            data_repository->DeleteRepository();
        }
    }

    m_exportDataRepositories.clear();
}
