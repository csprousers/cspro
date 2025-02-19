#pragma once


class EngineSettings
{
public:
    bool GetTreatSpecialValuesAsZero() const    { return m_treatSpecialValuesAsZero; }
    void SetTreatSpecialValuesAsZero(bool flag) { m_treatSpecialValuesAsZero = flag; }

private:
    bool m_treatSpecialValuesAsZero = false;
};
