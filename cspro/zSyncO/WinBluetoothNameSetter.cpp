#include "stdafx.h"
#include <zToolsO/Utf8Convert.h>
#include <cfgmgr32.h>
#include <devguid.h>
#include <Setupapi.h>
#pragma comment(lib, "setupapi.lib")


// routines modified from https://social.msdn.microsoft.com/Forums/vstudio/en-US/3ba39175-9243-4b78-960e-c80cc0f58d6a/change-bluetooth-device-name-in-c-or-c


std::unique_ptr<TCHAR[]> GetGenericBluetoothAdapterInstanceID()
{
    // Find all Bluetooth radio modules
    HDEVINFO hDevInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_BLUETOOTH, nullptr, nullptr, DIGCF_PRESENT);

    if( hDevInfo == INVALID_HANDLE_VALUE )
        return nullptr;

    // Get first Generic Bluetooth Adapter InstanceID
    auto deviceInstanceID = std::make_unique<TCHAR[]>(MAX_DEVICE_ID_LEN);

    for( unsigned i = 0; ; ++i )
    {
        SP_DEVINFO_DATA DeviceInfoData;
        DeviceInfoData.cbSize = sizeof(DeviceInfoData);

        if( !SetupDiEnumDeviceInfo(hDevInfo, i, &DeviceInfoData) )
            break;

        CONFIGRET r = CM_Get_Device_ID(DeviceInfoData.DevInst, deviceInstanceID.get(), MAX_PATH, 0);

        if( r == CR_SUCCESS && _tcsncmp(_T("USB"), deviceInstanceID.get(), 3) == 0 )
            return deviceInstanceID;
    }

    return nullptr;
}


bool SetBluetoothName(const CString& bluetooth_name)
{
    std::unique_ptr<TCHAR[]> instanceID = GetGenericBluetoothAdapterInstanceID();

    if( instanceID == nullptr )
        return false; // Failed to get Generic Bluetooth Adapter InstanceID

    CString instanceIDModified = instanceID.get();
    instanceIDModified.Replace(_T("\\"), _T("#"));

    CString fileName = FormatText(_T("\\\\.\\%s#{a5dcbf10-6530-11d2-901f-00c04fb951ed}"), (LPCTSTR)instanceIDModified);
    HANDLE hDevice = CreateFile(fileName, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr);

    if( hDevice == INVALID_HANDLE_VALUE )
        return false; // Failed to open device. Error code: GetLastError()

    // Change radio module local name in registry
    CString rmLocalNameKey = FormatText(_T("SYSTEM\\ControlSet001\\Enum\\%s\\Device Parameters"), instanceID.get());
    HKEY hKey;
    LSTATUS ret = RegOpenKeyEx(HKEY_LOCAL_MACHINE, rmLocalNameKey, 0, KEY_SET_VALUE, &hKey);

    if( ret != ERROR_SUCCESS )
        return false; // Failed to open registry key. Error code: ret

    std::string utf8_name = UTF8Convert::WideToUTF8(bluetooth_name);

    ret = RegSetValueEx(hKey, _T("Local Name"), 0, REG_BINARY, (const BYTE*)utf8_name.c_str(), utf8_name.length());

    if( ret != ERROR_SUCCESS )
        return false; // Failed to set registry key. Error code: ret

    RegCloseKey(hKey);

    UINT ctlCode = 0x411008; // for OS version dwMajorVersion >= 6
    long reload = 4; // tells the control function to reset or reload or similar...
    DWORD bytes = 0; // merely a placeholder

    // Send radio module driver command to update device information
    if (!DeviceIoControl(hDevice, ctlCode, &reload, 4, nullptr, 0, &bytes, nullptr) )
        return false; // Failed to update radio module local name. Error code: GetLastError()

    return true;
}
