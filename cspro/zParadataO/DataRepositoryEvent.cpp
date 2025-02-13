#include "stdafx.h"
#include "DataRepositoryEvent.h"
#include <zDataO/DataRepository.h>

namespace Paradata
{
    void DataRepositoryEvent::SetupTables(Log& log)
    {
        Table& table = log.CreateTable(ParadataTable::DataRepositoryInfo)
                .AddColumn(_T("dictionary_name"), Table::ColumnType::Long)
                .AddColumn(_T("filename"), Table::ColumnType::Text)
                .AddColumn(_T("type"), Table::ColumnType::Integer)
            ;

        for( int type = (int)DataRepositoryType::Null; type <= (int)DataRepositoryType::Stata; ++type )
            table.AddCode(type, ToString((DataRepositoryType)type));

        log.CreateTable(ParadataTable::DataRepositoryInstance)
                .AddColumn(_T("data_source_info"), Table::ColumnType::Long)
                .AddColumn(_T("access_type"), Table::ColumnType::Integer)
                        .AddCode((int)DataRepositoryAccess::BatchInput, _T("batch_input"))
                        .AddCode((int)DataRepositoryAccess::BatchOutput, _T("batch_output"))
                        .AddCode((int)DataRepositoryAccess::BatchOutputAppend, _T("batch_output_append"))
                        .AddCode((int)DataRepositoryAccess::ReadOnly, _T("read_only"))
                        .AddCode((int)DataRepositoryAccess::ReadWrite, _T("read_write"))
                        .AddCode((int)DataRepositoryAccess::EntryInput, _T("entry_input"))
                .AddColumn(_T("open_type"), Table::ColumnType::Integer)
                        .AddCode((int)DataRepositoryOpenFlag::CreateNew, _T("create_new"))
                        .AddCode((int)DataRepositoryOpenFlag::OpenOrCreate, _T("open_or_create"))
                        .AddCode((int)DataRepositoryOpenFlag::OpenMustExist, _T("open_must_exist"))
            ;

        log.CreateTable(ParadataTable::DataRepositoryEvent)
                .AddColumn(_T("data_source_instance"), Table::ColumnType::Long, true)
                .AddColumn(_T("action"), Table::ColumnType::Integer)
                        .AddCode((int)Action::Close, _T("close"))
                        .AddCode((int)Action::Open, _T("open"))
                        .AddCode((int)Action::ReadCase, _T("load_case"))
                        .AddCode((int)Action::WriteCase, _T("write_case"))
                        .AddCode((int)Action::DeleteCase, _T("delete_case"))
                        .AddCode((int)Action::CaseNotFound, _T("case_not_found"))
                        .AddCode((int)Action::UndeleteCase, _T("undelete_case"))
                .AddColumn(_T("case_info"), Table::ColumnType::Long, true)
                .AddColumn(_T("case_key_info"), Table::ColumnType::Long, true)
                .AddColumn(_T("partial_save"), Table::ColumnType::Boolean, true)
                        .AddCode(0, _T("not_partial"))
                        .AddCode(1, _T("partial"))
            ;
    }

    DataRepositoryEvent::DataRepositoryEvent(Action action, std::shared_ptr<NamedObject> dictionary,
        const CString& case_uuid/* = CString()*/, const CString& case_key/* = CString()*/, bool partial_save/* = false*/)
        :   m_action(action),
            m_dictionary(dictionary),
            m_caseUuid(case_uuid),
            m_caseKey(case_key),
            m_partialSave(partial_save)
    {
    }

    void DataRepositoryEvent::Save(Log& log, long base_event_id) const
    {
        std::optional<long> case_info_id = !m_caseUuid.IsEmpty() ?
            (std::optional<long>)log.AddCaseInfo(m_dictionary, m_caseUuid) : std::nullopt;

        std::optional<long> case_key_info_id = !m_caseKey.IsEmpty() ?
            (std::optional<long>)log.AddCaseKeyInfo(m_dictionary, m_caseKey, case_info_id) : std::nullopt;

        Table& data_repository_event_table = log.GetTable(ParadataTable::DataRepositoryEvent);
        data_repository_event_table.Insert(&base_event_id,
            GetOptionalValueOrNull(log.GetInstance(*this)),
            (int)m_action,
            GetOptionalValueOrNull(case_info_id),
            GetOptionalValueOrNull(case_key_info_id),
            ( m_action == Action::WriteCase ) ? &m_partialSave : nullptr
        );

        if( m_action == Action::Close )
            log.StopInstance(*this);
    }


    DataRepositoryOpenEvent::DataRepositoryOpenEvent(std::shared_ptr<NamedObject> dictionary, const CString& filename,
        int data_repository_type, int data_repository_access, int data_repository_open_flag)
        :   DataRepositoryEvent(Action::Open, dictionary),
            m_filename(filename),
            m_dataRepositoryType(data_repository_type),
            m_dataRepositoryAccess(data_repository_access),
            m_dataRepositoryOpenFlag(data_repository_open_flag)
    {
    }

    void DataRepositoryOpenEvent::Save(Log& log, long base_event_id) const
    {
        Table& data_repository_info_table = log.GetTable(ParadataTable::DataRepositoryInfo);
        long data_repository_info_id = 0;
        data_repository_info_table.Insert(&data_repository_info_id,
            log.AddNamedObject(m_dictionary),
            (LPCTSTR)m_filename,
            m_dataRepositoryType
        );

        Table& data_repository_instance_table = log.GetTable(ParadataTable::DataRepositoryInstance);
        long data_repository_instance_id = 0;
        data_repository_instance_table.Insert(&data_repository_instance_id,
            data_repository_info_id,
            m_dataRepositoryAccess,
            m_dataRepositoryOpenFlag
        );

        log.StartInstance(*this, data_repository_instance_id);

        DataRepositoryEvent::Save(log, base_event_id);
    }
}
