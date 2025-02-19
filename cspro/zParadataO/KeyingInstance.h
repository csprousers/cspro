#pragma once

#include <zParadataO/zParadataO.h>

namespace Paradata { class KeyingInstance; class Log; }


class ZPARADATAO_API Paradata::KeyingInstance
{
    friend class CaseEvent;

protected:
    static void SetupTables(Log& log);
    long Save(Log& log) const;

public:
    KeyingInstance();

    void Pause();
    void UnPause();

    void IncreaseKeystrokes()   { ++m_keystrokes; }
    void IncreaseKeyingErrors() { ++m_keyingErrors; }

    void IncreaseFieldsVerified()      { ++m_fieldsVerified; }
    void IncreaseFieldsKeyerError()    { ++m_fieldsKeyerError; }
    void IncreaseFieldsVerifierError() { ++m_fieldsVerifierError; }

    void SetRecordsWritten(int records_written) { m_recordsWritten = records_written; }

private:
    int m_pauseCount;
    double m_pauseDuration;
    std::optional<double> m_pauseTimestamp;

    int m_keystrokes;
    int m_keyingErrors;

    int m_fieldsVerified;
    int m_fieldsKeyerError;
    int m_fieldsVerifierError;

    std::optional<int> m_recordsWritten;
};
