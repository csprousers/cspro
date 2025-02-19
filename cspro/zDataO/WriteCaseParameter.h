#pragma once

#include <zCaseO/CaseKey.h>


class WriteCaseParameter : public CaseKey
{
private:
    template<typename CaseKeyT>
    WriteCaseParameter(CaseKeyT&& case_key, bool is_modification);

public:
    template<typename CaseKeyT>
    static WriteCaseParameter CreateModifyParameter(CaseKeyT&& case_key);

    static WriteCaseParameter CreateInsertParameter(double insert_before_position_in_repository);

    bool IsModifyParameter() const { return m_isModification; }
    bool IsInsertParameter() const { return !m_isModification; }

    void SetNotesModified()       { m_notesModified = true; }
    bool AreNotesModified() const { ASSERT(m_isModification); return m_notesModified; }

private:
    const bool m_isModification;
    bool m_notesModified;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename CaseKeyT>
WriteCaseParameter::WriteCaseParameter(CaseKeyT&& case_key, const bool is_modification)
    :   CaseKey(std::forward<CaseKeyT>(case_key)),
        m_isModification(is_modification),
        m_notesModified(false)
{
}


template<typename CaseKeyT>
WriteCaseParameter WriteCaseParameter::CreateModifyParameter(CaseKeyT&& case_key)
{
    return WriteCaseParameter(std::forward<CaseKeyT>(case_key), true);
}


inline WriteCaseParameter WriteCaseParameter::CreateInsertParameter(const double insert_before_position_in_repository)
{
    return WriteCaseParameter(CaseKey(std::string(), insert_before_position_in_repository), false);
}
