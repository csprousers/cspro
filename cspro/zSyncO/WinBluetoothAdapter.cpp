#include "stdafx.h"
#include "WinBluetoothAdapter.h"
#include "IObexTransport.h"
#include "ISyncListener.h"
#include "SyncException.h"
#include "WinBluetoothNameSetter.h"
#include "WinBluetoothScanner.h"
#include "WinObexBluetoothTransport.h"
#include <zUtilO/WinBluetoothFunctions.h>
#include <chrono>
#include <future>
#include <iterator>
#include <thread>
#include <BluetoothAPIs.h>
#include <Winsock2.h>
#include <Ws2bth.h>


namespace {

    // Enable bluetooth discovery and incoming connections then restore
    // previous state on destruction (RAII pattern).
    class BluetoothRadioEnabler {
    public:
        BluetoothRadioEnabler(std::shared_ptr<WinBluetoothFunctions> pBtFuncs)
            : m_pBtFuncs(pBtFuncs)
        {
            BLUETOOTH_FIND_RADIO_PARAMS btFindParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

            //Locate bluetooth radio
            HBLUETOOTH_RADIO_FIND btFind = m_pBtFuncs->BluetoothFindFirstRadio(&btFindParams, &m_hRadio);
            if (btFind == NULL) {
                throw SyncError(100101, L"Bluetooth is not enabled in system settings or this device does not support Bluetooth");
            }
            m_discoveryEnabled = m_pBtFuncs->BluetoothIsDiscoverable(m_hRadio) == TRUE;
            if (!m_discoveryEnabled)
                m_pBtFuncs->BluetoothEnableDiscovery(m_hRadio, true);
            m_incomingConnectionsEnabled = m_pBtFuncs->BluetoothIsConnectable(m_hRadio) == TRUE;
            if (!m_incomingConnectionsEnabled)
                m_pBtFuncs->BluetoothEnableIncomingConnections(m_hRadio, true);
        }

        ~BluetoothRadioEnabler()
        {
            if (!m_incomingConnectionsEnabled)
                m_pBtFuncs->BluetoothEnableIncomingConnections(m_hRadio, false);
            if (!m_discoveryEnabled)
                m_pBtFuncs->BluetoothEnableDiscovery(m_hRadio, false);
        }

    private:
        HANDLE m_hRadio;
        bool m_incomingConnectionsEnabled;
        bool m_discoveryEnabled;
        std::shared_ptr<WinBluetoothFunctions> m_pBtFuncs;
    };

    CString getWinsockErrorMessage(int err)
    {
        LPTSTR msgBuff = NULL;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,
            err,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            (LPTSTR)&msgBuff,
            0, NULL);
        CString msgString(msgBuff);
        LocalFree(msgBuff);
        return msgString;
    }

    /// <summary>Register/deregister Bluetooth service discovery protocol service</summary>
    /// To allow a client to connect to Bluetooth service you must register the service
    /// by it's service ID (GUID). Clients connecting specify the same GUID via service
    /// discovery protocol to connect to it. SDP essentially provides a mapping from
    /// the serviceId to the Bluetooth port to communicate on. This class creates
    /// a socket to listen for client connections and registers that socket with
    /// SDP. The service is unregistered on destruction of the object. It is important
    /// to unregister the service otherwise later client connections will fail.
    /// The listening socket is closed on destruction of the object and should not
    /// be closed by clients.
    class SDPService {
    public:

        SDPService()
            : m_listeningSocket(INVALID_SOCKET),
              m_pQuerySet(NULL),
              m_pAddrInfo(NULL),
              m_pSockAddrBthLocal(NULL),
              m_registered(false)
        {
        }

        SDPService(const SDPService&) = delete;
        SDPService& operator=(const SDPService&) = delete;

        /// <summary>Create the listening socket and register the service</summary>
        void registerService(GUID serviceId)
        {
            m_serviceId = serviceId;

            // Create a socket to listen on
            m_listeningSocket = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
            if (m_listeningSocket == INVALID_SOCKET) {
                throw SyncError(100101, getWinsockErrorMessage(WSAGetLastError()));
            }

            // Bind socket to blueooth, any port
            m_pSockAddrBthLocal = new SOCKADDR_BTH{ 0 };
            memset(m_pSockAddrBthLocal, 0, sizeof(*m_pSockAddrBthLocal));
            m_pSockAddrBthLocal->addressFamily = AF_BTH;
            m_pSockAddrBthLocal->btAddr = 0;
            m_pSockAddrBthLocal->serviceClassId = GUID_NULL;
            m_pSockAddrBthLocal->port = BT_PORT_ANY;


            if (bind(m_listeningSocket, (SOCKADDR*)m_pSockAddrBthLocal, sizeof(*m_pSockAddrBthLocal)) == SOCKET_ERROR) {
                throw SyncError(100101, getWinsockErrorMessage(WSAGetLastError()));
            }

            if (listen(m_listeningSocket, SOMAXCONN) == SOCKET_ERROR) {
                throw SyncError(100101, getWinsockErrorMessage(WSAGetLastError()));
            }

            // Get socket info
            int addrLen = sizeof(SOCKADDR_BTH);
            if (getsockname(m_listeningSocket, (SOCKADDR*)m_pSockAddrBthLocal, &addrLen) == SOCKET_ERROR) {
                throw SyncError(100101, getWinsockErrorMessage(WSAGetLastError()));
            }

            m_pAddrInfo = new CSADDR_INFO{ 0 };
            m_pAddrInfo->LocalAddr.iSockaddrLength = addrLen;
            m_pAddrInfo->LocalAddr.lpSockaddr = (LPSOCKADDR)m_pSockAddrBthLocal;
            m_pAddrInfo->iSocketType = SOCK_STREAM;
            m_pAddrInfo->iProtocol = BTHPROTO_RFCOMM;

            // Advertise service listening on the socket
            m_pQuerySet = new WSAQUERYSET{ 0 };
            m_pQuerySet->dwSize = sizeof(*m_pQuerySet);
            m_pQuerySet->lpszServiceInstanceName = L"CSPro Bluetooth Peer to Peer Synchronization";
            m_pQuerySet->lpszComment = NULL;
            m_pQuerySet->lpServiceClassId = (LPGUID)&m_serviceId;
            m_pQuerySet->dwNameSpace = NS_BTH;
            m_pQuerySet->dwNumberOfCsAddrs = 1;
            m_pQuerySet->lpcsaBuffer = m_pAddrInfo;

            if (WSASetService(m_pQuerySet, RNRSERVICE_REGISTER, 0) == SOCKET_ERROR) {
                throw SyncError(100101, getWinsockErrorMessage(WSAGetLastError()));
            }

            m_registered = true;
        }

        ~SDPService()
        {
            if (m_registered)
                unregisterService();
            if (m_listeningSocket != INVALID_SOCKET)
                closesocket(m_listeningSocket);
            if (m_pQuerySet)
                delete m_pQuerySet;
            if (m_pAddrInfo)
                delete m_pAddrInfo;
            if (m_pSockAddrBthLocal)
                delete m_pSockAddrBthLocal;
        }

        /// <summary>Get the socket created during register to listen for clients on</summary>
        SOCKET getListeningSocket() const
        {
            return m_listeningSocket;
        }

    private:
        void unregisterService()
        {
            if (WSASetService(m_pQuerySet, RNRSERVICE_DELETE, 0) == SOCKET_ERROR) {
                throw SyncError(100101, getWinsockErrorMessage(WSAGetLastError()));
            }
        }

        GUID m_serviceId;
        WSAQUERYSET* m_pQuerySet;
        CSADDR_INFO* m_pAddrInfo;
        SOCKADDR_BTH* m_pSockAddrBthLocal;
        SOCKET m_listeningSocket;
        bool m_registered;
    };

    bool scanForRemoteDevice(WinBluetoothScanner* pScanner, CString remoteDeviceName, PSOCKADDR_BTH pAddress, ISyncListener* pListener)
    {
        // Scanning is done in a background thread in the WinBluetoothScanner since it is a time consuming operation
        // and we want to be able to cancel it.

        std::atomic_bool doneFlag{ false };
        std::unique_ptr<SyncError> pScanError;

        // Set a scan listener that will copy bt address and set doneFlag to true if it finds the device in the scan results
        pScanner->setResultCallback([remoteDeviceName, pAddress, &doneFlag](const WinBluetoothScanner::DeviceList& dl) {
            auto i = std::find_if(dl.begin(), dl.end(), [remoteDeviceName](const BluetoothDeviceInfo& d) { return d.csName == remoteDeviceName;});
            if (i != dl.end()) {
                int addrSize = sizeof(*pAddress);
                LPTSTR addrString = const_cast<LPTSTR>((LPCTSTR) i->csAddress);
                WSAStringToAddress(addrString, AF_BTH, NULL, (LPSOCKADDR) pAddress, &addrSize);
                doneFlag = true;
            }
        });
        pScanner->setErrorCallback([&pScanError, &doneFlag](const SyncError& e) {
            pScanError = std::unique_ptr<SyncError>(new SyncError(e));
            doneFlag = true;
        });

        // Start the scan thread
        pScanner->startScan();

        // Wait for scan in background thread to complete or for 30 second timeout
        // checking for cancel 10 times/second
        const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
        const int keepOnTryingForSeconds = 30; // this is approximate since lookup takes a good chunk of time
        while (!doneFlag && (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startTime).count() < keepOnTryingForSeconds)) {
            if (pListener) {
                pListener->onProgress();
                if (pListener->isCancelled()) {
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        // Stop the background thread
        pScanner->stopScan();
        pScanner->setResultCallback(std::function<void(const WinBluetoothScanner::DeviceList&)>());
        pScanner->setErrorCallback(std::function<void(const SyncError&)>());

        if (pScanError)
            throw SyncError(*pScanError);

        return (doneFlag && (!pListener || !pListener->isCancelled()));
    }

    SOCKET connectToDevice(PSOCKADDR_BTH pAddr, GUID serviceUuid)
    {
        // Create a socket to use for connecting
        SOCKET sock = socket(AF_BTH, SOCK_STREAM, BTHPROTO_RFCOMM);
        if (INVALID_SOCKET == sock) {
            return INVALID_SOCKET;
        }

        // Connect
        SOCKADDR_BTH sockAddr = *pAddr;
        sockAddr.addressFamily = AF_BTH;
        sockAddr.serviceClassId = serviceUuid;
        sockAddr.port = 0;

        if (connect(sock, (struct sockaddr *) &sockAddr, sizeof(SOCKADDR_BTH)) == SOCKET_ERROR) {
            closesocket(sock);
            return INVALID_SOCKET;
        }

        return sock;
    }

    SOCKET connectToDeviceWithCancel(PSOCKADDR_BTH pAddr, GUID serviceUuid, ISyncListener* pListener)
    {
        SOCKET result = INVALID_SOCKET;

        const std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
        const int keepOnTryingForSeconds = 30; // this is approximate since lookup takes a good chunk of time
        while (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - startTime).count() < keepOnTryingForSeconds) {

            std::future<SOCKET> future = std::async(std::launch::async, [pAddr, serviceUuid]() {
                return connectToDevice(pAddr, serviceUuid);
            });

            // Wait for connect in background thread to complete
            // checking for cancel 10 times/second
            std::future_status status;
            do {
                status = future.wait_for(std::chrono::milliseconds(100));
                if (pListener) {
                    pListener->onProgress();
                    if (pListener->isCancelled()) {
                        // Request that thread abort but keep running loop
                        // so that UI stays responsive while we wait for thread to exit.
                        throw SyncCancelException();
                    }
                }
            } while (status != std::future_status::ready);

            result = future.get();
            if (result != INVALID_SOCKET)
                return result;
        }

        return INVALID_SOCKET;
    }
}


WinBluetoothAdapter::WinBluetoothAdapter(std::shared_ptr<WinBluetoothFunctions> pBtFuncs) :
    m_pBtFuncs(pBtFuncs)
{
    // Intitialize winsock (must do this at least once per DLL/exe, ok to do multiple times)
    WSADATA wsaData;
    WSAStartup(0x202, &wsaData); // request winsock v2.2

    m_pScanner = new WinBluetoothScanner(m_pBtFuncs);
}

WinBluetoothAdapter* WinBluetoothAdapter::create()
{
    auto pBtFuncs = WinBluetoothFunctions::instance();

    // Failed to load Bluetooth DLLs, machine does not have Bluetooth support
    // (e.g. Windows Server OS)
    if (!pBtFuncs)
        return nullptr;

    return new WinBluetoothAdapter(pBtFuncs);
}

WinBluetoothAdapter::~WinBluetoothAdapter()
{
    delete m_pScanner;

    // Cleanup winsock
    WSACleanup();
}

IObexTransport* WinBluetoothAdapter::connectToRemoteDevice(CString remoteDeviceName,
    CString remoteDeviceAddress, GUID service, ISyncListener* pListener /*= NULL */)
{
    SOCKADDR_BTH deviceAddress;

    if (remoteDeviceAddress.IsEmpty()) {
        // Lookup device from name
        bool bFound;

        // First check the last scan result before starting a new scan
        WinBluetoothScanner::DeviceList lastScanResult = m_pScanner->getLastScanResult();
        auto i = std::find_if(lastScanResult.begin(), lastScanResult.end(), [remoteDeviceName](const BluetoothDeviceInfo& d) { return d.csName == remoteDeviceName;});
        if (i != lastScanResult.end()) {
            int addrSize = sizeof(deviceAddress);
            LPTSTR addrString = const_cast<LPTSTR>((LPCTSTR) i->csAddress);
            WSAStringToAddress(addrString, AF_BTH, NULL, (LPSOCKADDR) &deviceAddress, &addrSize);
            bFound = true;
        } else {
            // Not in last scan, do a new scan
            bFound = scanForRemoteDevice(m_pScanner, remoteDeviceName, &deviceAddress, pListener);
        }

        if (!bFound)
            return NULL;
    } else {
        int addrSize = sizeof(deviceAddress);
        WSAStringToAddress(remoteDeviceAddress.GetBuffer(), AF_BTH, NULL, (LPSOCKADDR) &deviceAddress, &addrSize);
    }

    SOCKET socket = connectToDeviceWithCancel(&deviceAddress, service, pListener);
    if (socket == INVALID_SOCKET)
        return NULL;

    return new WinObexBluetoothTransport(socket);
}

IObexTransport* WinBluetoothAdapter::acceptConnection(
    GUID serviceId,
    ISyncListener* pListener /*= NULL*/)
{
    // Enable bluetooth and restore to previous state on destruction
    BluetoothRadioEnabler enableRadio(m_pBtFuncs);

    SDPService sdpService;
    sdpService.registerService(serviceId);

    // Wait for a connection on the socket
    SOCKET clientSocket = INVALID_SOCKET;
    fd_set readSet;
    timeval timeout;
    while (true) {

        if (pListener) {
            pListener->onProgress();
            if (pListener->isCancelled()) {
                throw SyncCancelException();
            }
        }

        // Select modifies the readSet so we need to recreate it each time
        // through the loop
        FD_ZERO(&readSet);
        FD_SET(sdpService.getListeningSocket(), &readSet);
        timeout.tv_sec = 0;
        timeout.tv_usec = 500000; // 0.5 secs
        int selectResult = select(sdpService.getListeningSocket(), &readSet, NULL, NULL, &timeout);
        if (selectResult == 1)
        {
            // client connected to socket, can accept without blocking
            break;
        }
        else if (selectResult == SOCKET_ERROR) {
            // Error
            throw SyncError(100101, getWinsockErrorMessage(WSAGetLastError()));
        }

        // If we get here selectResult is zero which means that select timed out
        // so we try again
    }

    clientSocket = accept(sdpService.getListeningSocket(), NULL, 0);
    if (clientSocket == INVALID_SOCKET) {
        throw SyncError(100101, getWinsockErrorMessage(WSAGetLastError()));
    }

    return new WinObexBluetoothTransport(clientSocket);
}

void WinBluetoothAdapter::enable()
{
    // On Windows always enabled
}

void WinBluetoothAdapter::disable()
{
    // On Windows always enabled
}

bool WinBluetoothAdapter::isEnabled() const
{
    // On Windows always enabled
    return true;
}

WinBluetoothScanner* WinBluetoothAdapter::scanner()
{
    return m_pScanner;
}

std::wstring WinBluetoothAdapter::getName() const
{
    BLUETOOTH_FIND_RADIO_PARAMS btFindParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };

    // Locate bluetooth radio
    HANDLE hRadio;

    if( m_pBtFuncs->BluetoothFindFirstRadio(&btFindParams, &hRadio) != nullptr )
    {
        BLUETOOTH_RADIO_INFO radioInfo = { sizeof(BLUETOOTH_RADIO_INFO) };

        if( m_pBtFuncs->BluetoothGetRadioInfo(hRadio, &radioInfo) == ERROR_SUCCESS )
            return radioInfo.szName;
    }

    return std::wstring();
}

void WinBluetoothAdapter::setName(const CString& bluetooth_name)
{
    if( SetBluetoothName(bluetooth_name) )
    {
        // the name change does not immediately occur so loop until getName returns successfully, waiting up to 5 seconds
        constexpr int MaxWaitTimesSeconds = 5;
        auto start_time = std::chrono::steady_clock::now();

        while( !SO::Equals(getName(), bluetooth_name) )
        {
            if( std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - start_time).count() > MaxWaitTimesSeconds )
                throw CSProException("The name was changed but did not seem to take effect.");

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }

    else
    {
        throw CSProException(_T("Unable to access the Bluetooth device or modify the registry.%s"),
                             IsUserAnAdmin() ? _T("") : _T(" This operation may require admin rights."));
    }
}
