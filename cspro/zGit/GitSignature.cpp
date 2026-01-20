#include "StdAfx.h"
#include "GitSignature.h"


GitSignature::GitSignature(std::string name, std::string email, const git_time& when)
    :   m_name(std::move(name)),
        m_email(std::move(email)),
        m_wrapper{ m_name.c_str(), m_email.c_str(), when }
{
    static_assert(sizeof(m_wrapper) == sizeof(git_signature));
    ASSERT(m_name.find_first_of("<>") == std::string::npos);
    ASSERT(m_email.find_first_of("<>") == std::string::npos);
}


GitSignature::GitSignature(const git_signature& signature)
    :   GitSignature(signature.name, signature.email, signature.when)
{
}


GitSignature GitSignature::Create(std::string name, std::string email)
{
    git_time when;
    when.time = GetTimestamp();
    when.offset = DateTime::GetUtcOffsetNow();
    when.sign = ( when.offset >= 0 ) ? '+' : '-';

    return GitSignature(std::move(name), std::move(email), when);
}


bool GitSignature::operator==(const GitSignature& rhs) const noexcept
{
    // 'when' is compared first as that is the most likely to be different
    return ( m_wrapper.when == rhs.m_wrapper.when &&
             m_name == rhs.m_name &&
             m_email == rhs.m_email );
}


std::string GitSignature::GetDisplayString() const noexcept
{
    return SO::Concatenate(m_name, " <", m_email, ">");
}
