#pragma once


class CaseKey
{
public:
    CaseKey(CString key, double position_in_repository)
        :   m_key(std::move(key)),
            m_positionInRepository(position_in_repository)
    {
    }

    CaseKey()
        :   m_positionInRepository(-1)
    {
    }

    CaseKey(const CaseKey& rhs)
        :   m_key(rhs.GetKey()),
            m_positionInRepository(rhs.m_positionInRepository)
    {
    }

    virtual ~CaseKey()
    {
    }

    CaseKey& operator=(const CaseKey& rhs)
    {
        m_key = rhs.GetKey();
        m_positionInRepository = rhs.m_positionInRepository;
        return *this;
    }


    virtual const CString& GetKey() const
    {
        return m_key;
    }

    virtual void SetKey(CString key)
    {
        m_key = std::move(key);
    }

    /// <summary>
    /// Returns a double that can be used to locate this case in the repository from which it was read.
    /// The number is arbitrary and should not be used in any calculations. The number is initialized as -1.
    /// </summary>
    double GetPositionInRepository() const
    {
        return m_positionInRepository;
    }

    void SetPositionInRepository(double position_in_repository)
    {
        m_positionInRepository = position_in_repository;
    }

protected:
    CString m_key;
    double m_positionInRepository;
};
