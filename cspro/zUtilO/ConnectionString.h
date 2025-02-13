#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/PropertyString.h>
#include <zDataO/DataRepositoryDefines.h>


// A connection string contains the repository and filename
// if applicable (null type does not have a filename).
// The connection string object can be converted to/from
// a string with the format: filename|type=type-value
// Where type is the repository type: "None", "Text",
// "CSProDB", or "EncryptedCSProDB" and filename is the
// path to the repository file if one exists (for none
// this will always be blank). Attributes other than the
// filename can be specified using percent-encoding.

class CLASS_DECL_ZUTILO ConnectionString : public PropertyString
{
    friend class DataFileFilterManager;

private:
    ConnectionString(bool defined, DataRepositoryType type)
        :   m_defined(defined),
            m_dataRepositoryType(type)
    {
    }

public:
    // Create an undefined connection string.
    ConnectionString()
        :   ConnectionString(false, DataRepositoryType::Null)
    {
    }

    // Create a connection string from a text string.
    explicit ConnectionString(wstring_view connection_string_text_sv);

    explicit ConnectionString(const CString& connection_string_text)       : ConnectionString(wstring_view(connection_string_text)) { }
    explicit ConnectionString(const std::wstring& connection_string_text)  : ConnectionString(wstring_view(connection_string_text)) { }
    explicit ConnectionString(NullTerminatedString connection_string_text) : ConnectionString(wstring_view(connection_string_text)) { }

    // Create a connection string for a null repository.
    static ConnectionString CreateNullRepositoryConnectionString()
    {
        return ConnectionString(true, DataRepositoryType::Null);
    }

    // Create a connection string for a memory repository.
    static ConnectionString CreateMemoryRepositoryConnectionString()
    {
        return ConnectionString(true, DataRepositoryType::Memory);
    }

    // Get whether or not the connection strings are equal.
    bool Equals(const ConnectionString& rhs_connection_string) const;

    // Get whether or not the connection string is defined.
    bool IsDefined() const
    {
        return m_defined;
    }

    // Clear the connection string.
    void Clear()
    {
        m_defined = false;
    }

    // Get the filename of the repository based on a connection string.
    const std::wstring& GetFilename() const
    {
        ASSERT(m_defined);
        return m_filename;
    }

    // Get whether or not the filename matches the current filename.
    bool FilenameMatches(wstring_view filename_sv) const
    {
        return ( m_defined && SO::EqualsNoCase(m_filename, filename_sv) );
    }

    // Get whether or not a filename is present as part of the connection string.
    bool IsFilenamePresent() const
    {
        return ( m_defined && !m_filename.empty() );
    }

    // Get the type of the repository based on a connection string.
    DataRepositoryType GetType() const
    {
        ASSERT(m_defined);
        return m_dataRepositoryType;
    }

    // Convert the connection string to a string representation.
    std::wstring ToString() const
    {
        return ToString(m_filename);
    }

    // Convert the connection string, with the specified filename, to a string representation.
    std::wstring ToString(std::wstring filename) const;

    // Convert the connection string to a string representation without the directory information.
    std::wstring ToStringWithoutDirectory() const;

    // Convert the connection string to a string suitable for displaying to a user (e.g., in a warning message).
    std::wstring ToDisplayString() const;

    // Convert the connection string to a string representation with
    // a relative path based on the specified directory.
    std::wstring ToRelativeString(wstring_view directory_name_sv, bool use_display_mode = false) const;

    // Convert the filename from relative to absolute based on the directory.
    void AdjustRelativePath(wstring_view directory_name_sv);

    // Returns a name that combines the repository type as well as information about the source. This
    // name can be used when printing out information about the repository in listing files.
    std::wstring GetName(DataRepositoryNameType name_type) const;

    // Writes the connection string in JSON format.
    void WriteJson(JsonWriter& json_writer, bool write_relative_path = true) const;

protected:
    void SetMainValue(std::wstring main_value) override { m_filename = WS2CS(main_value); }

    bool PreprocessProperty(wstring_view attribute_sv, wstring_view value_sv) override;

    void InitializeDataRepositoryType();

private:
    bool m_defined;
    std::wstring m_filename;
    DataRepositoryType m_dataRepositoryType;
};


constexpr const TCHAR* ConnectionStringDataRepositoryPropertyType = _T("type");

CLASS_DECL_ZUTILO extern const TCHAR* const DataRepositoryTypeNames[];
CLASS_DECL_ZUTILO extern const TCHAR* const DataRepositoryTypeDefaultExtensions[];

CLASS_DECL_ZUTILO const TCHAR* ToString(DataRepositoryType type);
