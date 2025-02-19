#pragma once

#include <zNetwork/zNetwork.h>
#include <zNetwork/FileBasedConnection.h>
#include <zNetwork/LoginCredentials.h>


// FTP client base class:
// Supports standard FTP operations including download, upload, directory listing

class ZNETWORK_API FtpConnection : public FileBasedConnection
{
public:
    virtual ~FtpConnection() { }

    bool IsConnected() const { return !m_url.empty(); }

    void Connect(const SyncConnectionString& sync_connection_string, const LoginCredentials& login_credentials, std::string* out_username = nullptr);
    void Disconnect();

    const std::string& GetProvidedUrl() const { return m_providedUrl; }

protected:
    // return the adjusted URL that is actually used for the connection
    virtual std::string DoConnect(const std::string& username, const std::string& password) = 0;
    virtual void DoDisconnect() = 0;

private:
    void ConnectUsingUsernamePassword(const UsernamePassword& username_password, std::string* out_username);
    void ConnectUsingLoginDialogOrSavedCredentials(const LoginCredentials& login_credentials, const std::string& credential_attribute, std::string* out_username);

protected:
    std::string m_providedUrl; // the URL provided by the user
    std::string m_url;         // the adjusted URL provided by a subclass
    std::string m_urlPath;     // the URL's path
};
