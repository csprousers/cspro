#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/PropertyString.h>


enum class SyncServiceType { Bluetooth, CSWeb, Dropbox, Ftp, LocalFiles };


// --------------------------------------------------------------------------
// SyncConnectionString
//
// A sync connection string contains information about how to connect to
// a synchronization service.
//
// Attributes other than the URL can be specified using percent-encoding.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO SyncConnectionString : public PropertyString
{
public:
    static constexpr const char* PropertyType = "type";

private:
    enum class ResourceType { Undefined, Text, Url, DirectoryPath };

    SyncConnectionString(ResourceType resource_type, SyncServiceType sync_service_type);

public:
    // Creates an undefined sync connection string.
    SyncConnectionString();

    // Creates a sync connection string from a text string.
    SyncConnectionString(std::string_view sync_connection_string_text_sv);
    SyncConnectionString(const std::string& sync_connection_string_text);

    // Creates a sync connection string for Dropbox.
    static SyncConnectionString CreateDropboxSyncConnectionString();
    static SyncConnectionString CreateLocalDropboxSyncConnectionString();

    // Creates a sync connection string for LocalFiles.
    static SyncConnectionString CreateLocalFilesSyncConnectionString(std::string directory_path);

    // Returns true if the sync connection string is defined.
    bool IsDefined() const;

    // Clears the sync connection string, making it undefined.
    void Clear();

    // Returns the sync service type.
    SyncServiceType GetType() const;

    // Returns the sync service's URL.
    const std::string& GetUrl() const;

    // Returns the evaluated directory path (from the directory path, or from a file URL).
    std::string GetEvaluatedDirectoryPath() const;

    // Returns the evaluated Bluetooth server device name (from the path).
    std::string GetEvaluatedBluetoothServerDeviceName() const;

    // Adds the username and password properties.
    void SetUsernamePasswordProperties(std::string username, std::string password);

    // Returns the sync connection string represented as a string.
    std::string ToString() const;

    // Returns the sync connection string represented as a string suitable for displaying to a user (i.e., without properties).
    std::string ToDisplayString() const;

    // Returns the sync connection string represented as a string with any sensitive properties removed.
    std::string ToSafeString() const;

    // Returns the sync connection string represented as a string, with directory paths adjusted based on the specified directory.
    std::string ToRelativeString(const std::string& directory_path) const;

    // Converts sync connection strings that use directory paths from relative to absolute based on the directory.
    void AdjustRelativePath(const std::string& directory_path);

    // Validates the sync connection string, throwing exceptions on error.
    void Validate(std::optional<SyncServiceType> required_sync_service_type = std::nullopt) const;

    // Creates a sync connection string from JSON format.
    static SyncConnectionString CreateFromJson(const JsonNode& json_node);

    // Writes the sync connection string in JSON format.
    void WriteJson(JsonWriter& json_writer) const;

    // Serializes the sync connection string to an archive.
    void serialize(Serializer& ar);

private:
    void SetMainValue(std::string_view main_value_sv) override;

    static std::optional<SyncServiceType> TextToSyncServiceType(std::string_view text_sv);

    static SyncServiceType CalculateImplicitSyncServiceType(const std::string& resource);

    void Initialize(std::optional<SyncServiceType> sync_service_type);

    std::string ToString(std::string resource) const;

private:
    ResourceType m_resourceType;
    std::string m_resource;
    SyncServiceType m_syncServiceType;
};


CLASS_DECL_ZUTILO const char* ToString(SyncServiceType sync_service_type);



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SyncConnectionString::SyncConnectionString(const ResourceType resource_type, const SyncServiceType sync_service_type)
    :   m_resourceType(resource_type),
        m_syncServiceType(sync_service_type)
{
}


inline SyncConnectionString::SyncConnectionString()
    :   SyncConnectionString(ResourceType::Undefined, SyncServiceType::CSWeb)
{
}


inline SyncConnectionString::SyncConnectionString(const std::string& sync_connection_string_text)
    :   SyncConnectionString(std::string_view(sync_connection_string_text))
{
}


inline bool SyncConnectionString::IsDefined() const
{
    return ( m_resourceType != ResourceType::Undefined );
}


inline void SyncConnectionString::Clear()
{
    m_resourceType = ResourceType::Undefined;
    m_resource.clear();
    m_syncServiceType = SyncServiceType::CSWeb;
}


inline SyncServiceType SyncConnectionString::GetType() const
{
    ASSERT(IsDefined());
    return m_syncServiceType;
}


inline const std::string& SyncConnectionString::GetUrl() const
{
    ASSERT(IsDefined() && m_resourceType == ResourceType::Url);
    return m_resource;
}


inline std::string SyncConnectionString::ToString() const
{
    return ToString(m_resource);
}
