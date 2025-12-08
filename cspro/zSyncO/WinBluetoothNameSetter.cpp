#include "stdafx.h"
#include "WinBluetoothNameSetter.h"
#include <cfgmgr32.h>
#include <devguid.h>
#include <Setupapi.h>
#pragma comment(lib, "setupapi.lib")


// routines modified from https://social.msdn.microsoft.com/Forums/vstudio/en-US/3ba39175-9243-4b78-960e-c80cc0f58d6a/change-bluetooth-device-name-in-c-or-c

std::string WinBluetoothNameSetter::GetGenericBluetoothAdapterInstanceID()
{
    // Find all Bluetooth radio modules
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_BLUETOOTH, nullptr, nullptr, DIGCF_PRESENT);

    if( hDevInfo == INVALID_HANDLE_VALUE )
        return std::string();

    // Get first Generic Bluetooth Adapter InstanceID
    auto deviceInstanceID = std::make_unique_for_overwrite<wchar_t[]>(MAX_DEVICE_ID_LEN);

    for( unsigned i = 0; ; ++i )
    {
        SP_DEVINFO_DATA DeviceInfoData;
        DeviceInfoData.cbSize = sizeof(DeviceInfoData);

        if( !SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData) )
            break;

        const CONFIGRET r = CM_Get_Device_ID(DeviceInfoData.DevInst, deviceInstanceID.get(), MAX_DEVICE_ID_LEN, 0);

        if( r == CR_SUCCESS && wcsncmp(deviceInstanceID.get(), L"USB", 3) == 0 )
            return TC::ToUtf8(deviceInstanceID.get());
    }

    return std::string();
}


bool WinBluetoothNameSetter::SetBluetoothName(const std::string& bluetooth_name)
{
    const std::string instanceID = GetGenericBluetoothAdapterInstanceID();

    if( instanceID.empty()  )
        return false; // Failed to get Generic Bluetooth Adapter InstanceID

    std::string instanceIDModified = instanceID;
    SO::Replace(instanceIDModified, '\\', '#');

    const std::string fileName = FormatText("\\\\.\\%s#{a5dcbf10-6530-11d2-901f-00c04fb951ed}", instanceIDModified.c_str());
    HANDLE hDevice = CreateFile(TC::ToWide(fileName).c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

    if( hDevice == INVALID_HANDLE_VALUE )
        return false; // Failed to open device. Error code: GetLastError()

    // Change radio module local name in registry
    const std::string rmLocalNameKey = FormatText("SYSTEM\\ControlSet001\\Enum\\%s\\Device Parameters", instanceID.c_str());
    HKEY hKey;
    LSTATUS ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TC::ToWide(rmLocalNameKey).c_str(), 0, KEY_SET_VALUE, &hKey);

    if( ret != ERROR_SUCCESS )
        return false; // Failed to open registry key. Error code: ret

    ret = RegSetValueEx(hKey, L"Local Name", 0, REG_BINARY, reinterpret_cast<const BYTE*>(bluetooth_name.c_str()), uint32_cast(bluetooth_name.length()));

    if( ret != ERROR_SUCCESS )
        return false; // Failed to set registry key. Error code: ret

    RegCloseKey(hKey);

    UINT ctlCode = 0x411008; // for OS version dwMajorVersion >= 6
    long reload = 4; // tells the control function to reset or reload or similar...
    DWORD bytes = 0; // merely a placeholder

    // Send radio module driver command to update device information
    if( !DeviceIoControl(hDevice, ctlCode, &reload, 4, nullptr, 0, &bytes, nullptr) )
        return false; // Failed to update radio module local name. Error code: GetLastError()

    return true;
}
