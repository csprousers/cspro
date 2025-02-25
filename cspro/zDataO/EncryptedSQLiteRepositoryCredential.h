#pragma once

#include <zDataO/EncryptedSQLiteRepository.h>
#include <zToolsO/Hash.h>
#include <zDictO/DDClass.h>


class EncryptedSQLiteRepositoryCredential
{
private:
    static constexpr int CurrentVersion = 2; // version 2 introduced in CSPro 8.1

public:
    double GetStorageTimestamp() const       { return m_storageTimestamp; }
    const std::byte* GetPasswordHash() const { return m_passwordHash.data(); }

    const std::string& GetFilePath() const       { return m_filePath; }
    const std::string& GetDictionaryName() const { return m_dictionaryName; }

    // Creates a credential string.
    static std::string Create(const CDataDict* dictionary, const std::string& file_path, const std::byte* password_hash);

    // Parses the credential string, throwing exceptions if not valid.
    EncryptedSQLiteRepositoryCredential(std::string_view credential_string_sv);

private:
    double m_storageTimestamp;
    std::vector<std::byte> m_passwordHash;
    std::string m_filePath;
    std::string m_dictionaryName;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::string EncryptedSQLiteRepositoryCredential::Create(const CDataDict* const dictionary, const std::string& file_path,
                                                               const std::byte* const password_hash)
{
    ASSERT(!file_path.empty() && password_hash != nullptr);

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::version, CurrentVersion)
                .Write(JK::timestamp, GetTimestamp())
                .Write(JK::password, Hash::BytesToHexString(password_hash, EncryptedSQLiteRepository::PasswordHashSize))
                .WritePath(JK::path, file_path);

    if( dictionary != nullptr )
        json_writer->Write(JK::dictionary, dictionary->GetName());

    json_writer->EndObject();

    return json_writer->ReleaseString();
}


inline EncryptedSQLiteRepositoryCredential::EncryptedSQLiteRepositoryCredential(const std::string_view credential_string_sv)
{
    auto throw_exception = []() { throw CSProException("Invalid credential format."); };

    if( credential_string_sv.empty() )
        throw_exception();

    // version 1 credentials, which were not JSON
    if( credential_string_sv.front() != '{' )
    {
        m_passwordHash = Hash::HexStringToBytes(credential_string_sv, true);

        struct Header { char version; double storage_timestamp; };
        const Header* const header = reinterpret_cast<const Header*>(m_passwordHash.data());

        if( m_passwordHash.size() < sizeof(Header) || header->version != 1 )
            throw_exception();

        m_storageTimestamp = header->storage_timestamp;
        m_passwordHash.erase(m_passwordHash.begin(), m_passwordHash.begin() + sizeof(Header));
    }

    // version 2 credentials
    else
    {
        const JsonNode json_node = Json::Parse(credential_string_sv);

        if( json_node.Get<int>(JK::version) != CurrentVersion )
            throw_exception();

        m_storageTimestamp = json_node.Get<double>(JK::timestamp);
        m_passwordHash = Hash::HexStringToBytes(json_node.Get<std::string_view>(JK::password), true);
        m_filePath = json_node.GetAbsolutePath(JK::path);
        m_dictionaryName = json_node.GetOrConstruct<std::string>(JK::dictionary);
    }

    if( m_passwordHash.size() != EncryptedSQLiteRepository::PasswordHashSize )
        throw_exception();
}
