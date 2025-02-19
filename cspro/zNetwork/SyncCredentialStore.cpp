#include "stdafx.h"
#include "SyncCredentialStore.h"
#include "OAuth2Token.h"


// --------------------------------------------------------------------------
// SyncCredentialStore
// --------------------------------------------------------------------------

SyncCredentialStore::OAuth2TokenCredentialManager SyncCredentialStore::CreateOAuth2TokenCredentialManager(std::string credential_attribute)
{
    return OAuth2TokenCredentialManager(*this, std::move(credential_attribute));
}


// --------------------------------------------------------------------------
// SyncCredentialStore::OAuth2TokenCredentialManager
// --------------------------------------------------------------------------

struct SyncCredentialStore::OAuth2TokenCredentialManager::Data
{
    SyncCredentialStore& sync_credential_store;
    std::string credential_attribute;
    std::vector<std::tuple<std::string, OAuth2Token>> email_and_oauth2_tokens;
};


SyncCredentialStore::OAuth2TokenCredentialManager::OAuth2TokenCredentialManager(SyncCredentialStore& sync_credential_store, std::string credential_attribute)
    :   m_data(std::make_unique<Data>(Data { sync_credential_store, std::move(credential_attribute) }))
{
    ASSERT(!m_data->credential_attribute.empty());

    // read any existing tokens
    const std::string previous_oauth2_tokens_json = m_data->sync_credential_store.Retrieve(m_data->credential_attribute);

    if( !previous_oauth2_tokens_json.empty() )
    {
        try
        {
            const JsonNode previous_oauth2_tokens_json_node = Json::Parse(previous_oauth2_tokens_json);

            for( const JsonNode& previous_oauth2_token_json_node : previous_oauth2_tokens_json_node.GetArray() )
            {
                m_data->email_and_oauth2_tokens.emplace_back(previous_oauth2_token_json_node.Get<std::string>(JK::email),
                                                             previous_oauth2_token_json_node.Get<OAuth2Token>(JK::token));
            }
        }
        catch(...) { } // ignore errors parsing the JSON
    }
}


SyncCredentialStore::OAuth2TokenCredentialManager::~OAuth2TokenCredentialManager()
{
}


auto SyncCredentialStore::OAuth2TokenCredentialManager::LookupToken(const std::string* const email) const noexcept
{
    if( m_data->email_and_oauth2_tokens.empty() )
        return m_data->email_and_oauth2_tokens.cend();

    // if not using an email, use the last-added token
    if( email == nullptr )
        return m_data->email_and_oauth2_tokens.cbegin() + m_data->email_and_oauth2_tokens.size() - 1;

    // otherwise search for the correct email
    return std::find_if(m_data->email_and_oauth2_tokens.cbegin(), m_data->email_and_oauth2_tokens.cend(),
                        [&](const auto& email_and_oauth2_token) { return ( *email == std::get<0>(email_and_oauth2_token) ); });
}


std::optional<OAuth2Token> SyncCredentialStore::OAuth2TokenCredentialManager::GetToken(const std::string* const email) const noexcept
{
    const auto lookup = LookupToken(email);

    if( lookup != m_data->email_and_oauth2_tokens.cend() )
        return std::get<1>(*lookup);

    return std::nullopt;
}


void SyncCredentialStore::OAuth2TokenCredentialManager::SaveToken(std::string email, OAuth2Token oauth2_token) noexcept
{
    // erase the existing token
    const auto lookup = LookupToken(&email);

    if( lookup != m_data->email_and_oauth2_tokens.cend() )
        m_data->email_and_oauth2_tokens.erase(lookup);

    // add the new token at the end
    m_data->email_and_oauth2_tokens.emplace_back(std::move(email), std::move(oauth2_token));

    // update the credentials
    try
    {
        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

        json_writer->WriteObjects(m_data->email_and_oauth2_tokens,
            [&](const auto& email_and_oauth2_token)
            {
                json_writer->Write(JK::email, std::get<0>(email_and_oauth2_token))
                            .Write(JK::token, std::get<1>(email_and_oauth2_token));
            });

        m_data->sync_credential_store.Store(m_data->credential_attribute, json_writer->GetString());
    }
    catch(...) { ASSERT(false); }
}
