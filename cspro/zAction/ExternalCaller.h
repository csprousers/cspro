#pragma once

#include <zAction/Caller.h>

namespace ActionInvoker { class ExternalCaller; }


class ActionInvoker::ExternalCaller : public ActionInvoker::Caller
{
public:
    ExternalCaller(int caller_id)
        :   m_callerId(caller_id)
    {
    }

    // disables the need to use access tokens for the specified access token
    void AddAccessTokenOverride(SharableString access_token)
    {
        ASSERT(!m_accessTokenOverride.IsSet() && access_token.IsSet());
        m_accessTokenOverride = std::move(access_token);
    }

    // Caller overrides
    int GetCallerId() const override
    {
        return m_callerId;
    }

    bool IsExternalCaller() const override
    {
        return true;
    }

    std::optional<bool> GetUserOverrodeAccessTokenRequirement() const override
    {
        return m_userOverrideAccessTokenRequirement;
    }

    void SetUserOverrodeAccessTokenRequirement(bool allowed_access) override
    {
        m_userOverrideAccessTokenRequirement = allowed_access;
    };

    bool IsAccessTokenValid(const std::string& access_token) const override
    {
        ASSERT(!m_userOverrideAccessTokenRequirement.has_value());

        return ( m_accessTokenOverride.IsSet() &&
                 access_token == *m_accessTokenOverride );
    }

private:
    int m_callerId;
    std::optional<bool> m_userOverrideAccessTokenRequirement;
    SharableString m_accessTokenOverride;
};
