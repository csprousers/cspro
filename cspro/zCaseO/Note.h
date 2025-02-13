#pragma once

#include <zCaseO/NamedReference.h>
#include <time.h>


// Note for a field or other named reference in a case

class Note
{
public:
	Note(const CString& content, std::shared_ptr<NamedReference> named_reference, const CString& operator_id, time_t modified_date_time = 0)
		:	m_content(content),
			m_namedReference(std::move(named_reference)),
			m_operatorId(operator_id),
			m_modifiedDateTime(( modified_date_time == 0 ) ? time(nullptr) : modified_date_time)
	{
        ASSERT(m_namedReference != nullptr);
	}

    const CString& GetContent() const
    {
        return m_content;
    }
	
    void SetContent(const CString& content)
	{
		m_content = content;
		m_modifiedDateTime = time(nullptr);
	}
	
    const NamedReference& GetNamedReference() const
    {
        return *m_namedReference;
    }
	
    NamedReference& GetNamedReference()
    {
        return *m_namedReference;
    }

    std::shared_ptr<const NamedReference> GetSharedNamedReference() const
    {
        return m_namedReference;
    }
	
    const CString& GetOperatorId() const
    {
        return m_operatorId;
    }
	
    time_t GetModifiedDateTime() const
    {
        return m_modifiedDateTime;
    }

private:
    CString m_content;
	std::shared_ptr<NamedReference> m_namedReference;
    CString m_operatorId;
    time_t m_modifiedDateTime;
};
