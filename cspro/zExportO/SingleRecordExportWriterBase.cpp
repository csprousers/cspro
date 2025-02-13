#include "stdafx.h"
#include "SingleRecordExportWriterBase.h"


SingleRecordExportWriterBase::SingleRecordExportWriterBase(const DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access,
                                                           const ConnectionString& connection_string)
    :   ExportWriterBase(type, std::move(case_access), connection_string),
        m_singleExportRecordMapping(nullptr)
{
}


void SingleRecordExportWriterBase::CreateExportRecordMappings()
{
    ExportWriterBase::CreateExportRecordMappings();

    // find the single mapping that will be used
    for( std::vector<ExportRecordMapping>& export_record_mapping_for_level : m_exportRecordMappingByLevel )
    {
        for( ExportRecordMapping& export_record_mapping : export_record_mapping_for_level )
        {
            if( !export_record_mapping.item_mappings.empty() )
            {
                if( m_singleExportRecordMapping == nullptr )
                {
                    m_singleExportRecordMapping = &export_record_mapping;
                }

                // warn about ignored records
                else
                {
                    if( m_caseAccess->GetCaseConstructionReporter() != nullptr )
                    {
                        const std::wstring message = FormatTextCS2WS(_T("multiple records are not supported so '%s' will not be exported"),
                                                                     export_record_mapping.formatted_record_name.c_str());
                        m_caseAccess->GetCaseConstructionReporter()->IssueMessage(MessageType::Warning, 31102, ToString(m_type), message.c_str());
                    }
                }
            }
        }
    }

    ASSERT(m_singleExportRecordMapping != nullptr);
}


void SingleRecordExportWriterBase::WriteCase(const Case& data_case)
{
    if( m_singleExportRecordMapping->level_number == 0 )
    {
        const CaseLevel& root_case_level = data_case.GetRootCaseLevel();
        WriteCaseRecord(*m_singleExportRecordMapping, root_case_level.GetCaseRecord(m_singleExportRecordMapping->record_number));
    }

    // if not on the first level, go through each level to find the record
    else
    {
        for( const CaseLevel* case_level : data_case.GetAllCaseLevels() )
        {
            if( m_singleExportRecordMapping->level_number == case_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber() )
                WriteCaseRecord(*m_singleExportRecordMapping, case_level->GetCaseRecord(m_singleExportRecordMapping->record_number));
        }
    }
}
