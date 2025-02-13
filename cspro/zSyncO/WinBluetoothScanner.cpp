#include "stdafx.h"
#include "WinBluetoothScanner.h"
#include <BluetoothAPIs.h>
#include <zUtilO/WinBluetoothFunctions.h>

namespace {

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

    WinBluetoothScanner::DeviceList scanForRemoteDevices(std::shared_ptr<WinBluetoothFunctions> pBtFuncs)
    {
        WinBluetoothScanner::DeviceList results;

        // Check to see if there is a bluetooth radio on the device
        BLUETOOTH_FIND_RADIO_PARAMS btFindParams = { sizeof(BLUETOOTH_FIND_RADIO_PARAMS) };
        HANDLE hRadio;
        HBLUETOOTH_RADIO_FIND btFind = pBtFuncs->BluetoothFindFirstRadio(&btFindParams, &hRadio);
        if (btFind == NULL) {
            throw SyncError(100101, L"Bluetooth is not enabled in system settings or this device does not support Bluetooth");
        }

        WSAQUERYSET restrictions;
        ZeroMemory(&restrictions, sizeof(restrictions));
        restrictions.dwNameSpace = NS_BTH;
        restrictions.dwSize = sizeof(restrictions);
        DWORD dwControlFlags = LUP_CONTAINERS | LUP_RETURN_NAME | LUP_RETURN_ADDR | LUP_FLUSHCACHE;

        DWORD dwBuffSize = sizeof(WSAQUERYSET);
        std::vector<BYTE> buffer(dwBuffSize);
        WSAQUERYSET* pQuerySet = (WSAQUERYSET*)&buffer[0];

        // Start lookup
        HANDLE hLookup;
        if (WSALookupServiceBegin(&restrictions, dwControlFlags, &hLookup) != 0) {

            int err = WSAGetLastError();

            // If no devices found will return service not found
            if (err == WSASERVICE_NOT_FOUND)
                return results;

            throw SyncError(100101, getWinsockErrorMessage(err));
        }

        // Iterate over all devices found
        while (true) {

            if (WSALookupServiceNext(hLookup, dwControlFlags, &dwBuffSize, pQuerySet) == 0) {
                // Check for a matching device name
                if (pQuerySet->lpszServiceInstanceName != NULL) {
                    BluetoothDeviceInfo info;
                    info.csName = pQuerySet->lpszServiceInstanceName;
                    _TCHAR addrBuffer[1000];
                    DWORD dwAddressSize = sizeof(addrBuffer);
                    WSAAddressToString(pQuerySet->lpcsaBuffer->RemoteAddr.lpSockaddr, pQuerySet->lpcsaBuffer->RemoteAddr.iSockaddrLength,
                        NULL, addrBuffer, &dwAddressSize);
                    info.csAddress = addrBuffer;
                    results.push_back(info);
                }
            }
            else {
                int err = WSAGetLastError();
                if (err == WSAENOMORE || err == WSA_E_NO_MORE) {
                    break;
                }
                else if (err == WSAEFAULT) {
                    // Buffer too small - make it bigger and try again next time through loop
                    buffer.resize(dwBuffSize);
                    pQuerySet = (WSAQUERYSET*)&buffer[0];
                }
                else {
                    throw SyncError(100101, getWinsockErrorMessage(err));
                }
            }
        }

        WSALookupServiceEnd(hLookup);

        return results;
    }

}

WinBluetoothScanner::WinBluetoothScanner(std::shared_ptr<WinBluetoothFunctions> pBtFuncs)
    : m_bScanEndRequested(false),
      m_bScanThreadEnded(true),
      m_pBtFuncs(pBtFuncs)
{
}

WinBluetoothScanner::~WinBluetoothScanner()
{
    if (m_scanThread.joinable()) {
        stopScan();
        m_scanThread.join();
    }
}

void WinBluetoothScanner::startScan()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // If the scan thread is still running setting m_bScanEndRequested will cause it to continue scanning
    // so we don't have to start a new thread
    m_bScanEndRequested = false;

    // If the thread has ended we need to create a new thread
    if (m_bScanThreadEnded) {
        m_bScanThreadEnded = false;
        // Thread could still not have completely finished (after setting the ended flag but before exiting)
        // in which case it is an error to assign a new thread to the thread variable so
        // wait for it to complete before creating new thread.
        if (m_scanThread.joinable())
            m_scanThread.join();
        m_scanThread = std::thread(&WinBluetoothScanner::scanThreadMain, this);
    }
}

void WinBluetoothScanner::stopScan()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_bScanEndRequested = true;
}

void WinBluetoothScanner::setResultCallback(std::function<void(const DeviceList&)>&& cb)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_resultCallback = std::forward<std::function<void(const DeviceList&)>>(cb);
}

void WinBluetoothScanner::setErrorCallback(std::function<void(const SyncError&)>&& cb)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_errorCallback = std::forward<std::function<void(const SyncError&)>>(cb);
}

WinBluetoothScanner::DeviceList WinBluetoothScanner::getLastScanResult()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_lastScanResult;
}

void WinBluetoothScanner::scanThreadMain()
{

    while (true) {
        try {
            DeviceList devicesFound = scanForRemoteDevices(m_pBtFuncs);
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_lastScanResult = devicesFound;
                if (m_resultCallback)
                    m_resultCallback(m_lastScanResult);
            }

            // Wait a second before scanning again - if didn't find device
            // on last scan it probably isn't discoverable yet
            std::this_thread::sleep_for(std::chrono::seconds(1));

            {
                // Check if we are done
                std::lock_guard<std::mutex> lock(m_mutex);
                if (m_bScanEndRequested) {
                    m_bScanThreadEnded = true;
                    break;
                }
            }
        }
        catch (const SyncError& e)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_errorCallback) {
                m_errorCallback(e);
                m_bScanThreadEnded = true;
                break;
            }
        }
    }
}
