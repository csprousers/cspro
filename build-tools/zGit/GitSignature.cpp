#include "StdAfx.h"
#include "GitSignature.h"


GitSignature::GitSignature(const git_signature& signature) noexcept
    :   m_name(signature.name),
        m_email(signature.email),
        m_when(signature.when)
{
}


bool GitSignature::operator==(const GitSignature& rhs) const noexcept
{
    // 'when' is compared first as that is the most likely to be different
    return ( m_when == rhs.m_when &&
             m_name == rhs.m_name &&
             m_email == rhs.m_email );
}


std::string GitSignature::GetDisplayString() const noexcept
{
    return SO::Concatenate(m_name, " <", m_email, ">");
}
