#pragma once

#include <zNetwork/LoginCredentials.h>

static constexpr bool DefaultToCSWebApiV2 = false;


class SyncTestCredentials
{
public:
    struct Credentials { SyncConnectionString sync_connection_string; UsernamePassword username_password; };

    SyncTestCredentials();

    const Credentials& GetCredentials(const std::string& name) const;
    const Credentials& GetCredentialsCSWeb(bool v2 = DefaultToCSWebApiV2) const;
    const Credentials& GetCredentialsFtp() const;

private:
    static std::string GetCredentialsFilePath();

private:
    static std::map<std::string, Credentials> m_credentials;
};
