#include "stdafx.h"
#include "BluetoothObexServer.h"
#include "IBluetoothAdapter.h"
#include "IObexTransport.h"
#include "ObexConstants.h"
#include "ObexServer.h"
#include "SyncObexHandler.h"
#include <zNetwork/SyncListenerRAII.h>


namespace
{
    class BluetoothEnabler
    {
    public:
        BluetoothEnabler(IBluetoothAdapter* pAdapter)
            :   m_bWasEnabled(pAdapter->IsEnabled()),
                m_pAdapter(pAdapter)
        {
            //if (!m_bWasEnabled)
                m_pAdapter->Enable();
        }

        ~BluetoothEnabler()
        {
            if (!m_bWasEnabled)
                m_pAdapter->Disable();
        }

    private:
        bool m_bWasEnabled;
        IBluetoothAdapter* m_pAdapter;
    };
}


BluetoothObexServer::BluetoothObexServer(std::shared_ptr<IBluetoothAdapter> pAdapter, std::unique_ptr<SyncObexHandler> pHandler,
                                         std::shared_ptr<SyncListener> sync_listener)
    :   m_pAdapter(std::move(pAdapter)),
        m_pHandler(std::move(pHandler)),
        m_syncListener(std::move(sync_listener))
{
    ASSERT(m_pAdapter != nullptr && m_pHandler != nullptr);
}


int BluetoothObexServer::run()
{
    try {
        const SyncListenerCloser sync_listener_closer(m_syncListener.get());

        // Waiting for connections...
        if (m_syncListener != nullptr) {
            m_syncListener->Start(100105);
        }

        BluetoothEnabler btEnabler(m_pAdapter.get());

        std::unique_ptr<IObexTransport> pTransport(m_pAdapter->AcceptConnection(OBEX_SYNC_SERVICE_UUID, m_syncListener.get()));

        if (!pTransport.get())
            return 0; // Cancelled

        // Connected
        if (m_syncListener != nullptr) {
            m_syncListener->Progress(0, 100106);
        }

        ObexServer obexServer(m_pHandler.get());
        obexServer.SetSyncListener(m_syncListener);
        obexServer.run(pTransport.get());
        return 1;
    }
    catch (const SyncError& e) {
        if (m_syncListener != nullptr) {
            m_syncListener->ReportError(e);
        }
    }
    catch (const SyncCancelException&) {
    }
    return 0;
}
