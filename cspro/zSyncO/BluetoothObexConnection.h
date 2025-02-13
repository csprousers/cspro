#pragma once

#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/ObexConstants.h>
#include <zNetwork/HeaderList.h>

class ObexClient;
struct IBluetoothAdapter;
struct IDataChunk;
struct IObexTransport;
struct ISyncListener;


/**
* Client side connection for smart sync over Bluetooth using Obex
*/
class BluetoothObexConnection {

public:

    BluetoothObexConnection(IBluetoothAdapter* pAdapter);

    ~BluetoothObexConnection();

    bool connect(const BluetoothDeviceInfo& deviceInfo);

    void disconnect();

    ObexResponseCode get(CString type, CString path, const HeaderList& requestHeaders, std::ostream& response, HeaderList& responseHeaders);

    ObexResponseCode put(CString type, CString path, bool isLastFileChunk, std::istream& content, size_t contentSize, const HeaderList& requestHeaders, std::ostream& response, HeaderList& responseHeaders);

    IDataChunk& getChunk();

    virtual void setListener(ISyncListener* pListener);

private:

    IObexTransport* m_pTransport;
    ObexClient* m_pObexClient;
    IBluetoothAdapter* m_pAdapter;
    ISyncListener* m_pListener;
};
