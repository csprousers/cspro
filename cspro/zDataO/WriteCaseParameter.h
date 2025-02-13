#pragma once

#include <zCaseO/CaseKey.h>


class WriteCaseParameter : public CaseKey
{
private:
    WriteCaseParameter(CaseKey case_key, bool is_modification)
        :   CaseKey(std::move(case_key)),
            m_isModification(is_modification),
            m_notesModified(false)
    {
    }

public:
    static WriteCaseParameter CreateModifyParameter(CaseKey case_key)
    {
        return WriteCaseParameter(std::move(case_key), true);
    }

    static WriteCaseParameter CreateInsertParameter(double insert_before_position_in_repository)
    {
        return WriteCaseParameter(CaseKey(CString(), insert_before_position_in_repository), false);
    }

    bool IsModifyParameter() const { return m_isModification; }
    bool IsInsertParameter() const { return !m_isModification; }

    void SetNotesModified()
    {
        m_notesModified = true;
    }

    bool AreNotesModified() const
    {
        ASSERT(m_isModification);
        return m_notesModified;
    }

private:
    const bool m_isModification;
    bool m_notesModified;
};
