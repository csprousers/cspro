#include "stdafx.h"
#include "SyncLoginAccessor.h"

#ifdef WIN_DESKTOP
#include "ChooseBluetoothDeviceDialog.h"
#else
#include "IBluetoothAdapter.h"
#include "ObexConstants.h"
#endif


std::shared_ptr<IBluetoothAdapter> SyncLoginAccessor::GetBluetoothAdapter()
{
    if( m_bluetoothAdapter == nullptr )
    {
#ifdef WIN_DESKTOP
        m_bluetoothAdapter = WinBluetoothAdapter::Create();
#else
        m_bluetoothAdapter = PlatformInterface::GetInstance()->GetApplicationInterface()->CreateBluetoothAdapter();
#endif
    }

    return m_bluetoothAdapter;
}


std::optional<BluetoothDeviceInfo> SyncLoginAccessor::ChooseBluetoothDevice()
{
#ifdef WIN_DESKTOP
    const std::shared_ptr<IBluetoothAdapter> bluetooth_adapter = GetBluetoothAdapter();

    if( bluetooth_adapter == nullptr )
        return std::nullopt;

    ChooseBluetoothDeviceDialog dlg(assert_cast<WinBluetoothAdapter*>(bluetooth_adapter.get()));

    return dlg.ChooseBluetoothDevice();

#else
    return PlatformInterface::GetInstance()->GetApplicationInterface()->ChooseBluetoothDevice(OBEX_SYNC_SERVICE_UUID);
#endif
}
