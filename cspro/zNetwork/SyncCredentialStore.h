#pragma once

#include <zNetwork/zNetwork.h>
#include <zUtilO/CredentialStore.h>

class OAuth2Token;


// --------------------------------------------------------------------------
// SyncCredentialStore
// --------------------------------------------------------------------------

class SyncCredentialStore : public CredentialStore
{
protected:
    std::string PrefixAttribute(const std::string& attribute) override
    {
        return "CSPro_sync_" + attribute;
    }

public:
    class OAuth2TokenCredentialManager;
    ZNETWORK_API OAuth2TokenCredentialManager CreateOAuth2TokenCredentialManager(std::string credential_attribute);
};



// --------------------------------------------------------------------------
// SyncCredentialStore::OAuth2TokenCredentialManager
// --------------------------------------------------------------------------

class ZNETWORK_API SyncCredentialStore::OAuth2TokenCredentialManager
{
    friend SyncCredentialStore;

private:
    OAuth2TokenCredentialManager(SyncCredentialStore& sync_credential_store, std::string credential_attribute);

public:
    ~OAuth2TokenCredentialManager();

    std::optional<OAuth2Token> GetToken(const std::string* email) const noexcept;

    void SaveToken(std::string email, OAuth2Token oauth2_token) noexcept;

private:
    auto LookupToken(const std::string* email) const noexcept;

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
