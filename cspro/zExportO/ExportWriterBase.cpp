#include "stdafx.h"
#include "ExportWriterBase.h"


ExportWriterBase::ExportWriterBase(const DataRepositoryType type, std::shared_ptr<const CaseAccess> case_access,
                                   const ConnectionString& connection_string)
    :   m_type(type),
        m_caseAccess(std::move(case_access)),
        m_connectionString(connection_string)
{
    m_suppressMappedSpecialValues = ( m_connectionString.HasProperty(CSProperty::mappedSpecialValues, CSValue::suppress) ||
                                      m_connectionString.HasProperty(_T("mapped-special-values"), _T("notappl")) ); // pre-8.0
}


void ExportWriterBase::CreateExportRecordMappings()
{
    const CaseMetadata& case_metadata = m_caseAccess->GetCaseMetadata();

    m_supportedCaseItemTypes =
    {
        CaseItem::Type::String,
        CaseItem::Type::FixedWidthString,
        CaseItem::Type::Numeric,
        CaseItem::Type::FixedWidthNumeric,
        CaseItem::Type::FixedWidthNumericWithStringBuffer
    };

    if( SupportsBinaryData() )
        m_supportedCaseItemTypes.emplace(CaseItem::Type::Binary);

    const std::wstring* single_record_to_export_name = m_connectionString.GetProperty(CSProperty::record);
    bool found_single_record = false;

    // create the mappings
    std::vector<const CaseRecordMetadata*> id_case_records;
    bool has_record_mappings = false;

    for( const CaseLevelMetadata* case_level_metadata : case_metadata.GetCaseLevelsMetadata() )
    {
        // create a mapping entry for this level
        std::vector<ExportRecordMapping>& export_record_mapping_for_level = m_exportRecordMappingByLevel.emplace_back();

        // add this level's ID case record
        id_case_records.emplace_back(case_level_metadata->GetIdCaseRecordMetadata());

        for( const CaseRecordMetadata* case_record_metadata : case_level_metadata->GetCaseRecordsMetadata() )
        {
            // if only exporting a specific record, potentially ignore this record
            if( single_record_to_export_name != nullptr )
            {
                if( !SO::EqualsNoCase(*single_record_to_export_name, case_record_metadata->GetDictionaryRecord().GetName()) )
                    continue;

                found_single_record = true;
            }

            // create a mapping entry for this record
            ExportRecordMapping export_record_mapping = CreateExportRecordMapping(id_case_records, case_record_metadata);

            // ignore this record if nothing was mapped
            if( export_record_mapping.item_mappings.empty() )
                continue;

            export_record_mapping_for_level.emplace_back(std::move(export_record_mapping));

            has_record_mappings = true;
        }
    }

    if( !has_record_mappings )
    {
        if( single_record_to_export_name != nullptr && !found_single_record )
        {
            throw CSProException(_T("'%s' is not a record in the dictionary '%s'"),
                                 single_record_to_export_name->c_str(),
                                 case_metadata.GetDictionary().GetName().GetString());
        }

        else
        {
            throw CSProException("There are no items that can be handled by the exporter");
        }
    }
}


ExportRecordMapping ExportWriterBase::CreateExportRecordMapping(const std::vector<const CaseRecordMetadata*>& id_case_records,
                                                                const CaseRecordMetadata* case_record_metadata)
{
    ExportRecordMapping export_record_mapping
    {
        case_record_metadata,
        CreateFormattedName(m_formattedRecordNames, case_record_metadata->GetDictionaryRecord().GetName(), true),
        case_record_metadata->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber(),
        case_record_metadata->GetRecordIndex(),
        { },
        nullptr
    };

    std::set<std::wstring> formatted_names;
    formatted_names.insert(export_record_mapping.formatted_record_name);

    // map the case items
    auto map_case_items = [&](const CaseRecordMetadata* case_record_metadata, const std::optional<size_t> id_level_number)
    {
        for( const CaseItem* case_item : case_record_metadata->GetCaseItems() )
        {
            // ignore unsupported case items
            if( m_supportedCaseItemTypes.find(case_item->GetType()) == m_supportedCaseItemTypes.cend() )
            {
                if( m_caseAccess->GetCaseConstructionReporter() != nullptr )
                {
                    m_caseAccess->GetCaseConstructionReporter()->IssueMessage(MessageType::Warning, 31101,
                                                                              ToString(m_type),
                                                                              ToString(case_item->GetDictionaryItem().GetContentType()));
                }

                continue;
            }

            // add an entry per occurrence
            for( size_t occurrence = 0; occurrence < case_item->GetTotalNumberItemSubitemOccurrences(); ++occurrence )
            {
                // when necessary, the occurrence will be appended to the item name
                std::wstring item_name = CS2WS(case_item->GetDictionaryItem().GetName());

                if( case_item->GetTotalNumberItemSubitemOccurrences() > 1 )
                    SO::AppendFormat(item_name, _T("_%d"), static_cast<int>(occurrence) + 1);

                export_record_mapping.item_mappings.emplace_back(ExportItemMapping
                    {
                        case_item,
                        occurrence,
                        CreateFormattedName(formatted_names, item_name, false),
                        id_level_number,
                        nullptr
                    });
            }
        }
    };

    // add the ID case records
    for( size_t id_level_number = 0; id_level_number < id_case_records.size(); ++id_level_number )
        map_case_items(id_case_records[id_level_number], id_level_number);

    // add the non-ID case record
    map_case_items(case_record_metadata, std::nullopt);

    return export_record_mapping;
}


std::wstring ExportWriterBase::CreateFormattedName(std::set<std::wstring>& used_names, const wstring_view name_sv,
                                                   const bool record_name, const bool ensure_unique/* = false*/)
{
    for( int i = 0; ; ++i )
    {
        std::wstring formatted_name = name_sv;

        if( i > 0 )
            SO::AppendFormat(formatted_name, _T("_%d"), i);

        if( used_names.find(formatted_name) == used_names.cend() &&
            ( !ensure_unique || m_allUsedNames.find(formatted_name) == m_allUsedNames.cend() ) &&
            !IsReservedName(formatted_name, record_name) )
        {
            used_names.insert(formatted_name);
            m_allUsedNames.insert(formatted_name);
            return formatted_name;
        }
    }
}


std::wstring ExportWriterBase::CreateUniqueName(const wstring_view name_sv)
{
    return CreateFormattedName(m_allUsedNames, name_sv, false, true);
}


void ExportWriterBase::WriteCase(const Case& data_case)
{
    auto write_case_level = [&](const CaseLevel& case_level, const size_t level_number)
    {
        for( const ExportRecordMapping& export_record_mapping : m_exportRecordMappingByLevel[level_number] )
            WriteCaseRecord(export_record_mapping, case_level.GetCaseRecord(export_record_mapping.record_number));
    };

    if( m_exportRecordMappingByLevel.size() == 1 )
    {
        write_case_level(data_case.GetRootCaseLevel(), 0);
    }

    else
    {
        for( const CaseLevel* case_level : data_case.GetAllCaseLevels() )
            write_case_level(*case_level, case_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber());
    }
}


void ExportWriterBase::WriteCaseRecord(const ExportRecordMapping& export_record_mapping, const CaseRecord& case_record)
{
    ASSERT(export_record_mapping.case_record_metadata == &case_record.GetCaseRecordMetadata());

    if( !case_record.HasOccurrences() )
        return;

    StartRecord(export_record_mapping);

    std::optional<std::tuple<size_t, CaseItemIndex>> level_id_index;

    for( CaseItemIndex record_index = case_record.GetCaseItemIndex();
         record_index.GetRecordOccurrence() < case_record.GetNumberOccurrences();
         record_index.IncrementRecordOccurrence() )
    {
        StartRow();

        for( const ExportItemMapping& export_item_mapping : export_record_mapping.item_mappings )
        {
            // when outputting ID record items, we must get the correct ID record index
            if( export_item_mapping.id_level_number.has_value() )
            {
                if( !level_id_index.has_value() || std::get<size_t>(*level_id_index) != *export_item_mapping.id_level_number )
                {
                    ASSERT(*export_item_mapping.id_level_number <= export_record_mapping.level_number);

                    // traverse the level tree (upwards) until the correct one for this ID is found
                    const CaseLevel* case_level = &case_record.GetCaseLevel();

                    while( true )
                    {
                        if( case_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber() == *export_item_mapping.id_level_number )
                        {
                            level_id_index.emplace(*export_item_mapping.id_level_number, case_level->GetIdCaseRecord().GetCaseItemIndex());
                            break;
                        }

                        else
                        {
                            case_level = &case_level->GetParentCaseLevel();
                        }
                    }
                }

                WriteCaseItem(export_item_mapping, std::get<CaseItemIndex>(*level_id_index));
            }

            // record ID items can use the record index
            else
            {
                record_index.SetItemSubitemOccurrence(*export_item_mapping.case_item, export_item_mapping.item_subitem_occurrence);
                WriteCaseItem(export_item_mapping, record_index);
            }
        }

        EndRow();
    }
}
