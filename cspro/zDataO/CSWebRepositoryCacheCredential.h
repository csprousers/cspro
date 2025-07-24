#pragma

#include <zToolsO/Hash.h>
#include <zUtilO/CredentialStore.h>
#include <zNetwork/CSWebUser.h>


// --------------------------------------------------------------------------
// CSWebRepositoryCacheCredential
// --------------------------------------------------------------------------

struct CSWebRepositoryCacheCredential
{
    std::string dictionary_name;
    CSWebUser user;
    std::string cache_file_path;
    std::vector<std::byte> cache_password;

    static CSWebRepositoryCacheCredential CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;
};


// --------------------------------------------------------------------------
// CSWebRepositoryCacheCredentialStore
// --------------------------------------------------------------------------

class CSWebRepositoryCacheCredentialStore : public CredentialStore
{
public:
    CSWebRepositoryCacheCredentialStore(const std::string& dictionary_name, const CSWebUser& user);

protected:
    std::string PrefixAttribute(const std::string& attribute) override;

private:
    std::string m_prefix;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CSWebRepositoryCacheCredential CSWebRepositoryCacheCredential::CreateFromJson(const JsonNode& json_node)
{
    return
    {
        json_node.Get<std::string>(JK::dictionary),
        json_node.Get<CSWebUser>(JK::user),
        json_node.Get<std::string>(JK::path),
        Hash::HexStringToBytes(json_node.Get<std::string>(JK::password), true)
    };
}


inline void CSWebRepositoryCacheCredential::WriteJson(JsonWriter& json_writer) const
{
    json_writer.BeginObject()
               .Write(JK::dictionary, dictionary_name)
               .Write(JK::user, user)
               .Write(JK::path, cache_file_path)
               .Write(JK::password, Hash::BytesToHexString(cache_password.data(), cache_password.size()))
               .EndObject();
}
