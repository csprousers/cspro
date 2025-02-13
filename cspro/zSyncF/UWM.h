#pragma once

#include <zUtilO/UWMRanges.h>


namespace UWM::Sync
{
    const unsigned BluetoothUpdateDeviceList   = UWM::Ranges::SyncStart + 0;
    const unsigned BluetoothScanError          = UWM::Ranges::SyncStart + 1;
    const unsigned DropboxAuthProcessCompleted = UWM::Ranges::SyncStart + 2;

    CHECK_MESSAGE_NUMBERING(DropboxAuthProcessCompleted, UWM::Ranges::SyncLast)
}
