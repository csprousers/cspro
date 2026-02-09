#include "StdAfx.h"
#include "GitSignature.h"


GitSignature::GitSignature(SharableString name, SharableString email, const git_time& when) noexcept
    :   m_name(std::move(name)),
        m_email(std::move(email)),
        m_wrapper{ m_name->c_str(), m_email->c_str(), when }
{
    static_assert(sizeof(m_wrapper) == sizeof(git_signature));
    ASSERT(m_name->find_first_of("<>") == std::string::npos);
    ASSERT(m_email->find_first_of("<>") == std::string::npos);
}


GitSignature::GitSignature(const git_signature& signature) noexcept
    :   GitSignature(signature.name, signature.email, signature.when)
{
}


GitSignature GitSignature::Create(SharableString name, SharableString email) noexcept
{
    git_time when;
    when.time = GetTimestamp();
    when.offset = DateTime::GetUtcOffsetNow();
    when.sign = ( when.offset >= 0 ) ? '+' : '-';

    return GitSignature(std::move(name), std::move(email), when);
}


GitSignature GitSignature::CreateDefault(GitRepository& repo)
{
    repo.EnsureRepositoryIsOpen();

    git_signature* sig;

    if( git_signature_default(&sig, repo) != 0 )
        throw GitException();

    GitSignature signature(sig->name, sig->email, sig->when);

    git_signature_free(sig);

    return signature;
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
    return SO::Concatenate(*m_name, " <", *m_email, ">");
}
