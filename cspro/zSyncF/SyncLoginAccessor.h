#pragma once

#include <zSyncF/zSyncF.h>
#include <zNetwork/LoginAccessor.h>


class ZSYNCF_API SyncLoginAccessor : public LoginAccessor
{
public:
    std::shared_ptr<IBluetoothAdapter> GetBluetoothAdapter() override;

    std::optional<BluetoothDeviceInfo> ChooseBluetoothDevice() override;

private:
    std::shared_ptr<IBluetoothAdapter> m_bluetoothAdapter;
};
