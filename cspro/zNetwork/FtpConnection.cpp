#include "stdafx.h"
#include "FtpConnection.h"
#include "ParsedUri.h"


void FtpConnection::Connect(const SyncConnectionString& sync_connection_string, const LoginCredentials& login_credentials, std::string* const out_username/* = nullptr*/)
{
    ASSERT(sync_connection_string.GetType() == SyncServiceType::Ftp);

    if( IsConnected() )
        Disconnect();

    ASSERT(!IsConnected());

    // default to FTP if no scheme provided
    const ParsedUri parsed_uri(PortableFunctions::PathEnsureTrailingForwardSlash(sync_connection_string.GetUrl()), "ftp");

    // check for an invalid URL, or an invalid scheme (probably http)
    if( parsed_uri.path.empty() || !SO::EqualsOneOfNoCase(parsed_uri.scheme, "ftp", "ftps", "ftpes") )
        throw SyncError(100125, sync_connection_string.GetUrl());

    m_providedUrl = parsed_uri.ToUri();
    m_urlPath = parsed_uri.path;

    const std::optional<UsernamePassword> username_password = login_credentials.GetEvaluatedUsernamePassword(sync_connection_string);

    if( username_password.has_value() )
    {
        ConnectUsingUsernamePassword(*username_password, out_username);
    }

    else
    {
        ConnectUsingLoginDialogOrSavedCredentials(login_credentials, parsed_uri.ToUriWithoutPath(), out_username);
    }

    ASSERT(IsConnected());
}


void FtpConnection::ConnectUsingUsernamePassword(const UsernamePassword& username_password, std::string* const out_username)
{
    ASSERT(!IsConnected());

    m_url = DoConnect(username_password.username, username_password.password);

    ASSERT(IsConnected());

    if( out_username != nullptr )
        *out_username = username_password.username;
}


void FtpConnection::ConnectUsingLoginDialogOrSavedCredentials(const LoginCredentials& login_credentials, const std::string& credential_attribute, std::string* const out_username)
{
    // try the saved username/password first
    const std::shared_ptr<SyncCredentialStore> sync_credential_store = login_credentials.GetLoginAccessor().GetSyncCredentialStore();
    ASSERT(sync_credential_store != nullptr);
    std::optional<UsernamePassword> username_password = sync_credential_store->RetrieveOptionalFromJson<UsernamePassword>(credential_attribute);

    if( username_password.has_value() )
    {
        try
        {
            ConnectUsingUsernamePassword(*username_password, out_username);
            return;
        }

        catch( const SyncLoginDeniedError& )
        {
            // ignore login denied (bad password) errors, pass all others on
        }

        catch( const SyncError& )
        {
            throw;
        }
    }

    // keep prompting for credentials until we get successful login, cancel or error
    bool had_failure = false;

    while( true )
    {
        username_password = login_credentials.GetLoginAccessor().QueryUsernamePassword(m_providedUrl, had_failure);

        if( !username_password.has_value() )
            throw SyncCancelException();

        try
        {
            ConnectUsingUsernamePassword(*username_password, out_username);

            // save the credentials upon a successful login
            sync_credential_store->StoreAsJson(credential_attribute, *username_password);

            return;
        }

        catch( const SyncLoginDeniedError& )
        {
            // ignore login denied (bad password) errors, pass all others on
            had_failure = true;
        }

        catch( const SyncError& )
        {
            throw;
        }
    }
}


void FtpConnection::Disconnect()
{
    if( IsConnected() )
    {
        DoDisconnect();
        m_url.clear();
    }
}
