#pragma once

#include <zAppO/zAppO.h>
#include <zJson/JsonSerializer.h>


using DeviceId = std::string;


// Direction of sync - can be used as bitmask where SyncDirection::Both includes GET and PUT.
enum class SyncDirection
{
    Put  = 1,   // Client to server
    Get  = 2,   // Server to client
    Both = 3,   // Two way (PUT | GET)
};

DECLARE_ENUM_JSON_SERIALIZER_CLASS(SyncDirection, ZAPPO_API);

constexpr const char* ToString(SyncDirection direction)
{
    switch( direction )
    {
        case SyncDirection::Put:  return "put";
        case SyncDirection::Get:  return "get";
        case SyncDirection::Both: return "both";
        default:                  return ReturnProgrammingError("INVALID");
    }
}
