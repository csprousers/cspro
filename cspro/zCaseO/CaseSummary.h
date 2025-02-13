#pragma once

#include <zCaseO/CaseKey.h>
#include <zCaseO/CaseDefines.h>


class CaseSummary : public CaseKey
{
public:
    CaseSummary()
        :   m_deleted(false),
            m_verified(false),
            m_partialSaveMode(PartialSaveMode::None)
    {
    }

    CaseSummary(const CaseSummary& rhs)
        :   CaseKey(rhs),
            m_caseLabel(rhs.m_caseLabel),
            m_deleted(rhs.m_deleted),
            m_verified(rhs.m_verified),
            m_partialSaveMode(rhs.m_partialSaveMode),
            m_caseNote(rhs.GetCaseNote())
    {
    }

    CaseSummary& operator=(const CaseSummary& rhs)
    {
        CaseKey::operator=(rhs);
        m_caseLabel = rhs.m_caseLabel;
        m_deleted = rhs.m_deleted;
        m_verified = rhs.m_verified;
        m_partialSaveMode = rhs.m_partialSaveMode;
        m_caseNote = rhs.GetCaseNote();
        return *this;
    }


    const CString& GetCaseLabel() const
    {
        return m_caseLabel;
    }

    /// <summary>
    /// Returns the case label if one is set; if not, the key is returned.
    /// </summary>
    const CString& GetCaseLabelOrKey() const
    {
        return m_caseLabel.IsEmpty() ? GetKey() : m_caseLabel;
    }

    void SetCaseLabel(CString case_label)
    {
        m_caseLabel = std::move(case_label);
    }


    /// <summary>
    /// Returns true if the case has been deleted.
    /// </summary>
    bool GetDeleted() const
    {
        return m_deleted;
    }

    void SetDeleted(bool deleted)
    {
        m_deleted = deleted;
    }


    /// <summary>
    /// Returns true if the case has been verified (double entered).
    /// </summary>
    bool GetVerified() const
    {
        return m_verified;
    }

    void SetVerified(bool verified)
    {
        m_verified = verified;
    }


    /// <summary>
    /// Returns the partial save mode.
    /// </summary>
    PartialSaveMode GetPartialSaveMode() const
    {
        return m_partialSaveMode;
    }

    /// <summary>
    /// Returns whether or not the case is partially saved.
    /// </summary>
    bool IsPartial() const
    {
        return ( m_partialSaveMode != PartialSaveMode::None );
    }

    void SetPartialSaveMode(PartialSaveMode partial_save_mode)
    {
        m_partialSaveMode = partial_save_mode;
    }


    virtual const CString& GetCaseNote() const
    {
        return m_caseNote;
    }

    virtual void SetCaseNote(CString case_note)
    {
        m_caseNote = std::move(case_note);
    }

protected:
    CString m_caseLabel;
    bool m_deleted;
    bool m_verified;
    PartialSaveMode m_partialSaveMode;
    CString m_caseNote;
};
