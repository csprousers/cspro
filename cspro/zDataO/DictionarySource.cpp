#include "stdafx.h"
#include "DictionarySource.h"
#include "CSWebRepository.h"
#include "EncryptedSQLiteRepository.h"
#include "SQLiteRepository.h"


std::string DictionarySource::GetDictionaryPathOverride(const ConnectionString& connection_string)
{
    const std::string* const dictionary_path_override = connection_string.GetProperty(CSProperty::dictionaryPath);

    if( dictionary_path_override == nullptr || dictionary_path_override->empty() )
        return std::string();

    const std::string working_directory = connection_string.HasFilePath() ? GetWorkingDirectory(connection_string.GetFilePath()) :
                                                                            GetWorkingDirectory();

    return MakeFullPath(working_directory, *dictionary_path_override);
}


bool DictionarySource::HasEmbeddedDictionary(const ConnectionString& connection_string)
{
    // CSPro DB and Encrypted CSPro DB data sources have embedded dictionaries
    if( DataRepositoryHelpers::IsTypeFileBasedWithAnEmbeddedDictionary(connection_string.GetType()) )
        return true;

    // CSWeb data sources can be considered to have an embedded dictionary when the
    // dictionary name is explicitly specified
    if( connection_string.GetType() == DataRepositoryType::CSWeb &&
        connection_string.HasPropertyWithValue(CSProperty::dictionaryName) )
    {
        return true;
    }

    return false;
}


bool DictionarySource::HasEmbeddedDictionaryWhenCreated(const ConnectionString& connection_string)
{
    return ( DataRepositoryHelpers::IsTypeFileBasedWithAnEmbeddedDictionary(connection_string.GetType()) ||
             connection_string.GetType() == DataRepositoryType::CSWeb );
}


bool DictionarySource::HasAssociatedDictionary(const ConnectionString& connection_string)
{
    // a connection string has an associated dictionary when a dictionary path override is specified
    // or if the data source has an embedded dictionary
    return ( connection_string.HasPropertyWithValue(CSProperty::dictionaryPath) ||
             HasEmbeddedDictionary(connection_string) );
}


std::unique_ptr<CDataDict> DictionarySource::GetEmbeddedDictionary(const ConnectionString& connection_string)
{
    if( !connection_string.IsDefined() )
        return nullptr;

    if( connection_string.GetType() == DataRepositoryType::SQLite )
    {
        return SQLiteRepository::GetEmbeddedDictionary(connection_string);
    }

    else if( connection_string.GetType() == DataRepositoryType::EncryptedSQLite )
    {
        return EncryptedSQLiteRepository::GetEmbeddedDictionary(connection_string);
    }

    else if( connection_string.GetType() == DataRepositoryType::CSWeb )
    {
        const std::string* const dictionary_name_override = connection_string.GetProperty(CSProperty::dictionaryName);

        if( dictionary_name_override != nullptr && !dictionary_name_override->empty() )
            return CSWebRepository::GetDictionary(*dictionary_name_override, connection_string);
    }

    return nullptr;
}


std::unique_ptr<CDataDict> DictionarySource::GetAssociatedDictionary(const ConnectionString& connection_string)
{
    // prioritize overridden dictionary paths over embedded dictionaries
    const std::string dictionary_path_override = GetDictionaryPathOverride(connection_string);

    return !dictionary_path_override.empty() ? CDataDict::InstantiateAndOpen(dictionary_path_override, true) :
                                               GetEmbeddedDictionary(connection_string);
}


DictionarySource::DictionarySource(ConnectionString connection_string)
    :   m_connectionString(std::move(connection_string))
{
}


DictionarySource::DictionarySource(const std::string_view connection_string_text_sv)
    :   DictionarySource(ConnectionString(connection_string_text_sv))
{
}


void DictionarySource::Reset()
{
    m_connectionString.Clear();
    m_dictionary.reset();
    m_dictionaryFilePathStorage.emplace<std::monostate>();
}


bool DictionarySource::IsDictionaryFilePath(const ConnectionString& connection_string)
{
    return ( connection_string.HasFilePath() &&
             SO::EqualsNoCase(Path::GetExtension(connection_string.GetFilePath()), FileExtensions::Dictionary) );
}


std::shared_ptr<const CDataDict> DictionarySource::GetAssociatedDictionary()
{
    if( !m_connectionString.IsDefined() )
        return nullptr;

    // load dictionaries directly
    if( IsDictionaryFilePath(m_connectionString) )
    {
        m_dictionary = CDataDict::InstantiateAndOpen(m_connectionString.GetFilePath(), true);
        m_dictionaryFilePathStorage = m_connectionString.GetFilePath();
    }

    else
    {
        // prioritize overridden dictionary paths over embedded dictionaries
        std::string dictionary_path_override = GetDictionaryPathOverride(m_connectionString);

        if( !dictionary_path_override.empty() )
        {
            m_dictionary = CDataDict::InstantiateAndOpen(dictionary_path_override, true);
            m_dictionaryFilePathStorage = std::move(dictionary_path_override);
        }

        else
        {
            m_dictionary = GetEmbeddedDictionary(m_connectionString);

            if( m_dictionary != nullptr )
                m_dictionaryFilePathStorage.emplace<std::shared_ptr<TemporaryFile>>();
        }
    }

    ASSERT(( m_dictionary == nullptr ) == std::holds_alternative<std::monostate>(m_dictionaryFilePathStorage));

    return m_dictionary;
}


std::shared_ptr<const CDataDict> DictionarySource::GetDictionary()
{
    try
    {
        GetAssociatedDictionary();
    }

    catch( const CSProException& exception )
    {
        throw CSProException("Error loading dictionary '%s': %s", m_connectionString.ToDisplayString(true).c_str(), exception.what());
    }

    if( m_dictionary == nullptr )
        throw CSProException("The dictionary could not be loaded: " + m_connectionString.ToDisplayString());

    return m_dictionary;
}


bool DictionarySource::UsingEmbeddedDictionary() const
{
    ASSERT(!std::holds_alternative<std::monostate>(m_dictionaryFilePathStorage));

    return std::holds_alternative<std::shared_ptr<TemporaryFile>>(m_dictionaryFilePathStorage);
}


const std::string& DictionarySource::GetSourceFilePath() const
{
    if( std::holds_alternative<std::string>(m_dictionaryFilePathStorage) )
    {
        return std::get<std::string>(m_dictionaryFilePathStorage);
    }

    else if( std::holds_alternative<std::shared_ptr<TemporaryFile>>(m_dictionaryFilePathStorage) )
    {
        return m_connectionString.HasFilePath() ? m_connectionString.GetFilePath() :
                                                  SO::Empty_string;
    }

    else
    {
        return ReturnProgrammingError(SO::Empty_string);
    }
}


const std::string& DictionarySource::GetDictionaryFilePath() const
{
    if( std::holds_alternative<std::string>(m_dictionaryFilePathStorage) )
    {
        return std::get<std::string>(m_dictionaryFilePathStorage);
    }

    else
    {
        ASSERT(std::holds_alternative<std::shared_ptr<TemporaryFile>>(m_dictionaryFilePathStorage));
        return SO::Empty_string;
    }
}


const std::string& DictionarySource::GetFileBasedDictionaryFilePath()
{
    if( std::holds_alternative<std::string>(m_dictionaryFilePathStorage) )
    {
        return std::get<std::string>(m_dictionaryFilePathStorage);
    }

    else if( std::holds_alternative<std::shared_ptr<TemporaryFile>>(m_dictionaryFilePathStorage) )
    {
        if( std::get<std::shared_ptr<TemporaryFile>>(m_dictionaryFilePathStorage) == nullptr )
        {
            std::get<std::shared_ptr<TemporaryFile>>(m_dictionaryFilePathStorage) = std::make_unique<TemporaryFile>();
            m_dictionary->Save(std::get<std::shared_ptr<TemporaryFile>>(m_dictionaryFilePathStorage)->GetPath());
        }

        return std::get<std::shared_ptr<TemporaryFile>>(m_dictionaryFilePathStorage)->GetPath();
    }

    else
    {
        return ReturnProgrammingError(SO::Empty_string);
    }
}


DictionarySource DictionarySource::CreateFromJson(const JsonNode& json_node)
{
    return DictionarySource(json_node.Get<ConnectionString>());
}


void DictionarySource::WriteJson(JsonWriter& json_writer) const
{
    // write dictionary file paths as strings rather than connection strings
    if( IsDictionaryFilePath(m_connectionString) )
    {
        json_writer.WriteRelativePath(m_connectionString.GetFilePath());
    }

    else
    {
        json_writer.Write(m_connectionString);
    }
}
