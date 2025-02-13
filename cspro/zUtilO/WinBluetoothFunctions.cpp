#include "StdAfx.h"
#include "WinBluetoothFunctions.h"

WinBluetoothFunctions::WinBluetoothFunctions()
{
}

std::shared_ptr<WinBluetoothFunctions> WinBluetoothFunctions::instance()
{
    static std::shared_ptr<WinBluetoothFunctions> pInstance(create());
    return pInstance;
}

WinBluetoothFunctions* WinBluetoothFunctions::create()
{
    auto dll = ::LoadLibraryW(L"bthprops.cpl");
    if (!dll)
        return nullptr;

    WinBluetoothFunctions* pFuncs = new WinBluetoothFunctions();
    pFuncs->BluetoothGetDeviceInfo = reinterpret_cast<decltype(::BluetoothGetDeviceInfo)*>(::GetProcAddress(dll, "BluetoothGetDeviceInfo"));
    pFuncs->BluetoothFindFirstRadio = reinterpret_cast<decltype(::BluetoothFindFirstRadio)*>(::GetProcAddress(dll, "BluetoothFindFirstRadio"));
    pFuncs->BluetoothGetRadioInfo = reinterpret_cast<decltype(::BluetoothGetRadioInfo)*>(::GetProcAddress(dll, "BluetoothGetRadioInfo"));
    pFuncs->BluetoothEnableDiscovery = reinterpret_cast<decltype(::BluetoothEnableDiscovery)*>(::GetProcAddress(dll, "BluetoothEnableDiscovery"));
    pFuncs->BluetoothIsDiscoverable = reinterpret_cast<decltype(::BluetoothIsDiscoverable)*>(::GetProcAddress(dll, "BluetoothIsDiscoverable"));
    pFuncs->BluetoothEnableIncomingConnections = reinterpret_cast<decltype(::BluetoothEnableIncomingConnections)*>(::GetProcAddress(dll, "BluetoothEnableIncomingConnections"));
    pFuncs->BluetoothIsConnectable = reinterpret_cast<decltype(::BluetoothIsConnectable)*>(::GetProcAddress(dll, "BluetoothIsConnectable"));

    return pFuncs;
}
