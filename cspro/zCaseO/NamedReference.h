#pragma once


// Reference to a generic named reference, potentially with occurrences
class NamedReference
{
public:
    NamedReference(const CString& name, const CString& level_key)
		:   m_name(name),
			m_levelKey(level_key)
    {
    }

    virtual ~NamedReference() { }

    const CString& GetName() const
	{
		return m_name;
	}

    const CString& GetLevelKey() const
	{
		return m_levelKey;
	}

    void SetLevelKey(const CString& level_key)
	{
		m_levelKey = level_key;
	}

    virtual bool HasOccurrences() const
	{
		return false;
	}

    virtual CString GetMinimalOccurrencesText() const
    {
        return CString();
    }

    virtual const size_t* GetZeroBasedOccurrences() const
    {
        return nullptr;
    }

    virtual std::vector<size_t> GetOneBasedOccurrences() const
	{
		return std::vector<size_t>();
	}

    bool NameAndOccurrencesMatch(const NamedReference& rhs) const
    {
        return ( m_name.Compare(rhs.m_name) == 0 && OccurrencesMatch(rhs) );
    }

protected:
    virtual bool OccurrencesMatch(const NamedReference& rhs) const
    {
        return !rhs.HasOccurrences();
    }

	CString m_name;
    CString m_levelKey;
};
