#include "stdafx.h"
#include "LoginCredentials.h"


LoginCredentials::LoginCredentials(UsernamePassword username_password)
    :   m_data(std::move(username_password))
{
}


LoginCredentials::LoginCredentials(std::string username, std::string password)
    :   m_data(UsernamePassword { std::move(username), std::move(password) })
{
}


LoginCredentials::LoginCredentials::LoginCredentials(std::shared_ptr<LoginAccessor> login_accessor)
    :   m_data(std::move(login_accessor))
{
    ASSERT(std::get<1>(m_data) != nullptr);
}


std::optional<UsernamePassword> LoginCredentials::GetEvaluatedUsernamePassword(const LoginCredentials* const login_credentials,
                                                                               const SyncConnectionString& sync_connection_string)
{
    if( login_credentials != nullptr && login_credentials->UsesUsernamePassword() )
        return std::get<UsernamePassword>(login_credentials->m_data);

    const std::string* const username = sync_connection_string.GetProperty(SCSProperty::username);

    if( username != nullptr )
    {
        const std::string* const password =  sync_connection_string.GetProperty(SCSProperty::password);

        if( password != nullptr )
            return UsernamePassword { *username, *password };
    }

    return std::nullopt;
}


std::optional<UsernamePassword> LoginCredentials::GetEvaluatedUsernamePassword(const SyncConnectionString& sync_connection_string) const
{
    return GetEvaluatedUsernamePassword(this, sync_connection_string);
}
