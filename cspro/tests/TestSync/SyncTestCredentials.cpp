#include "stdafx.h"
#include "SyncTestCredentials.h"
#include "CaseTestHelpers.h"


/* ------------ the credentials file should contain sync connection strings like:
{
  "csweb": {
    "url": "...",
    "username": "...",
    "password": "..."
  },
  "ftp": {
    <same format as above>
  }
}
------------ */


std::map<std::string, SyncTestCredentials::Credentials> SyncTestCredentials::m_credentials;


SyncTestCredentials::SyncTestCredentials()
{
    if( !m_credentials.empty() )
        return;

    try
    {
        const JsonNode json_node = Json::ParseFile(GetCredentialsFilePath());

        json_node.ForeachNode(
            [&](const std::string_view key_sv, const JsonNode& value_node)
            {
                SyncConnectionString sync_connection_string = value_node.Get<SyncConnectionString>();
                std::optional<UsernamePassword> username_password = LoginCredentials::GetEvaluatedUsernamePassword(nullptr, sync_connection_string);

                if( username_password.has_value() )
                {
                    m_credentials.emplace(std::string(key_sv),
                                          Credentials { std::move(sync_connection_string), std::move(*username_password) });
                }
            });
    }
    catch(...) { }
}


std::string SyncTestCredentials::GetCredentialsFilePath()
{
    return Path::Combine(GetTestFilesDirectory(), "Server Credentials.json");
}


const SyncTestCredentials::Credentials& SyncTestCredentials::GetCredentials(const std::string& name) const
{
    const auto& lookup = m_credentials.find(name);

    if( lookup == m_credentials.cend() )
        Assert::Fail(TC::ToWide(FormatText("No credentials defined for '%s'. Define credentials here: %s", name.c_str(), GetCredentialsFilePath().c_str())).c_str());

    return lookup->second;
}


const SyncTestCredentials::Credentials& SyncTestCredentials::GetCredentialsCSWeb(const bool v2/* = DefaultToCSWebApiV2*/) const
{
    return GetCredentials(v2 ? "csweb-v2" : "csweb");
}


const SyncTestCredentials::Credentials& SyncTestCredentials::GetCredentialsFtp() const
{
    return GetCredentials("ftp");
}
