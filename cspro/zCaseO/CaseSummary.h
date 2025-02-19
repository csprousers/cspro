#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/CaseKey.h>
#include <zCaseO/CaseDefines.h>


class CaseSummary : public CaseKey
{
public:
    CaseSummary();
    CaseSummary(CaseKey case_key, std::string case_label, bool deleted, bool verified,
                PartialSaveMode partial_save_mode, std::string case_note);

    CaseSummary(const CaseSummary&) = default;
    CaseSummary(CaseSummary&&) noexcept = default;
    CaseSummary(const Case& data_case);
    CaseSummary(Case&& data_case);

    CaseSummary& operator=(const CaseSummary&) = default;
    CaseSummary& operator=(CaseSummary&&) = default;
    CaseSummary& operator=(const Case& data_case);
    CaseSummary& operator=(Case&& data_case);

    // Returns the case label.
    const std::string& GetCaseLabel() const   { return m_caseLabel; }
    void SetCaseLabel(std::string case_label) { m_caseLabel = std::move(case_label); }

    ZCASEO_API std::string GetSingleLineCaseLabel() const;

    // Returns the case label if one is set; if not, the key is returned.
    const std::string& GetCaseLabelOrKey() const { return m_caseLabel.empty() ? GetKey() : m_caseLabel; }

    // Returns true if the case has been deleted.
    bool GetDeleted() const       { return m_deleted; }
    void SetDeleted(bool deleted) { m_deleted = deleted; }

    // Returns true if the case has been verified (double entered).
    bool GetVerified() const        { return m_verified; }
    void SetVerified(bool verified) { m_verified = verified; }

    // Returns the partial save mode.
    PartialSaveMode GetPartialSaveMode() const                 { return m_partialSaveMode; }
    void SetPartialSaveMode(PartialSaveMode partial_save_mode) { m_partialSaveMode = partial_save_mode; }

    // Returns whether or not the case is partially saved.
    bool IsPartial() const { return ( m_partialSaveMode != PartialSaveMode::None ); }

    // Returns the case note.
    virtual const std::string& GetCaseNote() const  { return m_caseNote; }
    virtual void SetCaseNote(std::string case_note) { m_caseNote = std::move(case_note); }
    virtual void ResetCaseNote()                    { m_caseNote.clear(); }

protected:
    std::string m_caseLabel;
    bool m_deleted;
    bool m_verified;
    PartialSaveMode m_partialSaveMode;
    std::string m_caseNote;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CaseSummary::CaseSummary()
    :   m_deleted(false),
        m_verified(false),
        m_partialSaveMode(PartialSaveMode::None)
{
}


inline CaseSummary::CaseSummary(CaseKey case_key, std::string case_label, const bool deleted, const bool verified,
                                const PartialSaveMode partial_save_mode, std::string case_note)
    :   CaseKey(std::move(case_key)),
        m_caseLabel(std::move(case_label)),
        m_deleted(deleted),
        m_verified(verified),
        m_partialSaveMode(partial_save_mode),
        m_caseNote(std::move(case_note))
{
}
