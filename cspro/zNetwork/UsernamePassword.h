#pragma once

#include <zNetwork/zNetwork.h>


// --------------------------------------------------------------------------
// UsernamePassword
// --------------------------------------------------------------------------

struct UsernamePassword
{
    std::string username;
    std::string password;

    // throws an exception if not valid
    ZNETWORK_API static UsernamePassword CreateFromJson(const JsonNode& json_node);
    ZNETWORK_API void WriteJson(JsonWriter& json_writer) const;
};
