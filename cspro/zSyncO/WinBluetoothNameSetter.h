#pragma once


class WinBluetoothNameSetter
{
public:
    static bool SetBluetoothName(const std::string& bluetooth_name);

private:
    static std::string GetGenericBluetoothAdapterInstanceID();
};
