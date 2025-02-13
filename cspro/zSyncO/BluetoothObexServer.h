#pragma once
#include <zSyncO/zSyncO.h>
class SyncObexHandler;
struct IBluetoothAdapter;
struct ISyncListener;

///<summary>Main class for running server side of bluetooth peer to peer sync</summary>
class SYNC_API BluetoothObexServer {

public:

    BluetoothObexServer(CString deviceId, IBluetoothAdapter* pAdapter, SyncObexHandler* pHandler);

    ///<summary>Start the server, wait for incomming connection and process requests</summary>
    int run();

    ///<summary>Set callbacks for reporting errors and progress</summary>
    void setListener(ISyncListener *pListener);

private:
    SyncObexHandler* m_pHandler;
    IBluetoothAdapter* m_pAdapter;
    ISyncListener* m_pListener;
};