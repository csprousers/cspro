#pragma once


// NamedReference is a generic reference to something with a name, potentially with occurrences.

class NamedReference
{
public:
    NamedReference(std::string name, std::string level_key);

    virtual ~NamedReference() { }

    bool operator==(const NamedReference& rhs) const;
    bool operator!=(const NamedReference& rhs) const { return !operator==(rhs); }

    static bool AreEqual(const NamedReference* lhs, const NamedReference* rhs);

    const std::string& GetName() const { return m_name; }

    const std::string& GetLevelKey() const  { return m_levelKey; }
    void SetLevelKey(std::string level_key) { m_levelKey = std::move(level_key); }

    virtual bool HasOccurrences() const { return false; }

    virtual std::string GetMinimalOccurrencesText() const { return std::string(); }

    virtual const size_t* GetZeroBasedOccurrences() const      { return nullptr; }
    virtual std::vector<size_t> GetOneBasedOccurrences() const { return std::vector<size_t>(); }

    bool NameAndOccurrencesMatch(const NamedReference& rhs) const;

protected:
    virtual bool OccurrencesMatch(const NamedReference& rhs) const { return !rhs.HasOccurrences(); }

private:
    std::string m_name;
    std::string m_levelKey;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline NamedReference::NamedReference(std::string name, std::string level_key)
    :   m_name(std::move(name)),
        m_levelKey(std::move(level_key))
{
}


inline bool NamedReference::operator==(const NamedReference& rhs) const
{
    return ( m_name == rhs.m_name &&
             m_levelKey == rhs.m_levelKey &&
             OccurrencesMatch(rhs) );
}


inline bool NamedReference::AreEqual(const NamedReference* const lhs, const NamedReference* const rhs)
{
    return ( lhs == nullptr ) ? ( rhs == nullptr ) :
           ( rhs == nullptr ) ? false :
                                ( *lhs == *rhs );
}


inline bool NamedReference::NameAndOccurrencesMatch(const NamedReference& rhs) const
{
    return ( m_name == rhs.m_name &&
             OccurrencesMatch(rhs) );
}
