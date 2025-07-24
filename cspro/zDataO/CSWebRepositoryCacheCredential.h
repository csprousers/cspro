#pragma

#include <zToolsO/Hash.h>
#include <zNetwork/CSWebUser.h>
#include <zAppO/SyncTypes.h>


// --------------------------------------------------------------------------
// CSWebRepositoryCacheCredential
// --------------------------------------------------------------------------

struct CSWebRepositoryCacheCredential
{
    DeviceId server_device_id;
    CSWebUser user;
    std::string dictionary_name;
    std::string cache_file_path;
    std::optional<std::vector<std::byte>> cache_password;

    static CSWebRepositoryCacheCredential CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
};


// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CSWebRepositoryCacheCredential CSWebRepositoryCacheCredential::CreateFromJson(const JsonNode& json_node)
{
    return
    {
        json_node.Get<std::string>(JK::deviceId),
        json_node.Get<CSWebUser>(JK::user),
        json_node.Get<std::string>(JK::dictionary),
        json_node.Get<std::string>(JK::path),
        json_node.Contains(JK::password) ? std::make_optional<std::vector<std::byte>>(Hash::HexStringToBytes(json_node.Get<std::string>(JK::password), true)) :
                                           std::nullopt
    };
}


inline void CSWebRepositoryCacheCredential::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::deviceId, server_device_id)
               .Write(JK::user, user)
               .Write(JK::dictionary, dictionary_name)
               .Write(JK::path, cache_file_path);

    if( cache_password.has_value() )
        json_writer.Write(JK::password, Hash::BytesToHexString(cache_password->data(), cache_password->size()));

    json_writer.EndObject();
}
