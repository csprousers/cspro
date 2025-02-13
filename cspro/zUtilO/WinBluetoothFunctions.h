#pragma once
#include <zUtilO/zUtilO.h>

#include <BluetoothAPIs.h>

/// <summary>
/// Holds function pointers to functions imported from dynamically linked Bluetooth DLL.
/// Avoids static linking to Bluetooth DLLs which are not available on all operating
/// systems (e.g. Windows Servers).
/// </summary>
class CLASS_DECL_ZUTILO WinBluetoothFunctions {

private:

    WinBluetoothFunctions();
    static WinBluetoothFunctions* create();

public:

    /// <summary>
    /// Load Bluetooth DLL and store pointers to imported functions.
    /// </summary>
    /// <returns>Collection of function pointers or null if unable to load DLL.</returns>
    static std::shared_ptr<WinBluetoothFunctions> instance();

    decltype(::BluetoothGetDeviceInfo)* BluetoothGetDeviceInfo;
    decltype(::BluetoothFindFirstRadio) *BluetoothFindFirstRadio;
    decltype(::BluetoothGetRadioInfo) *BluetoothGetRadioInfo;
    decltype(::BluetoothEnableDiscovery) *BluetoothEnableDiscovery;
    decltype(::BluetoothIsDiscoverable) *BluetoothIsDiscoverable;
    decltype(::BluetoothEnableIncomingConnections) *BluetoothEnableIncomingConnections;
    decltype(::BluetoothIsConnectable) *BluetoothIsConnectable;
};
