#pragma once
#include <zSyncO/zSyncO.h>
#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/IBluetoothAdapter.h>

///<summary>Interface for choosing for nearby Bluetooth devices</summary>
struct IChooseBluetoothDeviceDialog
{
    /// <summary>
    /// Prompt user for credentials
    /// </summary>
    /// <param name="pAdapter">Bluetooth adapter</param>
    /// <param name="deviceInfo">Name/address of device returned</param>
    /// <returns>True if device chosen, false if canceled</returns>
    virtual bool Show(BluetoothDeviceInfo& deviceInfo) = 0;

    virtual ~IChooseBluetoothDeviceDialog() {}
};

