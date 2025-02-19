#pragma once

#include <zCaseO/NamedReference.h>
#include <time.h>


// Note for a field or other named reference in a case

class Note
{
public:
    Note(SharableString content, std::shared_ptr<NamedReference> named_reference, std::string operator_id, int64_t modified_date_time = 0);

    bool operator==(const Note& rhs) const;
    bool operator!=(const Note& rhs) const { return !operator==(rhs); }

    const SharableString& GetContentSharableString() const { return m_content; }
    const std::string& GetContent() const                  { return *m_content; }

    void SetContent(SharableString content, bool update_modified_date_time = true);

    std::shared_ptr<const NamedReference> GetSharedNamedReference() const { return m_namedReference; }
    const NamedReference& GetNamedReference() const                       { return *m_namedReference; }
    NamedReference& GetNamedReference()                                   { return *m_namedReference; }

    const std::string& GetOperatorId() const { return m_operatorId; }

    int64_t GetModifiedDateTime() const { return m_modifiedDateTime; }

private:
    SharableString m_content;
    std::shared_ptr<NamedReference> m_namedReference;
    std::string m_operatorId;
    int64_t m_modifiedDateTime;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Note::Note(SharableString content, std::shared_ptr<NamedReference> named_reference, std::string operator_id, int64_t modified_date_time/* = 0*/)
    :   m_content(std::move(content)),
        m_namedReference(std::move(named_reference)),
        m_operatorId(std::move(operator_id)),
        m_modifiedDateTime(( modified_date_time == 0 ) ? time(nullptr) : modified_date_time)
{
    ASSERT(m_namedReference != nullptr);
}


inline bool Note::operator==(const Note& rhs) const
{
    return ( *m_content == *rhs.m_content &&
             NamedReference::AreEqual(m_namedReference.get(), rhs.m_namedReference.get()) &&
             m_operatorId == rhs.m_operatorId &&
             m_modifiedDateTime == rhs.m_modifiedDateTime );
}


inline void Note::SetContent(SharableString content, const bool update_modified_date_time/* = true*/)
{
    m_content = std::move(content);

    if( update_modified_date_time )
        m_modifiedDateTime = time(nullptr);
}
