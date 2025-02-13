#include "stdafx.h"
#include "CaseEvent.h"
#include "KeyingInstance.h"
#include <zUtilO/Interapp.h>

namespace Paradata
{
    void CaseEvent::SetupTables(Log& log)
    {
        KeyingInstance::SetupTables(log);

        log.CreateTable(ParadataTable::CaseInstance)
                .AddColumn(_T("case_info"), Table::ColumnType::Long)
            ;

        log.CreateTable(ParadataTable::CaseEvent)
                .AddColumn(_T("action"), Table::ColumnType::Boolean)
                        .AddCode(0, _T("stop"))
                        .AddCode(1, _T("start"))
                .AddColumn(_T("keying_instance"), Table::ColumnType::Long, true)
            ;
    }

    CaseEvent::CaseEvent(bool start, std::shared_ptr<NamedObject> dictionary/* = nullptr*/)
        :   m_start(start),
            m_dictionary(dictionary)
    {
    }

    std::shared_ptr<CaseEvent> CaseEvent::CreateStartEvent(std::shared_ptr<NamedObject> dictionary, const CString& case_uuid)
    {
        std::shared_ptr<CaseEvent> case_event(new CaseEvent(true, dictionary));
        case_event->m_caseUuid = case_uuid;

#ifdef WIN_DESKTOP
        AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_CONTROL_PARADATA_KEYING_INSTANCE, 0);
#endif

        return case_event;
    }

    std::shared_ptr<CaseEvent> CaseEvent::CreateStopEvent()
    {
        std::shared_ptr<CaseEvent> case_event(new CaseEvent(false));

#ifdef WIN_DESKTOP
        AfxGetApp()->GetMainWnd()->SendMessage(WM_IMSA_CONTROL_PARADATA_KEYING_INSTANCE, 1, (LPARAM)&case_event->m_keyingInstance);

        if( case_event->m_keyingInstance != nullptr )
            case_event->m_keyingInstance->UnPause();
#endif

        return case_event;
    }

    bool CaseEvent::PreSave(Log& log) const
    {
        if( m_start )
        {
            long case_info_id = log.AddCaseInfo(m_dictionary, m_caseUuid);

            // fill the case instance table
            Table& case_instance_table = log.GetTable(ParadataTable::CaseInstance);
            long case_instance_id = 0;
            case_instance_table.Insert(&case_instance_id,
                case_info_id
            );

            log.StartInstance(Log::Instance::Case, case_instance_id);
        }

        return log.GetInstance(Log::Instance::Case).has_value();
    }

    void CaseEvent::Save(Log& log, long base_event_id) const
    {
        std::optional<long> keying_instance_id = ( m_keyingInstance != nullptr ) ?
            (std::optional<long>)m_keyingInstance->Save(log) : std::nullopt;

        Table& case_event_table = log.GetTable(ParadataTable::CaseEvent);
        case_event_table.Insert(&base_event_id,
            m_start,
            GetOptionalValueOrNull(keying_instance_id)
        );

        if( !m_start )
            log.StopInstance(Log::Instance::Case);
    }
}
