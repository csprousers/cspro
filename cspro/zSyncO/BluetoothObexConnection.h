#pragma once

#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/ObexConstants.h>

class HeaderList;
class IBluetoothAdapter;
class IDataChunk;
struct IObexTransport;
class ObexClient;
class SyncListener;


//  Client side connection for smart sync over Bluetooth using Obex

class BluetoothObexConnection
{
public:
    BluetoothObexConnection(std::shared_ptr<IBluetoothAdapter> pAdapter);
    ~BluetoothObexConnection();

    bool connect(const BluetoothDeviceInfo& deviceInfo);

    void disconnect();

    ObexResponseCode get(CString type, CString path, const HeaderList& requestHeaders, std::ostream& response, HeaderList& responseHeaders);

    ObexResponseCode put(CString type, CString path, bool isLastFileChunk, std::istream& content, size_t contentSize, const HeaderList& requestHeaders, std::ostream& response, HeaderList& responseHeaders);

    IDataChunk& getChunk();

    void SetSyncListener(std::shared_ptr<SyncListener> sync_listener) { m_syncListener = std::move(sync_listener); }

private:
    std::shared_ptr<IBluetoothAdapter> m_pAdapter;
    std::unique_ptr<IObexTransport> m_pTransport;
    std::unique_ptr<ObexClient> m_pObexClient;
    std::shared_ptr<SyncListener> m_syncListener;
};
