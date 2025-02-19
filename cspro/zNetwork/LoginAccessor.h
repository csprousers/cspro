#pragma once

#include <zNetwork/zNetwork.h>

struct BluetoothDeviceInfo;
class FtpConnection;
class HttpConnection;
class IBluetoothAdapter;
struct UsernamePassword;
class SyncCredentialStore;


// --------------------------------------------------------------------------
// LoginAccessor
// --------------------------------------------------------------------------

class ZNETWORK_API LoginAccessor
{
public:
    virtual ~LoginAccessor() { }

    // Returns the non-null credential store for sync-related credentials.
    // The default implementation returns an instance of SyncCredentialStore.
    virtual std::shared_ptr<SyncCredentialStore> GetSyncCredentialStore();


    // Returns a Bluetooth adapter, returning null if one cannot be created.
    virtual std::shared_ptr<IBluetoothAdapter> GetBluetoothAdapter() = 0;

    // Creates a FtpConnection object, returning null if the platform does not support FTP access.
    // The default implementation returns a platform-specific instance of FtpConnection.
    virtual std::unique_ptr<FtpConnection> CreateFtpConnection();

    // Creates a HttpConnection object, returning null if the platform does not support HTTP access.
    // The default implementation returns a platform-specific instance of HttpConnection.
    virtual std::unique_ptr<HttpConnection> CreateHttpConnection();


    // Prompts the user for username/password credentials:
    // - server: name of server to log into;
    // - show_invalid_error: if true, display an error message that the username/password is invalid.
    // Returns the username/password if credentials are entered or std::nullopt if canceled.
    // The default implementation displays a login dialog.
    virtual std::optional<UsernamePassword> QueryUsernamePassword(const std::string& server, bool show_invalid_error);

    // Prompts the user to choose from a nearby Bluetooth device.
    // Returns the device info if a connection is made or std::nullopt if canceled.
    virtual std::optional<BluetoothDeviceInfo> ChooseBluetoothDevice() = 0;


    // Sets a routine for QueryUsernamePassword to be used by Excel2CSPro.
    struct WinFormsQueryUsernamePassword { virtual ~WinFormsQueryUsernamePassword() { }
                                           virtual std::optional<UsernamePassword> QueryUsernamePassword(bool show_invalid_error) = 0; };
    static void SetWinFormsQueryUsernamePasswordCallback(WinFormsQueryUsernamePassword* callback);
};



// --------------------------------------------------------------------------
// LoginAccessorWithoutBluetoothSupport
// --------------------------------------------------------------------------

class ZNETWORK_API LoginAccessorWithoutBluetoothSupport : public LoginAccessor
{
private:
    std::shared_ptr<IBluetoothAdapter> GetBluetoothAdapter() override;
    std::optional<BluetoothDeviceInfo> ChooseBluetoothDevice() override;
};
