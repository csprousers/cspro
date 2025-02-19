#include "stdafx.h"
#include "DataRepositoryEvent.h"
#include <zUtilO/ConnectionString.h>

using namespace Paradata;


// --------------------------------------------------------------------------
// DataRepositoryEvent
// --------------------------------------------------------------------------

void DataRepositoryEvent::SetupTables(Log& log)
{
    Table& table = log.CreateTable(ParadataTable::DataRepositoryInfo)
            .AddColumn("dictionary_name", Table::ColumnType::Long)
            .AddColumn("filename", Table::ColumnType::Text)
            .AddColumn("type", Table::ColumnType::Integer)
        ;

    for( DataRepositoryType data_repository_type = DataRepositoryType::Null;
         data_repository_type <= DataRepositoryType::Stata;
         data_repository_type = static_cast<DataRepositoryType>(static_cast<int>(data_repository_type) + 1) )
    {
        table.AddCode(DataRepositoryTypeToParadataInt(data_repository_type), ToString(data_repository_type));
    }

    log.CreateTable(ParadataTable::DataRepositoryInstance)
            .AddColumn("data_source_info", Table::ColumnType::Long)
            .AddColumn("access_type", Table::ColumnType::Integer)
                    .AddCode(DataRepositoryAccess::BatchInput, "batch_input")
                    .AddCode(DataRepositoryAccess::BatchOutput, "batch_output")
                    .AddCode(DataRepositoryAccess::BatchOutputAppend, "batch_output_append")
                    .AddCode(DataRepositoryAccess::ReadOnly, "read_only")
                    .AddCode(DataRepositoryAccess::ReadWrite, "read_write")
                    .AddCode(DataRepositoryAccess::EntryInput, "entry_input")
            .AddColumn("open_type", Table::ColumnType::Integer)
                    .AddCode(DataRepositoryOpenFlag::CreateNew, "create_new")
                    .AddCode(DataRepositoryOpenFlag::OpenOrCreate, "open_or_create")
                    .AddCode(DataRepositoryOpenFlag::OpenMustExist, "open_must_exist")
        ;

    log.CreateTable(ParadataTable::DataRepositoryEvent)
            .AddColumn("data_source_instance", Table::ColumnType::Long, true)
            .AddColumn("action", Table::ColumnType::Integer)
                    .AddCode(Action::Close, "close")
                    .AddCode(Action::Open, "open")
                    .AddCode(Action::ReadCase, "load_case")
                    .AddCode(Action::WriteCase, "write_case")
                    .AddCode(Action::DeleteCase, "delete_case")
                    .AddCode(Action::CaseNotFound, "case_not_found")
                    .AddCode(Action::UndeleteCase, "undelete_case")
            .AddColumn("case_info", Table::ColumnType::Long, true)
            .AddColumn("case_key_info", Table::ColumnType::Long, true)
            .AddColumn("partial_save", Table::ColumnType::Boolean, true)
                    .AddCode(0, "not_partial")
                    .AddCode(1, "partial")
        ;
}


DataRepositoryEvent::DataRepositoryEvent(const Action action, std::shared_ptr<NamedObject> dictionary,
                                         std::string case_uuid/* = std::string()*/, std::string case_key/* = std::string()*/,
                                         const bool partial_save/* = false*/)
    :   m_action(action),
        m_dictionary(std::move(dictionary)),
        m_caseUuid(std::move(case_uuid)),
        m_caseKey(std::move(case_key)),
        m_partialSave(partial_save)
{
}


constexpr int DataRepositoryEvent::DataRepositoryTypeToParadataInt(const DataRepositoryType data_repository_type)
{
    static_assert(static_cast<int>(DataRepositoryType::Null) == 0 &&
                  static_cast<int>(DataRepositoryType::Stata) == 15);

    switch( data_repository_type )
    {
        case DataRepositoryType::Null:               return 0;
        case DataRepositoryType::Text:               return 1;
        case DataRepositoryType::SQLite:             return 2;
        case DataRepositoryType::EncryptedSQLite:    return 3;
        case DataRepositoryType::Memory:             return 4;
        case DataRepositoryType::Json:               return 5;
        case DataRepositoryType::CSWeb:              return 15;
        case DataRepositoryType::CommaDelimited:     return 6;
        case DataRepositoryType::SemicolonDelimited: return 7;
        case DataRepositoryType::TabDelimited:       return 8;
        case DataRepositoryType::Excel:              return 9;
        case DataRepositoryType::CSProExport:        return 10;
        case DataRepositoryType::R:                  return 11;
        case DataRepositoryType::SAS:                return 12;
        case DataRepositoryType::SPSS:               return 13;
        case DataRepositoryType::Stata:              return 14;
        default:                                     return ReturnProgrammingError(-1);
    }
}


void DataRepositoryEvent::Save(Log& log, long base_event_id) const
{
    const std::optional<long> case_info_id = !m_caseUuid.empty() ? std::make_optional(log.AddCaseInfo(m_dictionary.get(), m_caseUuid)) :
                                                                   std::nullopt;

    const std::optional<long> case_key_info_id = !m_caseKey.empty() ? std::make_optional(log.AddCaseKeyInfo(m_dictionary.get(), m_caseKey, case_info_id)) :
                                                                      std::nullopt;

    Table& data_repository_event_table = log.GetTable(ParadataTable::DataRepositoryEvent);
    data_repository_event_table.Insert(&base_event_id,
        GetOptionalValueOrNull(log.GetInstance(*this)),
        static_cast<int>(m_action),
        GetOptionalValueOrNull(case_info_id),
        GetOptionalValueOrNull(case_key_info_id),
        ( m_action == Action::WriteCase ) ? &m_partialSave : nullptr
    );

    if( m_action == Action::Close )
        log.StopInstance(*this);
}



// --------------------------------------------------------------------------
// DataRepositoryOpenEvent
// --------------------------------------------------------------------------

DataRepositoryOpenEvent::DataRepositoryOpenEvent(std::shared_ptr<NamedObject> dictionary, std::string repository_name,
                                                 const DataRepositoryType type, const DataRepositoryAccess access_type, const DataRepositoryOpenFlag open_flag)
    :   DataRepositoryEvent(Action::Open, std::move(dictionary)),
        m_repositoryName(std::move(repository_name)),
        m_dataRepositoryType(type),
        m_dataRepositoryAccess(access_type),
        m_dataRepositoryOpenFlag(open_flag)
{
}


void DataRepositoryOpenEvent::Save(Log& log, long base_event_id) const
{
    Table& data_repository_info_table = log.GetTable(ParadataTable::DataRepositoryInfo);
    long data_repository_info_id = 0;
    data_repository_info_table.Insert(&data_repository_info_id,
        log.AddNamedObject(m_dictionary.get()),
        m_repositoryName.c_str(),
        static_cast<int>(m_dataRepositoryType)
    );

    Table& data_repository_instance_table = log.GetTable(ParadataTable::DataRepositoryInstance);
    long data_repository_instance_id = 0;
    data_repository_instance_table.Insert(&data_repository_instance_id,
        data_repository_info_id,
        static_cast<int>(m_dataRepositoryAccess),
        static_cast<int>(m_dataRepositoryOpenFlag)
    );

    log.StartInstance(*this, data_repository_instance_id);

    DataRepositoryEvent::Save(log, base_event_id);
}
