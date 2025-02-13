#pragma once
#include <zSyncO/zSyncO.h>
#include <zSyncO/SyncException.h>
#include <thread>
#include <mutex>
#include <functional>
#include <Winsock2.h>
#include <Ws2bth.h>
#include <zSyncO/BluetoothDeviceInfo.h>
class WinBluetoothFunctions;

/// <summary>Service to scan for Bluetooth devices in a background thread</summary>
/// Because device scanning is very slow, in order to be able to cancel we need to run
/// it in the background. This service lets you register a callback when the scan is complete.
/// Note that while it is safe to call startScan() multiple times on a single instance
/// of this object it is not safe to do so on multiple instances since the underlying
/// Windows calls for Bluetooth scanning are not thread safe.
class SYNC_API WinBluetoothScanner {

public:
    WinBluetoothScanner(std::shared_ptr<WinBluetoothFunctions> pBtFuncs);
    ~WinBluetoothScanner();

    WinBluetoothScanner(WinBluetoothScanner const&) = delete;
    WinBluetoothScanner(WinBluetoothScanner&&) = delete;
    WinBluetoothScanner& operator=(WinBluetoothScanner const&) = delete;
    WinBluetoothScanner& operator=(WinBluetoothScanner &&) = delete;

    ///<summary>Start a new scan. Scan will continue to return results via callback until stopScan is called.</summary>
    void startScan();

    ///<summary>End current scan</summary>
    void stopScan();

    typedef std::vector<BluetoothDeviceInfo> DeviceList;

    ///<summary>Set function to be called when scan results are ready</summary>
    ///<param name="callback">Function object that takes a DeviceList that is called when scan results are ready</param>
    void setResultCallback(std::function<void(const DeviceList&)>&& callback);

    ///<summary>Set function to be called when a scanning error occurs</summary>
    ///<param name="callback">Function object that takes a SyncError</param>
    void setErrorCallback(std::function<void(const SyncError&)>&& callback);

    ///<summary>Get cached results from last scan</summary>
    DeviceList getLastScanResult();

private:

    void scanThreadMain();

    std::function<void(const DeviceList&)> m_resultCallback;
    std::function<void(const SyncError&)> m_errorCallback;
    std::thread m_scanThread;
    bool m_bScanEndRequested;
    bool m_bScanThreadEnded;
    DeviceList m_lastScanResult;
    std::mutex m_mutex;
    std::shared_ptr<WinBluetoothFunctions> m_pBtFuncs;
};
