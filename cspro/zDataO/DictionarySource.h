#pragma once

#include <zDataO/zDataO.h>
#include <zUtilO/ConnectionString.h>
#include <zUtilO/TemporaryFile.h>

class CDataDict;


// --------------------------------------------------------------------------
// DictionarySource
//
// This class manages a dictionary that is either read from the disk or
// from a data source's embedded or associated dictionary.
//
// The class also has several static methods to retrieve embedded and
// associated dictionaries.
// --------------------------------------------------------------------------

class ZDATAO_API DictionarySource
{
public:
    // --------------------------------------------------------------------------
    // Static methods
    // --------------------------------------------------------------------------

    // Returns a dictionary file path from the connection string's dictionaryPath override.
    // If no override exists, a blank string is returned.
    static std::string GetDictionaryPathOverride(const ConnectionString& connection_string);

    // Returns true if an embedded dictionary exists in the data source identified by the connection string.
    static bool HasEmbeddedDictionary(const ConnectionString& connection_string);

    // Returns true if a data source created using by the connection string will have an embedded dictionary.
    static bool HasEmbeddedDictionaryWhenCreated(const ConnectionString& connection_string);

    // Returns true if an associated dictionary exists in the data source identified by the connection string.
    // This dictionary may be embedded in the data source or otherwise specified in the connection string.
    static bool HasAssociatedDictionary(const ConnectionString& connection_string);

    // Returns a data source's embedded dictionary, returning null if none is available.
    // Exceptions are thrown on errors reading the dictionary.
    static std::unique_ptr<CDataDict> GetEmbeddedDictionary(const ConnectionString& connection_string);

    // Returns a data source's associated dictionary, returning null if none is available.
    // Exceptions are thrown on errors reading the dictionary.
    static std::unique_ptr<CDataDict> GetAssociatedDictionary(const ConnectionString& connection_string);


    // --------------------------------------------------------------------------
    // DictionarySource object
    // --------------------------------------------------------------------------

    // The connection string does not need to refer to a data source but can also be the file path to a dictionary.
    DictionarySource(ConnectionString connection_string = ConnectionString());
    DictionarySource(std::string_view connection_string_text_sv);

    // Returns true if the dictionary source is defined.
    bool IsDefined() const { return m_connectionString.IsDefined(); }

    // Returns the dictionary source's connection string.
    const ConnectionString GetConnectionString() const { return m_connectionString; }

    // Resets the dictionary source to an undefined state.
    void Reset();

    // Loads the dictionary from the disk or from a data source.
    // Exceptions are thrown on load errors but no exception is thrown if there is not an associated dictionary.
    std::shared_ptr<const CDataDict> GetAssociatedDictionary();

    // Loads the dictionary from the disk or from a data source.
    // Exceptions are thrown on load errors or if there is no associated dictionary.
    std::shared_ptr<const CDataDict> GetDictionary();

    // Returns true if the loaded dictionary was embedded in a data source.
    bool UsingEmbeddedDictionary() const;

    // Returns the file path of the dictionary (or overridden dictionary), or the file path of the data source if using
    // an embedded dictionary. A blank string is returned if the source of the embedded dictionary is not file-based.
    const std::string& GetSourceFilePath() const;

    // Returns the file path of the dictionary (or overridden dictionary) when the dictionary is not an embedded dictionary.
    const std::string& GetDictionaryFilePath() const;

    // Returns the file path of the dictionary (or overridden dictionary) when the dictionary is not an embedded dictionary.
    // If the dictionary was loaded form a data source's embedded dictionary, it will be saved to a temporary file.
    // An exception is thrown if there is a problem saving the temporary file.
    const std::string& GetFileBasedDictionaryFilePath();

    // Serialization functions.
    static DictionarySource CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

private:
    static bool IsDictionaryFilePath(const ConnectionString& connection_string);

private:
    ConnectionString m_connectionString;
    std::shared_ptr<const CDataDict> m_dictionary;

    using DictionaryFilePathStorage = std::variant<std::monostate, std::string, std::shared_ptr<TemporaryFile>>;
    DictionaryFilePathStorage m_dictionaryFilePathStorage;
};
