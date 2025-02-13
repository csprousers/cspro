#pragma once


using DeviceId = CString;

// Direction of sync - can be used as bitmask where SyncDirection::Both includes
// GET and PUT.
enum class SyncDirection
{
    Put = 1,    // Client to server
    Get = 2,    // Server to client
    Both = 3,   // Two way (PUT | GET)
};

enum class SyncServerType
{
    CSWeb = 0,
    Dropbox = 1,
    FTP = 2,
    LocalDropbox = 3,
    LocalFiles = 4
};
