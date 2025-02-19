#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/LoginAccessor.h>
#include <zNetwork/UsernamePassword.h>

class SyncConnectionString;


// --------------------------------------------------------------------------
// LoginCredentials
// --------------------------------------------------------------------------

class ZNETWORK_API LoginCredentials
{
public:
    LoginCredentials(UsernamePassword username_password);
    LoginCredentials(std::string username, std::string password);
    LoginCredentials(std::shared_ptr<LoginAccessor> login_accessor);

    LoginCredentials(const LoginCredentials&) = delete;
    LoginCredentials(LoginCredentials&&) = default;

    bool UsesUsernamePassword() const { return ( m_data.index() == 0 ); }
    bool UseLoginAccessor() const     { return ( m_data.index() == 1 ); }

    LoginAccessor& GetLoginAccessor() const                       { ASSERT(UseLoginAccessor()); return *std::get<1>(m_data); }
    std::shared_ptr<LoginAccessor> GetSharedLoginAccessor() const { ASSERT(UseLoginAccessor()); return std::get<1>(m_data); }

    // If the login credentials object holds a UsernamePassword, it is returned.
    // If the sync connection string defines the username and password, a UsernamePassword object is created with those values.
    // Otherwise std::nullopt is returned.
    static std::optional<UsernamePassword> GetEvaluatedUsernamePassword(const LoginCredentials* login_credentials,
                                                                        const SyncConnectionString& sync_connection_string);
    std::optional<UsernamePassword> GetEvaluatedUsernamePassword(const SyncConnectionString& sync_connection_string) const;

private:
    std::variant<UsernamePassword, std::shared_ptr<LoginAccessor>> m_data;
};
