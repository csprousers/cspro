#include "stdafx.h"
#include "KeyingInstance.h"

using namespace Paradata;


void KeyingInstance::SetupTables(Log& log)
{
    log.CreateTable(ParadataTable::KeyingInstance)
            .AddColumn("pause_count", Table::ColumnType::Integer)
            .AddColumn("pause_duration", Table::ColumnType::Double)
            .AddColumn("keystrokes", Table::ColumnType::Integer)
            .AddColumn("keying_errors", Table::ColumnType::Integer)
            .AddColumn("fields_verified", Table::ColumnType::Integer)
            .AddColumn("fields_verified_keyer_error", Table::ColumnType::Integer)
            .AddColumn("fields_verified_verifier_error", Table::ColumnType::Integer)
            .AddColumn("records_written", Table::ColumnType::Integer, true)
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
        m_pauseTimestamp = ::GetTimestamp<double>();
    }
}


void KeyingInstance::UnPause()
{
    if( m_pauseTimestamp.has_value() )
    {
        m_pauseDuration += ( ::GetTimestamp<double>() - *m_pauseTimestamp );
        m_pauseTimestamp.reset();
    }
}
