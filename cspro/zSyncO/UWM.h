#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Sync
{
    constexpr unsigned UpdateDialogUI            = UWM::Ranges::SyncStart + 0;
    constexpr unsigned BluetoothUpdateDeviceList = UWM::Ranges::SyncStart + 1;
    constexpr unsigned BluetoothScanError        = UWM::Ranges::SyncStart + 2;

    CHECK_MESSAGE_NUMBERING(BluetoothScanError, UWM::Ranges::SyncLast)
}
