#pragma once
#include "zParadataO.h"

namespace Paradata
{
    class Log;

    class ZPARADATAO_API KeyingInstance
    {
        friend class CaseEvent;

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

    protected:
        static void SetupTables(Log& log);
        long Save(Log& log) const;

    public:
        KeyingInstance();

        void Pause();
        void UnPause();

        void IncreaseKeystrokes();
        void IncreaseKeyingErrors();

        void IncreaseFieldsVerified();
        void IncreaseFieldsKeyerError();
        void IncreaseFieldsVerifierError();

        void SetRecordsWritten(int records_written);
    };
}
