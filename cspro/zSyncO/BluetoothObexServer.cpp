#include "stdafx.h"
#include "ObexConstants.h"
#include "SyncException.h"
#include "BluetoothObexServer.h"
#include "SyncObexHandler.h"
#include "IObexTransport.h"
#include "ObexServer.h"
#include "IBluetoothAdapter.h"
#include "ISyncListener.h"

namespace {

    class BluetoothEnabler {

    public:
        BluetoothEnabler(IBluetoothAdapter* pAdapter)
            : m_bWasEnabled(pAdapter->isEnabled()),
            m_pAdapter(pAdapter)
        {
            //if (!m_bWasEnabled)
                m_pAdapter->enable();
        }

        ~BluetoothEnabler()
        {
            if (!m_bWasEnabled)
                m_pAdapter->disable();
        }
    private:
        bool m_bWasEnabled;
        IBluetoothAdapter* m_pAdapter;
    };

}
BluetoothObexServer::BluetoothObexServer(CString deviceId,
    IBluetoothAdapter* pAdapter,
    SyncObexHandler* pObexHandler)
    : m_pHandler(pObexHandler),
      m_pAdapter(pAdapter)
{
}

int BluetoothObexServer::run()
{
    try {
        SyncListenerCloser listenerCloser(m_pListener);

        // Waiting for connections...
        if (m_pListener) {
            m_pListener->onStart(100105);
        }

        BluetoothEnabler btEnabler(m_pAdapter);

        std::unique_ptr<IObexTransport> pTransport(m_pAdapter->acceptConnection(
            OBEX_SYNC_SERVICE_UUID, m_pListener));

        if (!pTransport.get())
            return 0; // Cancelled

        // Connected
        if (m_pListener)
            m_pListener->onProgress(0, 100106);

        ObexServer obexServer(m_pHandler);
        obexServer.setListener(m_pListener);
        obexServer.run(pTransport.get());
        return 1;
    } catch (const SyncError& e) {
        if (m_pListener) {
            m_pListener->onError(e.m_errorCode, e.GetErrorMessage().c_str());
        }
    }
    catch (const SyncCancelException&) {
    }
    return 0;
}

void BluetoothObexServer::setListener(ISyncListener *pListener)
{
    m_pListener = pListener;
}
