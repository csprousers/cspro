#pragma once

#include <zSyncO/zSyncO.h>

class IBluetoothAdapter;
class SyncListener;
class SyncObexHandler;


// Main class for running server side of bluetooth peer to peer sync

class SYNC_API BluetoothObexServer
{
public:
    BluetoothObexServer(std::shared_ptr<IBluetoothAdapter> pAdapter, std::unique_ptr<SyncObexHandler> pHandler,
                        std::shared_ptr<SyncListener> sync_listener);

    // Start the server, wait for incomming connection and process requests
    int run();

private:
    std::shared_ptr<IBluetoothAdapter> m_pAdapter;
    std::unique_ptr<SyncObexHandler> m_pHandler;
    std::shared_ptr<SyncListener> m_syncListener;
};
