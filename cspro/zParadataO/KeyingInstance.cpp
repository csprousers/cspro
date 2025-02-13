#include "stdafx.h"
#include "KeyingInstance.h"

namespace Paradata
{
    void KeyingInstance::SetupTables(Log& log)
    {
        log.CreateTable(ParadataTable::KeyingInstance)
                .AddColumn(_T("pause_count"), Table::ColumnType::Integer)
                .AddColumn(_T("pause_duration"), Table::ColumnType::Double)
                .AddColumn(_T("keystrokes"), Table::ColumnType::Integer)
                .AddColumn(_T("keying_errors"), Table::ColumnType::Integer)
                .AddColumn(_T("fields_verified"), Table::ColumnType::Integer)
                .AddColumn(_T("fields_verified_keyer_error"), Table::ColumnType::Integer)
                .AddColumn(_T("fields_verified_verifier_error"), Table::ColumnType::Integer)
                .AddColumn(_T("records_written"), Table::ColumnType::Integer, true)
            ;
    }

    KeyingInstance::KeyingInstance()
        :   m_pauseCount(0),
            m_pauseDuration(0),
            m_keystrokes(0),
            m_keyingErrors(0),
            m_fieldsVerified(0),
            m_fieldsKeyerError(0),
            m_fieldsVerifierError(0)
    {
    }

    long KeyingInstance::Save(Log& log) const
    {
        Table& keying_instance_table = log.GetTable(ParadataTable::KeyingInstance);
        long keying_instance_id = 0;
        keying_instance_table.Insert(&keying_instance_id,
            m_pauseCount,
            m_pauseDuration,
            m_keystrokes,
            m_keyingErrors,
            m_fieldsVerified,
            m_fieldsKeyerError,
            m_fieldsVerifierError,
            GetOptionalValueOrNull(m_recordsWritten)
        );

        return keying_instance_id;
    }

    void KeyingInstance::Pause()
    {
        if( !m_pauseTimestamp.has_value() )
        {
            ++m_pauseCount;
            m_pauseTimestamp = ::GetTimestamp();
        }
    }

    void KeyingInstance::UnPause()
    {
        if( m_pauseTimestamp.has_value() )
        {
            m_pauseDuration += ( ::GetTimestamp() - *m_pauseTimestamp );
            m_pauseTimestamp.reset();
        }
    }

    void KeyingInstance::IncreaseKeystrokes()
    {
        ++m_keystrokes;
    }

    void KeyingInstance::IncreaseKeyingErrors()
    {
        ++m_keyingErrors;
    }

    void KeyingInstance::IncreaseFieldsVerified()
    {
        ++m_fieldsVerified;
    }

    void KeyingInstance::IncreaseFieldsKeyerError()
    {
        ++m_fieldsKeyerError;
    }

    void KeyingInstance::IncreaseFieldsVerifierError()
    {
        ++m_fieldsVerifierError;
    }

    void KeyingInstance::SetRecordsWritten(int records_written)
    {
        m_recordsWritten = records_written;
    }
}
