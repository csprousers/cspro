#pragma once

#include <zSyncO/zSyncO.h>
#include <zNetwork/LoginAccessor.h>


class SYNC_API SyncLoginAccessor : public LoginAccessor
{
public:
    std::shared_ptr<IBluetoothAdapter> GetBluetoothAdapter() override;

    std::optional<BluetoothDeviceInfo> ChooseBluetoothDevice() override;

private:
    std::shared_ptr<IBluetoothAdapter> m_bluetoothAdapter;
};
