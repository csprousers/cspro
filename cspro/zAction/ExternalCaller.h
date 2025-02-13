#pragma once

#include <zAction/Caller.h>


namespace ActionInvoker
{
    class ExternalCaller : public ActionInvoker::Caller
    {
    public:
        // disables the need to use access tokens for the specified access token
        void AddAccessTokenOverride(std::wstring access_token)
        {
            ASSERT(m_accessTokenOverride == nullptr);
            m_accessTokenOverride = std::make_unique<std::wstring>(std::move(access_token));
        }

        // Caller overrides
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

        bool IsAccessTokenValid(const std::wstring& access_token) const override
        {
            ASSERT(!m_userOverrideAccessTokenRequirement.has_value());

            return ( m_accessTokenOverride != nullptr &&
                     access_token == *m_accessTokenOverride );
        }

    private:
        std::optional<bool> m_userOverrideAccessTokenRequirement;
        std::unique_ptr<std::wstring> m_accessTokenOverride;
    };
}
