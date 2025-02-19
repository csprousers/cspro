#pragma once

#include <zCaseO/zCaseO.h>

class Case;


class CaseKey
{
public:
    CaseKey(std::string key, double position_in_repository);
    CaseKey();

    virtual ~CaseKey() { }

    CaseKey(const CaseKey&) = default;
    CaseKey(CaseKey&&) noexcept = default;
    CaseKey(const Case& data_case);
    CaseKey(Case&& data_case);

    CaseKey& operator=(const CaseKey&) = default;
    CaseKey& operator=(CaseKey&&) = default;
    CaseKey& operator=(const Case& data_case);
    CaseKey& operator=(Case&& data_case);

    virtual const std::string& GetKey() const { return m_key; }
    virtual void SetKey(std::string key)      { m_key = std::move(key); }

    ZCASEO_API std::string GetSingleLineKey() const;

    // Returns a double that can be used to locate this case in the repository from which it was read.
    // The number is arbitrary and should not be used in any calculations. The number is initialized as -1.
    double GetPositionInRepository() const                      { return m_positionInRepository; }
    void SetPositionInRepository(double position_in_repository) { m_positionInRepository = position_in_repository; }

protected:
    std::string m_key;
    double m_positionInRepository;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CaseKey::CaseKey(std::string key, double position_in_repository)
    :   m_key(std::move(key)),
        m_positionInRepository(position_in_repository)
{
}


inline CaseKey::CaseKey()
    :   m_positionInRepository(-1)
{
}
