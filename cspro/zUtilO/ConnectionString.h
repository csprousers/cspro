#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/PropertyString.h>
#include <zDataO/DataRepositoryDefines.h>


// --------------------------------------------------------------------------
// ConnectionString
//
// A connection string contains information about a data repository,
// including the type as well as the file path when applicable.
//
// The connection string object can be converted to/from a string with
// the format: file_path|type=type-value
// where type is the repository type: "None", "Text", "CSProDB", etc.
// and file_path is the path to the data repository file if one exists.
//
// Attributes other than the path can be specified using percent-encoding.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO ConnectionString : public PropertyString
{
    friend class DataFileFilterManager;

public:
    static DataRepositoryType GetDefaultDataRepositoryTypeFromText(const std::string_view text_sv);

private:
    enum class ResourceType { Undefined, None, FilePath, Url };

    ConnectionString(ResourceType resource_type, DataRepositoryType data_repository_type);

public:
    // Creates an undefined connection string.
    ConnectionString();

    // Creates a connection string from a text string.
    explicit ConnectionString(std::string_view connection_string_text_sv);
    explicit ConnectionString(const std::string& connection_string_text);

    // Creates a connection string for a null repository.
    static ConnectionString CreateNullRepositoryConnectionString();

    // Creates a connection string for a memory repository.
    static ConnectionString CreateMemoryRepositoryConnectionString();

    // Returns true if the connection strings are equal.
    bool Equals(const ConnectionString& rhs_connection_string, bool compare_properties = false) const;

    // Returns true if the connection strings refer to the same file path or URL.
    bool SharesResource(const ConnectionString& rhs_connection_string) const;
    bool SharesResource(const std::vector<ConnectionString>& rhs_connection_strings) const;

    // Returns true if the connection string is defined.
    bool IsDefined() const;

    // Clears the connection string, making it undefined.
    void Clear();

    // Returns the type of the repository.
    DataRepositoryType GetType() const;

    // Returns true if a resource (file path or URL) is part of the connection string.
    bool HasResource() const;

    // Returns true if a file path is part of the connection string.
    bool HasFilePath() const;

    // Returns the full path of a repository that uses a file.
    const std::string& GetFilePath() const;

    // Returns true if the connection string has a file path that matches.
    bool FilePathMatches(std::string_view file_path_sv) const;

    // Returns true if a URL is part of the connection string.
    bool HasUrl() const;

    // Returns the URL of a repository that uses a URL.
    const std::string& GetUrl() const;

    // Returns true if the connection string's type would be correctly calculated based on implicit calculations.
    bool TypeCanBeCalculatedImplicitly() const;

    // Returns the connection string represented as a string.
    std::string ToString() const;

    // Returns the connection string represented as a string using the specified file path.
    // When inherit_type_from_parent is false, the repository type will be calculated based
    // on the file path rather than using the parent connection string's type.
    std::string ToString(std::string file_path, bool inherit_type_from_parent) const;

    // Returns the connection string represented as a string, with any directory removed from file paths.
    std::string ToStringWithoutDirectory() const;

    // Returns the connection string represented as a string suitable for displaying to a user (e.g., in a warning message).
    std::string ToDisplayString(bool use_filename_only = false) const;

    // Returns the connection string represented as a string with a file path adjusted based on the specified directory.
    std::string ToRelativeString(std::string directory_path, bool use_display_mode = false) const;

    // Converts connection strings that use file paths from relative to absolute based on the directory.
    void AdjustRelativePath(const std::string& directory_path);

    // Returns a name that combines the repository type with information about the source.
    // This name can be used when printing out information about the repository in listing files.
    std::string GetName(DataRepositoryNameType name_type) const;

    // Creates a connection string from JSON format.
    static ConnectionString CreateFromJson(const JsonNode& json_node);

    // Writes the connection string in JSON format.
    void WriteJson(JsonWriter& json_writer, bool write_relative_path = true) const;

private:
    void SetMainValue(std::string_view main_value_sv) override;

    static bool TextToType(DataRepositoryType& type, std::string_view text_sv);

    bool PreprocessProperty(std::string_view attribute_sv, std::string_view value_sv) override;

    void InitializeDataRepositoryType();

    template<bool return_value_for_no_resource>
    bool ResourceMatches(const ConnectionString& rhs_connection_string) const;

    bool TypeCanBeCalculatedImplicitly(const std::string& resource) const;

private:
    ResourceType m_resourceType;
    std::string m_resource;
    DataRepositoryType m_dataRepositoryType;
};


constexpr const char* ConnectionStringDataRepositoryPropertyType = "type";

CLASS_DECL_ZUTILO extern const char* const DataRepositoryTypeNames[];
CLASS_DECL_ZUTILO extern const char* const DataRepositoryTypeDefaultExtensions[];

CLASS_DECL_ZUTILO const char* ToString(DataRepositoryType type);



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline ConnectionString::ConnectionString(const ResourceType resource_type, const DataRepositoryType data_repository_type)
    :   m_resourceType(resource_type),
        m_dataRepositoryType(data_repository_type)
{
}


inline ConnectionString::ConnectionString()
    :   ConnectionString(ResourceType::Undefined, DataRepositoryType::Null)
{
}


inline ConnectionString::ConnectionString(const std::string& connection_string_text)
    :   ConnectionString(std::string_view(connection_string_text))
{
}


inline bool ConnectionString::IsDefined() const
{
    return ( m_resourceType != ResourceType::Undefined );
}


inline void ConnectionString::Clear()
{
    m_resourceType = ResourceType::Undefined;
    m_resource.clear();
    m_dataRepositoryType = DataRepositoryType::Null;
}


inline DataRepositoryType ConnectionString::GetType() const
{
    ASSERT(IsDefined());
    return m_dataRepositoryType;
}


inline bool ConnectionString::HasResource() const
{
    return ( !m_resource.empty() && ( m_resourceType == ResourceType::FilePath ||
                                      m_resourceType == ResourceType::Url ) );
}


inline bool ConnectionString::HasFilePath() const
{
    return ( m_resourceType == ResourceType::FilePath && !m_resource.empty() );
}


inline const std::string& ConnectionString::GetFilePath() const
{
    ASSERT(m_resourceType == ResourceType::FilePath);
    return m_resource;
}


inline bool ConnectionString::FilePathMatches(const std::string_view file_path_sv) const
{
    return ( HasFilePath() && SO::EqualsNoCase(m_resource, file_path_sv) );
}


inline bool ConnectionString::HasUrl() const
{
    return ( m_resourceType == ResourceType::Url && !m_resource.empty() );
}


inline const std::string& ConnectionString::GetUrl() const
{
    ASSERT(m_resourceType == ResourceType::Url);
    return m_resource;
}


inline std::string ConnectionString::ToString() const
{
    return ToString(m_resource, true);
}
