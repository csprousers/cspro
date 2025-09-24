#include "stdafx.h"
#include "JsonRepository.h"
#include "JsonRepositoryBinaryDataIO.h"
#include "JsonRepositoryIndexCreator.h"
#include "JsonRepositoryIterators.h"
#include <zToolsO/DirectoryLister.h>
#include <zToolsO/FileIO.h>
#include <zSql/Commands.h>
#include <zSql/SQLiteHelpers.h>
#include <zJson/JsonStream.h>


namespace
{
    // definitions
    constexpr const char* FilesDirectoryNameSuffix            = " (files)";

    constexpr std::string_view JsonArrayNewFile_sv            = "[\n]";
    constexpr std::string_view JsonArrayStart_sv              = "[\n";
    constexpr std::string_view JsonArrayEnd_sv                = "\n]";
    constexpr std::string_view JsonArrayCaseSeparatorComma_sv = ",\n";

    constexpr size_t IndexCreatorFillBufferSize               = 32 * 1024;
    constexpr size_t GrowFileBufferSize                       = 256 * 1024;


    // error messages
    namespace ErrorText
    {
        constexpr const char* DirectoryNotExistFormatter          = "The directory does not exist and could not be created: %s";
        constexpr const char* FileNotExistFormatter               = "The data file does not exist: %s";

        constexpr const char* FileCreateNewError                  = "Could not create a new data file.";
        constexpr const char* FileOpenError                       = "The data file could not be opened.";

        constexpr const char* FileInvalidEncoding                 = "CSPro does not support the specified text encoding: %s";

        constexpr const char* JsonParseExceptionPrefix            = "There was an error parsing the JSON data file: ";
        constexpr const char* JsonInvalidStart                    = "The JSON data file does not start with a '['";
        constexpr const char* JsonInvalidEnd                      = "The JSON data file does not end with a ']'";
        constexpr const char* JsonCharacterNotFoundFormatter      = "There was an error parsing the JSON data file looking for %s around position %d";
        constexpr const char* JsonCaseParseExceptionFormatter     = "There was an error parsing a case in the JSON data file at position %d: %s";

        constexpr const char* BinaryDataNoSignature               = "No signature was found describing how to access the binary data file.";
        constexpr const char* BinaryDataFileNotFoundFormatter     = "No file with the signature '%s' could be found as part of the JSON data file.";
        constexpr const char* BinaryDataFileTooManyFilesFormatter = "Too many files with the signature '%s' were found as part of the JSON data file in the directory: %s";
    }


    // exception modifiers
    [[noreturn]] void RethrowJsonParseException(const JsonParseException& exception)
    {
        const std::string message = std::string(ErrorText::JsonParseExceptionPrefix) + exception.what();
        throw DataRepositoryException::IOError(message.c_str());
    }
}


// SQLite statements
namespace Sqlite::Commands
{
    constexpr const char* CreateKeyTable = "CREATE TABLE `keys` (`uuid` TEXT PRIMARY KEY UNIQUE NOT NULL, `key` TEXT NOT NULL, "
                                                                "`label` TEXT NOT NULL, `note` TEXT NOT NULL, "
                                                                "`deleted` INTEGER NOT NULL, `verified` INTEGER NOT NULL, `partial` INTEGER NOT NULL, "
                                                                "`position` INTEGER NOT NULL, `bytes` INTEGER NOT NULL) WITHOUT ROWID;";

    constexpr const char* CreateKeyTableKeyIndex      = "CREATE INDEX `keys-key-index` ON `keys` ( `key` );";
    constexpr const char* CreateKeyTablePositionIndex = "CREATE INDEX `keys-position-index` ON `keys` ( `position` );";

    constexpr const char* InsertCase         = "INSERT INTO `keys` (`uuid`, `key`, `label`, `note`, `deleted`, `verified`, `partial`, `position`, `bytes`) VALUES( ?, ?, ?, ?, ?, ?, ?, ?, ? );";
    constexpr const char* ModifyCase         = "UPDATE `keys` SET `key` = ?, `label` = ?, `note` = ?, `deleted` = ?, `verified` = ?, `partial` = ?, `position` = ?, `bytes` = ? WHERE `uuid` = ?;";
    constexpr const char* ModifyCaseBytes    = "UPDATE `keys` SET `bytes` = ? WHERE `uuid` = ?;";
    constexpr const char* ShiftCasePositions = "UPDATE `keys` SET `position` = `position` + ? WHERE `position` >= ?;";

    constexpr const char* UuidExists = "SELECT 1 FROM `keys` WHERE `uuid` = ? LIMIT 1;";

    constexpr const char* NotDeletedKeyExists = "SELECT 1 FROM `keys` WHERE `key` = ? AND `deleted` = 0 LIMIT 1;";
    constexpr const char* CountNotDeletedKeys = "SELECT COUNT(*) FROM `keys` WHERE `deleted` = 0;";

    constexpr const char* QueryPositionBytesByNotDeletedKey = "SELECT `position`, `bytes` FROM `keys` WHERE `key` = ? AND `deleted` = 0 ORDER BY `position` LIMIT 1;";
    constexpr const char* QueryPositionUuidByNotDeletedKey  = "SELECT `position`, `uuid` FROM `keys` WHERE `key` = ? AND `deleted` = 0 ORDER BY `position` LIMIT 1;";

    constexpr const char* QueryPositionBytesByUuid = "SELECT `position`, `bytes` FROM `keys` WHERE `uuid` = ? LIMIT 1;";
    constexpr const char* QueryKeyPositionByUuid   = "SELECT `key`, `position` FROM `keys` WHERE `uuid` = ? LIMIT 1;";

    constexpr const char* QueryBytesByPosition   = "SELECT `bytes` FROM `keys` WHERE `position` = ? LIMIT 1;";
    constexpr const char* QueryKeyUuidByPosition = "SELECT `key`, `uuid` FROM `keys` WHERE `position` = ? LIMIT 1;";
    constexpr const char* QueryUuidByPosition    = "SELECT `uuid` FROM `keys` WHERE `position` = ? LIMIT 1;";

    constexpr const char* QueryIsLastPosition        = "SELECT 1 FROM `keys` WHERE `position` > ? LIMIT 1;";
    constexpr const char* QueryLastUuidBytesPosition = "SELECT `uuid`, `position`, `bytes` FROM `keys` ORDER BY `position` DESC LIMIT 1;";
}



// --------------------------------------------------------------------------
// JsonRepository
// --------------------------------------------------------------------------

JsonRepository::JsonRepository(std::shared_ptr<const CaseAccess> case_access, const DataRepositoryAccess access_type)
    :   IndexableTextRepository(DataRepositoryType::Json, std::move(case_access), access_type),
        m_file(nullptr),
        m_fileSize(0),
        m_filePosition(0),
        m_writeCommaBeforeNextCase(false),
        m_endArrayOnFileClose(false),
        m_jsonFormatCompact(false),
        m_useTransactionManager(false),
        m_numberTransactions(0)
{
    ModifyCaseAccess(m_caseAccess);
}


JsonRepository::~JsonRepository()
{
    try
    {
        Close();
    }

    catch( const DataRepositoryException::Error& )
    {
        // ignore errors
    }
}


void JsonRepository::ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access)
{
    m_caseAccess = std::move(case_access);
    m_caseJsonParserHelper = std::make_unique<JsonRepositoryCaseJsonParserHelper>(*this);
    m_temporaryCases.clear();
}


std::shared_ptr<Case> JsonRepository::GetTemporaryCase()
{
    // reuse a case if possible
    for( const std::shared_ptr<Case>& data_case : m_temporaryCases )
    {
        if( data_case.use_count() == 1 )
            return data_case;
    }

    // otherwise create a new one
    return m_temporaryCases.emplace_back(m_caseAccess->CreateCase());
}


void JsonRepository::Open(const DataRepositoryOpenFlag open_flag)
{
    const bool can_create_file = ( open_flag == DataRepositoryOpenFlag::CreateNew || open_flag == DataRepositoryOpenFlag::OpenOrCreate );

    if( can_create_file && !PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(m_connectionString.GetFilePath())) )
    {
        throw DataRepositoryException::IOError(ErrorText::DirectoryNotExistFormatter, PortableFunctions::PathGetDirectory(m_connectionString.GetFilePath()).c_str());
    }

    const bool file_exists = PortableFunctions::FileIsRegular(m_connectionString.GetFilePath());

    if( open_flag == DataRepositoryOpenFlag::OpenMustExist && !file_exists )
    {
        throw DataRepositoryException::IOError(ErrorText::FileNotExistFormatter, m_connectionString.GetFilePath().c_str());
    }

    // create a data file if one doesn't exist, if setfile/open used the clear flag,
    // or if opening in batch output mode, to overwrite what is already on the disk
    const bool create_new_file = ( !file_exists || open_flag == DataRepositoryOpenFlag::CreateNew || m_accessType == DataRepositoryAccess::BatchOutput );

    if( create_new_file )
    {
        DeleteRepositoryFiles(m_connectionString);

        try
        {
            FileIO::Write(m_connectionString.GetFilePath(), JsonArrayNewFile_sv);
        }

        catch( const CSProException& )
        {
            throw DataRepositoryException::IOError(ErrorText::FileCreateNewError);
        }
    }

    else
    {
        // check the encoding
        const TextEncoding text_encoding = TextEncoding::ReadFileBom(m_connectionString.GetFilePath());

        if( !text_encoding.IsUtf8() )
            throw DataRepositoryException::IOError(ErrorText::FileInvalidEncoding, text_encoding.ToString());
    }

    // open the data file
    OpenDataFile();

    // if the data file is searchable, then we need to make sure that an index exists
    if( m_requiresIndex )
        CreateOrOpenIndex(create_new_file);

    // unless this is the main data file for an entry application, wrap interactions in transactions
    if( m_accessType != DataRepositoryAccess::EntryInput )
    {
        m_useTransactionManager = true;
        TransactionManager::Register(*this);
    }
}


void JsonRepository::ToggleReadWriteMode()
{
    ASSERT(m_accessType == DataRepositoryAccess::ReadOnly || m_accessType == DataRepositoryAccess::ReadWrite);
    CloseDataFile();

    m_accessType = ( m_accessType == DataRepositoryAccess::ReadOnly ) ? DataRepositoryAccess::ReadWrite :
                                                                        DataRepositoryAccess::ReadOnly;
    OpenDataFile();
}


void JsonRepository::Close()
{
    if( m_useTransactionManager )
    {
        CommitTransactions();
        TransactionManager::Deregister(*this);
    }

    CloseDataFile();
    CloseIndex();
}


void JsonRepository::CloseDataFile()
{
    if( m_file != nullptr )
    {
        bool error_closing = false;

        if( m_endArrayOnFileClose )
        {
            PortableFunctions::fseeki64(m_file, 0, SEEK_END);
            error_closing = ( fwrite(JsonArrayEnd_sv.data(), 1, JsonArrayEnd_sv.size(), m_file) != JsonArrayEnd_sv.size() );
        }

        fclose(m_file);
        m_file = nullptr;

        if( error_closing )
            throw DataRepositoryException::GenericWriteError();
    }

    m_jsonStream.reset();
}


std::variant<std::monostate, std::string> JsonRepository::GetBinaryDataOutput(const ConnectionString& connection_string)
{
    // by default, binary data is written to the disk, but it can be embedded as a data URL;
    // when there is no filename, data will always be embedded
    if( connection_string.HasProperty(CSProperty::binaryDataFormat, CSValue::dataUrl) ||
        !connection_string.HasFilePath() )
    {
        return std::monostate();
    }

    // the directory name can be specified in the connection string
    const std::string* const directory_name_override = connection_string.GetProperty(CSProperty::binaryDataDirectory);

    if( directory_name_override != nullptr )
    {
        const std::string base_directory = PortableFunctions::PathGetDirectory(connection_string.GetFilePath());
        return MakeFullPath(base_directory, *directory_name_override);
    }

    else
    {
        return GetDefaultBinaryDataDirectory(connection_string);
    }
}


std::string JsonRepository::GetDefaultBinaryDataDirectory(const ConnectionString& connection_string)
{
    if( !connection_string.HasFilePath() )
        return std::string();

    const std::string base_directory = PortableFunctions::PathGetDirectory(connection_string.GetFilePath());

    return MakeFullPath(base_directory, PortableFunctions::PathGetFilename(connection_string.GetFilePath())) + FilesDirectoryNameSuffix;
}


void JsonRepository::DeleteRepository()
{
    Close();

    DeleteRepositoryFiles(m_connectionString);
}


void JsonRepository::DeleteRepositoryFiles(const ConnectionString& connection_string, const bool delete_binary_data_directory/* = true*/)
{
    auto delete_file = [](const std::string& file_path)
    {
        if( PortableFunctions::FileIsRegular(file_path) && !PortableFunctions::FileDelete(file_path) )
            throw DataRepositoryException::DeleteRepositoryError();
    };

    // delete the data and index
    delete_file(connection_string.GetFilePath());
    delete_file(PortableFunctions::PathAppendFileExtension(connection_string.GetFilePath(), FileExtensions::Data::IndexableTextIndex));

    // potentially delete the binary data
    if( delete_binary_data_directory )
    {
        const std::variant<std::monostate, std::string> binary_data_output = GetBinaryDataOutput(connection_string);

        if( std::holds_alternative<std::string>(binary_data_output) &&
            PortableFunctions::FileIsDirectory(std::get<std::string>(binary_data_output)) &&
            !PortableFunctions::DirectoryDelete(std::get<std::string>(binary_data_output), true) )
        {
            throw DataRepositoryException::DeleteRepositoryError();
        }
    }
}


void JsonRepository::RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string)
{
    const std::variant<std::monostate, std::string> old_binary_data_output = GetBinaryDataOutput(old_connection_string);
    std::variant<std::monostate, std::string> new_binary_data_output = GetBinaryDataOutput(new_connection_string);

    // if the user hard-coded a binary data directory, we do not want to delete it
    const bool delete_binary_data_directory = ( old_binary_data_output != new_binary_data_output );

    DeleteRepositoryFiles(new_connection_string, delete_binary_data_directory);

    auto rename_file = [](const std::string& old_file_path, const std::string& new_file_path)
    {
        if( PortableFunctions::FileIsRegular(old_file_path) && !PortableFunctions::FileRename(old_file_path, new_file_path) )
            throw DataRepositoryException::RenameRepositoryError();
    };

    // rename the data, index, and binary data
    rename_file(old_connection_string.GetFilePath(), new_connection_string.GetFilePath());

    rename_file(PortableFunctions::PathAppendFileExtension(old_connection_string.GetFilePath(), FileExtensions::Data::IndexableTextIndex),
                PortableFunctions::PathAppendFileExtension(new_connection_string.GetFilePath(), FileExtensions::Data::IndexableTextIndex));

    if( std::holds_alternative<std::string>(old_binary_data_output) &&
        PortableFunctions::FileIsDirectory(std::get<std::string>(old_binary_data_output)) )
    {
        // if the new file uses embedded binary data, the directory will not be defined; ideally in this case, the
        // binary data would be written into the new data file, but for now, simply force a directory for the new data
        if( !std::holds_alternative<std::string>(new_binary_data_output) )
        {
            new_binary_data_output = GetDefaultBinaryDataDirectory(new_connection_string);

            if( !std::holds_alternative<std::string>(new_binary_data_output) )
            {
                ASSERT(false);
                return;
            }
        }

        if( !PortableFunctions::DirectoryRename(std::get<std::string>(old_binary_data_output), std::get<std::string>(new_binary_data_output)) )
            throw DataRepositoryException::RenameRepositoryError();
    }
}


std::vector<std::string> JsonRepository::GetAssociatedFileList(const ConnectionString& connection_string)
{
    std::vector<std::string> associated_files;

    const std::variant<std::monostate, std::string> binary_data_output = GetBinaryDataOutput(connection_string);

    if( std::holds_alternative<std::string>(binary_data_output) )
        associated_files = DirectoryLister().GetPaths(std::get<std::string>(binary_data_output));

    associated_files.emplace_back(connection_string.GetFilePath());

    return associated_files;
}


void JsonRepository::OpenDataFile()
{
    try
    {
        m_file = nullptr;
        m_jsonStream.reset();
        m_jsonStreamObjectArrayIterator.reset();
        m_writeCommaBeforeNextCase = false;
        m_endArrayOnFileClose = false;
        m_binaryDataOutput = GetBinaryDataOutput(m_connectionString);
        m_caseJsonWriterSerializerHelper.reset();

        if( m_accessType == DataRepositoryAccess::BatchInput )
        {
            m_jsonStream = std::make_unique<JsonStream>(JsonStream::FromFile(m_connectionString.GetFilePath()));
        }

        else
        {
            OpenIndexedDataFile();
        }
    }

    catch( const CSProException& exception )
    {
        ASSERT(m_file == nullptr && m_jsonStream == nullptr);
        throw DataRepositoryException::IOError(exception.what());
    }
}


void JsonRepository::OpenIndexedDataFile()
{
    ASSERT(m_file == nullptr);

    m_file = PortableFunctions::FileOpen(m_connectionString.GetFilePath(), IsReadOnly() ? "rb" : "rb+");

    if( m_file == nullptr )
        throw DataRepositoryException::IOError(ErrorText::FileOpenError);

    try
    {
        // run batch output-specific tasks
        if( m_accessType == DataRepositoryAccess::BatchOutput || m_accessType == DataRepositoryAccess::BatchOutputAppend )
        {
            VerifyBatchOutputDataFile();
        }

        else
        {
            // determine the size of the file
            PortableFunctions::fseeki64(m_file, 0, SEEK_END);
            m_fileSize = PortableFunctions::ftelli64(m_file);
            m_filePosition = m_fileSize;
        }
    }

    catch(...)
    {
        fclose(m_file);
        m_file = nullptr;

        throw;
    }
}


void JsonRepository::OpenBatchInputDataFileAsIndexed()
{
    ASSERT(m_file == nullptr && m_jsonStream != nullptr);

    try
    {
        OpenIndexedDataFile();
    }

    catch( const CSProException& exception )
    {
        ASSERT(m_file == nullptr);
        throw DataRepositoryException::IOError(exception.what());
    }
}


void JsonRepository::VerifyBatchOutputDataFile()
{
    ASSERT(m_file != nullptr && PortableFunctions::ftelli64(m_file) == 0);

    // skip past the BOM if necessary
    const TextEncoding text_encoding(m_file);
    ASSERT(text_encoding.IsUtf8());

    // see if a JSON array has been started; the array start character must be
    // the first non-whitespace character in file (unless the file is empty)
    int ch;

    while( ( ch = fgetc(m_file) ) != EOF && std::isspace(ch) )
    {
    }

    if( ch == EOF )
    {
        // if the file was empty, write out the JSON array markers
        PortableFunctions::fseeki64(m_file, 0, SEEK_SET);

        if( fwrite(JsonArrayNewFile_sv.data(), 1, JsonArrayNewFile_sv.size(), m_file) != JsonArrayNewFile_sv.size() )
            throw DataRepositoryException::GenericWriteError();
    }

    else if( ch != '[' )
    {
        throw DataRepositoryException::IOError(ErrorText::JsonInvalidStart);
    }

    // determine the size of the file
    PortableFunctions::fseeki64(m_file, 0, SEEK_END);
    m_fileSize = PortableFunctions::ftelli64(m_file);
    m_filePosition = m_fileSize;

    // move to the location of the array end character and truncate the file
    constexpr int64_t FirstPositionOfNonStartArrayCharacter = 1;

    auto reverse_find_first_non_whitespace_char = [&](int64_t position)
    {
        for( ; position >= FirstPositionOfNonStartArrayCharacter; --position )
        {
            PortableFunctions::fseeki64(m_file, position, SEEK_SET);
            ch = fgetc(m_file);

            if( !std::isspace(ch) )
                break;
        }

        return position;
    };

    m_filePosition = reverse_find_first_non_whitespace_char(m_fileSize - 1);

    if( ch != ']' )
        throw DataRepositoryException::IOError(ErrorText::JsonInvalidEnd);

    m_endArrayOnFileClose = true;

    // if appending cases, check if there are existing cases
    if( m_accessType == DataRepositoryAccess::BatchOutputAppend )
    {
        const int64_t potential_right_bracket_pos = reverse_find_first_non_whitespace_char(m_filePosition - 1);

        if( ch == '}' )
        {
            m_filePosition = potential_right_bracket_pos + 1;
            m_writeCommaBeforeNextCase = true;
        }
    }

    else
    {
        ASSERT(m_accessType == DataRepositoryAccess::BatchOutput && m_filePosition == 2);
    }

    m_fileSize = m_filePosition;

    if( !PortableFunctions::FileTruncate(m_file, m_filePosition) )
        throw DataRepositoryException::GenericWriteError();
}


uint32_t JsonRepository::GetIdStructureHashForKeyIndex() const
{
    return m_caseAccess->GetDataDict().GetIdStructureHashForKeyIndex(true, false);
}


std::shared_ptr<IndexableTextRepository::IndexCreator> JsonRepository::GetIndexCreator()
{
    return std::make_shared<JsonRepositoryIndexCreator>(*this);
}


std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>> JsonRepository::GetSqlStatementsToPrepare()
{
    return std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>>
    {
        { Sqlite::Commands::InsertCase, m_stmtInsertCase },
        { Sqlite::Commands::UuidExists, m_stmtUuidExists }
    };
}


std::variant<const char*, std::shared_ptr<SQLiteStatement>> JsonRepository::GetSqlStatementForQuery(const SqlQueryType type)
{
    switch( type )
    {
        case SqlQueryType::ContainsNotDeletedKey:             return Sqlite::Commands::NotDeletedKeyExists;
        case SqlQueryType::GetPositionBytesFromNotDeletedKey: return Sqlite::Commands::QueryPositionBytesByNotDeletedKey;
        case SqlQueryType::GetBytesFromPosition:              return Sqlite::Commands::QueryBytesByPosition;
        case SqlQueryType::CountNotDeletedKeys:               return Sqlite::Commands::CountNotDeletedKeys;
    }

    throw ProgrammingErrorException();
}


void JsonRepository::PopulateCaseIdentifiers(std::string& key, std::string& uuid, double& position_in_repository)
{
    // search the index by key
    if( !key.empty() )
    {
        EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryPositionUuidByNotDeletedKey, m_stmtQueryPositionUuidByKey);
        const SQLiteResetOnDestruction rod(*m_stmtQueryPositionUuidByKey);

        m_stmtQueryPositionUuidByKey->Bind(1, key);

        if( m_stmtQueryPositionUuidByKey->Step() != SQLITE_ROW )
            throw DataRepositoryException::CaseNotFound();

        position_in_repository = m_stmtQueryPositionUuidByKey->GetColumn<double>(0);
        uuid = m_stmtQueryPositionUuidByKey->GetColumn<std::string>(1);
    }

    // or by UUID
    else if( !uuid.empty() )
    {
        EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryKeyPositionByUuid, m_stmtQueryKeyPositionByUuid);
        const SQLiteResetOnDestruction rod(*m_stmtQueryKeyPositionByUuid);

        m_stmtQueryKeyPositionByUuid->Bind(1, uuid);

        if( m_stmtQueryKeyPositionByUuid->Step() != SQLITE_ROW )
            throw DataRepositoryException::CaseNotFound();

        key = m_stmtQueryKeyPositionByUuid->GetColumn<std::string>(0);
        position_in_repository = m_stmtQueryKeyPositionByUuid->GetColumn<double>(1);
    }

    // or by file position
    else
    {
        EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryKeyUuidByPosition, m_stmtQueryKeyUuidByPosition);
        const SQLiteResetOnDestruction rod(*m_stmtQueryKeyUuidByPosition);

        m_stmtQueryKeyUuidByPosition->Bind(1, static_cast<int64_t>(position_in_repository));

        if( m_stmtQueryKeyUuidByPosition->Step() != SQLITE_ROW )
            throw DataRepositoryException::CaseNotFound();

        key = m_stmtQueryKeyUuidByPosition->GetColumn<std::string>(0);
        uuid = m_stmtQueryKeyUuidByPosition->GetColumn<std::string>(1);
    }
}


DataRepositoryUniqueCaseIdentifer JsonRepository::GetUniqueCaseIdentifer(const CaseKey& case_key)
{
    EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryUuidByPosition, m_stmtQueryUuidByPosition);
    const SQLiteResetOnDestruction rod(*m_stmtQueryUuidByPosition);

    m_stmtQueryUuidByPosition->Bind(1, static_cast<int64_t>(case_key.GetPositionInRepository()));

    if( m_stmtQueryUuidByPosition->Step() != SQLITE_ROW )
        throw DataRepositoryException::CaseNotFound();

    return DataRepositoryUniqueCaseIdentifer(DataRepositoryUniqueCaseIdentifer::Type::Uuid,
                                             m_stmtQueryUuidByPosition->GetColumn<std::string>(0));
}


SQLiteStatement JsonRepository::CreateKeySearchIteratorStatement(const char* const columns_to_query,
                                                                 const CaseIteratorSettings& iterator_settings,
                                                                 const size_t offset, const size_t limit)
{
    std::string order_by_text;

    if( iterator_settings.GetMethod().has_value() )
    {
        const CaseIterationMethod iteration_method = *iterator_settings.GetMethod();
        const std::optional<CaseIterationOrder>& iteration_order = iterator_settings.GetOrder();

        const char* const column = ( iteration_method == CaseIterationMethod::KeyOrder ) ? "`key`" :
                                                                                           "`position`";

        const char* const order = ( !iteration_order.has_value() )                       ? "" :
                                  ( *iteration_order == CaseIterationOrder::Ascending )  ? "ASC" :
                                                                                           "DESC";

        order_by_text = FormatText("ORDER BY %s %s ", column, order);
    }

    const std::string limit_text = FormatText("LIMIT %d OFFSET %d ", ( limit == SIZE_MAX ) ? -1 : static_cast<int>(limit), static_cast<int>(offset));

    std::string where_text;

    auto add_to_where_text = [&](const auto& condition)
    {
        where_text.append(where_text.empty() ? "WHERE (" : "AND (")
                  .append(condition)
                  .append(") ");
    };

    // process any filters
    const CaseIteratorParameters* const start_parameters = iterator_settings.GetParameters();
    bool use_key_prefix = false;
    bool use_operators = false;

    if( start_parameters != nullptr )
    {
        // use the key prefix if it is set and is not empty
        if( start_parameters->key_prefix.has_value() && !start_parameters->key_prefix->empty() )
        {
            use_key_prefix = true;
            add_to_where_text("`key` >= ? AND `key` < ?");

            use_operators = std::holds_alternative<std::string>(start_parameters->first_key_or_position) ?
                !std::get<std::string>(start_parameters->first_key_or_position).empty() :
                ( std::get<double>(start_parameters->first_key_or_position) != -1 );
        }

        else
        {
            use_operators = true;
        }

        if( use_operators )
        {

            add_to_where_text(FormatText("%s %s ?", std::holds_alternative<std::string>(start_parameters->first_key_or_position) ? "`key`" : "`position`",
                                                    ToString(start_parameters->start_type)));
        }
    }

    // filter on case properties
    if( iterator_settings.GetStatus() != CaseIterationCaseStatus::All )
    {
        add_to_where_text("`deleted` = 0");

        if( iterator_settings.GetStatus() == CaseIterationCaseStatus::PartialsOnly )
        {
            add_to_where_text("`partial` != 0");
        }

        else if( iterator_settings.GetStatus() == CaseIterationCaseStatus::DuplicatesOnly )
        {
            add_to_where_text("`key` IN ( SELECT `key` FROM `keys` WHERE `deleted` = 0 GROUP BY `key` HAVING COUNT(*) > 1 )");
        }
    }

    const std::string sql = FormatText("SELECT %s FROM `keys` %s %s %s;", columns_to_query,
                                                                          where_text.c_str(),
                                                                          order_by_text.c_str(),
                                                                          limit_text.c_str());

    SQLiteStatement stmt_query_keys = PrepareSqlStatementForQuery(sql);

    if( use_key_prefix )
    {
        stmt_query_keys.Bind(1, *start_parameters->key_prefix)
                       .Bind(2, SQLiteHelpers::GetTextPrefixBoundary(*start_parameters->key_prefix));
    }

    if( use_operators )
    {
        const int operator_argument_index = use_key_prefix ? 3 : 1;

        if( std::holds_alternative<std::string>(start_parameters->first_key_or_position) )
        {
            stmt_query_keys.Bind(operator_argument_index, std::get<std::string>(start_parameters->first_key_or_position));
        }

        else
        {
            stmt_query_keys.Bind(operator_argument_index, static_cast<int64_t>(std::get<double>(start_parameters->first_key_or_position)));
        }
    }

    return stmt_query_keys;
}


std::optional<CaseKey> JsonRepository::FindCaseKey(const CaseIterationMethod iteration_method, const CaseIterationOrder iteration_order,
                                                   const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    const CaseIteratorSettings iterator_settings(CaseIterationCaseStatus::NotDeletedOnly, iteration_method, iteration_order, start_parameters);

    SQLiteStatement stmt_query_keys = CreateKeySearchIteratorStatement("`key`, `position`", iterator_settings, 0, 1);

    if( stmt_query_keys.Step() == SQLITE_ROW )
        return CaseKey(stmt_query_keys.GetColumn<std::string>(0), stmt_query_keys.GetColumn<double>(1));

    return std::nullopt;
}


inline void JsonRepository::SetFilePosition(const int64_t file_position)
{
    ASSERT(file_position >= 0 && file_position <= m_fileSize);

    if( file_position != m_filePosition )
    {
        PortableFunctions::fseeki64(m_file, file_position, SEEK_SET);
        m_filePosition = file_position;

        ASSERT(m_filePosition == PortableFunctions::ftelli64(m_file));
    }
}


void JsonRepository::ReadCase(Case& data_case, const int64_t file_position, const size_t bytes_for_case)
{
    if( m_file == nullptr )
    {
        if( m_accessType == DataRepositoryAccess::BatchInput )
        {
            // CSIndex opens data files in batch input mode, but calls this method
            // when reading duplicates, so open the data file so that m_file is set
            // (despite OpenBatchInputDataFileAsIndexed's name, it doesn't actually require/open an index)
            OpenBatchInputDataFileAsIndexed();
        }

        else
        {
            throw DataRepositoryException::GenericReadError();
        }
    }

    SetFilePosition(file_position);

    // create a decent amount of space in the buffer to avoid many reallocations
    if( bytes_for_case > m_buffer.size() )
        m_buffer.resize(bytes_for_case * 2);

    if( fread(m_buffer.data(), 1, bytes_for_case, m_file) != bytes_for_case )
        throw DataRepositoryException::GenericReadError();

    m_filePosition += bytes_for_case;

    std::string_view case_json_sv(m_buffer.data(), bytes_for_case);

    // if this is not the last case in the file, it will have a comma at the end,
    // separating it in the JSON array from subsequent cases
    case_json_sv = SO::TrimRight(SO::TrimRight(case_json_sv), ',');

    try
    {
        ConvertJsonToCase(data_case, Json::Parse(case_json_sv));
        data_case.SetPositionInRepository(static_cast<double>(file_position));
    }

    catch( const JsonParseException& exception )
    {
        RethrowJsonParseException(exception);
    }
}


void JsonRepository::ReadCaseByUuid(Case& data_case, const std::string& uuid)
{
    EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryPositionBytesByUuid, m_stmtQueryPositionBytesByUuid);
    const SQLiteResetOnDestruction rod(*m_stmtQueryPositionBytesByUuid);

    m_stmtQueryPositionBytesByUuid->Bind(1, uuid);

    if( m_stmtQueryPositionBytesByUuid->Step() != SQLITE_ROW )
        throw DataRepositoryException::CaseNotFound();

    ReadCase(data_case, m_stmtQueryPositionBytesByUuid->GetColumn<int64_t>(0),
                        m_stmtQueryPositionBytesByUuid->GetColumn<size_t>(1));

    ASSERT(data_case.GetUuid() == uuid);
}


inline void JsonRepository::WriteData(const void* buffer, const size_t size, const PostWriteDataAction post_write_data_action)
{
    if( fwrite(buffer, 1, size, m_file) != size )
        throw DataRepositoryException::GenericWriteError();

    m_filePosition += size;

    ASSERT(m_filePosition == PortableFunctions::ftelli64(m_file));

    if( post_write_data_action != PostWriteDataAction::Nothing )
    {
        if( post_write_data_action == PostWriteDataAction::UpdateFileSizeAndTruncate && m_filePosition < m_fileSize )
        {
            if( !PortableFunctions::FileTruncate(m_file, m_filePosition) )
                throw DataRepositoryException::GenericWriteError();
        }

        m_fileSize = m_filePosition;
    }
}


void JsonRepository::GrowFile(const int64_t file_position, const size_t bytes_to_add)
{
    int64_t bytes_to_shift = m_fileSize - file_position;

    ASSERT(bytes_to_add != 0 && bytes_to_shift > 0);

    if( m_buffer.size() < GrowFileBufferSize )
        m_buffer.resize(GrowFileBufferSize);

    // read data from the end of the file and shift it until until reaching the proper file position
    int64_t next_read_position = m_fileSize;
    PostWriteDataAction post_write_data_action = PostWriteDataAction::UpdateFileSize;

    do
    {
        const size_t bytes_to_read = std::min(static_cast<size_t>(bytes_to_shift), m_buffer.size());

        next_read_position -= bytes_to_read;

        SetFilePosition(next_read_position);

        if( fread(m_buffer.data(), 1, bytes_to_read, m_file) != bytes_to_read )
            throw DataRepositoryException::GenericReadError();

        // move forward to write out the block
        SetFilePosition(next_read_position + bytes_to_add);

        WriteData(m_buffer.data(), bytes_to_read, post_write_data_action);
        post_write_data_action = PostWriteDataAction::Nothing;

        bytes_to_shift -= bytes_to_read;

    } while( bytes_to_shift > 0 );

    ASSERT(m_filePosition == PortableFunctions::ftelli64(m_file));
}


void JsonRepository::AddCaseToIndex(const Case& data_case, const int64_t file_position, const size_t bytes_for_case)
{
    ASSERT(m_stmtInsertCase != nullptr);
    const SQLiteResetOnDestruction rod(*m_stmtInsertCase);

    m_stmtInsertCase->Bind(1, data_case.GetUuid())
                     .Bind(2, data_case.GetKey())
                     .Bind(3, data_case.GetCaseLabel())
                     .Bind(4, data_case.GetCaseNote())
                     .Bind(5, data_case.GetDeleted())
                     .Bind(6, data_case.GetVerified())
                     .Bind(7, static_cast<int>(data_case.GetPartialSaveMode()))
                     .Bind(8, file_position)
                     .Bind(9, bytes_for_case);

    if( m_stmtInsertCase->Step() != SQLITE_DONE )
        throw DataRepositoryException::SQLiteError();
}


void JsonRepository::ModifyCaseIndexEntry(const Case& data_case, const int64_t file_position, const size_t bytes_for_case)
{
    EnsureSqlStatementIsPrepared(Sqlite::Commands::ModifyCase, m_stmtModifyCase);
    const SQLiteResetOnDestruction rod(*m_stmtModifyCase);

    m_stmtModifyCase->Bind(1, data_case.GetKey())
                     .Bind(2, data_case.GetCaseLabel())
                     .Bind(3, data_case.GetCaseNote())
                     .Bind(4, data_case.GetDeleted())
                     .Bind(5, data_case.GetVerified())
                     .Bind(6, static_cast<int>(data_case.GetPartialSaveMode()))
                     .Bind(7, file_position)
                     .Bind(8, bytes_for_case)
                     .Bind(9, data_case.GetUuid());

    if( m_stmtModifyCase->Step() != SQLITE_DONE )
        throw DataRepositoryException::SQLiteError();
}


void JsonRepository::ShiftIndexPositions(const int64_t first_file_position_to_shift, const size_t bytes_to_add)
{
    EnsureSqlStatementIsPrepared(Sqlite::Commands::ShiftCasePositions, m_stmtShiftCasePositions);
    const SQLiteResetOnDestruction rod(*m_stmtShiftCasePositions);

    m_stmtShiftCasePositions->Bind(1, bytes_to_add)
                             .Bind(2, first_file_position_to_shift);

    if( m_stmtShiftCasePositions->Step() != SQLITE_DONE )
        throw DataRepositoryException::SQLiteError();
}


void JsonRepository::WriteCase(Case& data_case, WriteCaseParameter* const write_case_parameter/* = nullptr*/)
{
    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    // quickly write out the case and get out (for batch processing)
    if( m_accessType == DataRepositoryAccess::BatchOutput || m_accessType == DataRepositoryAccess::BatchOutputAppend )
    {
        WriteCaseForBatchOutput(data_case);
        return;
    }

    ASSERT(m_file != nullptr);

    // determine where to write the case
    ASSERT(write_case_parameter == nullptr || ( ( write_case_parameter != nullptr ) == ( m_accessType == DataRepositoryAccess::EntryInput ) ));

    int64_t anchor_file_position = -1;
    bool write_before_anchor = false;
    bool use_new_uuid = false;

    if( write_case_parameter != nullptr )
    {
        ASSERT(m_accessType == DataRepositoryAccess::EntryInput);

        anchor_file_position = static_cast<int64_t>(write_case_parameter->GetPositionInRepository());

        if( write_case_parameter->IsInsertParameter() )
            write_before_anchor = true;
    }

    else if( m_accessType == DataRepositoryAccess::ReadWrite )
    {
        // check if a non-deleted case should be overwritten
        EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryPositionUuidByNotDeletedKey, m_stmtQueryPositionUuidByKey);
        const SQLiteResetOnDestruction rod(*m_stmtQueryPositionUuidByKey);

        m_stmtQueryPositionUuidByKey->Bind(1, data_case.GetKey());

        // if so, use the UUID of the existing case
        if( m_stmtQueryPositionUuidByKey->Step() == SQLITE_ROW )
        {
            anchor_file_position = m_stmtQueryPositionUuidByKey->GetColumn<int64_t>(0);
            data_case.SetUuid(m_stmtQueryPositionUuidByKey->GetColumn<std::string>(1));
        }

        // otherwise generate a new one so a loaded case that has its IDs modified will be saved with a unique UUID
        else
        {
            use_new_uuid = true;
        }
    }

    // determine how many bytes the current case takes up (if it is to be replaced)
    std::optional<size_t> bytes_for_case_to_replace;

    if( !write_before_anchor && anchor_file_position != -1 )
        bytes_for_case_to_replace = GetBytesFromPosition(anchor_file_position);

    WriteCaseForEntryInputReadWrite(data_case, anchor_file_position, std::move(bytes_for_case_to_replace), use_new_uuid);
}


void JsonRepository::WriteCaseForEntryInputReadWrite(Case& data_case, const int64_t anchor_file_position, const std::optional<size_t> bytes_for_case_to_replace, const bool use_new_uuid)
{
    ASSERT(m_accessType == DataRepositoryAccess::EntryInput || m_accessType == DataRepositoryAccess::ReadWrite);

    const bool add_case_to_end = ( anchor_file_position == -1 );
    const bool insert_case_above_anchor = ( !add_case_to_end && !bytes_for_case_to_replace.has_value() );

    ASSERT(add_case_to_end || ( anchor_file_position >= 1 && anchor_file_position < ( m_fileSize - std::string_view("]").size() ) ));
    ASSERT(!bytes_for_case_to_replace.has_value() || anchor_file_position >= 1);

    // use a new UUID as necessary
    if( use_new_uuid || data_case.GetUuid().empty() )
        data_case.SetUuid(CreateUuid());

    // convert the case
    const std::string case_json = ConvertCaseToJson(data_case);

    // determine if this case will be the last case in the file
    bool case_is_last_in_file = add_case_to_end;

    if( !add_case_to_end && !insert_case_above_anchor )
    {
        EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryIsLastPosition, m_stmtQueryIsLastPosition);
        const SQLiteResetOnDestruction rod(*m_stmtQueryIsLastPosition);

        m_stmtQueryIsLastPosition->Bind(1, anchor_file_position);

        if( m_stmtQueryIsLastPosition->Step() != SQLITE_ROW )
            case_is_last_in_file = true;
    }

    // if this is not the last (or only) case in the file, a comma needs to be added to the case contents
    std::string post_case_comma_and_spacing = case_is_last_in_file ? std::string() :
                                                                     std::string(JsonArrayCaseSeparatorComma_sv);

    // the case writing routine
    auto write_case = [&](const int64_t write_position)
    {
        SetFilePosition(write_position);

        WriteData(case_json.data(), case_json.size(), PostWriteDataAction::Nothing);

        if( !post_case_comma_and_spacing.empty() )
            WriteData(post_case_comma_and_spacing.data(), post_case_comma_and_spacing.size(), PostWriteDataAction::Nothing);

        // update the case's position
        data_case.SetPositionInRepository(static_cast<double>(write_position));
    };

    // wrap any updates to the index in a transaction
    if( m_useTransactionManager )
        WrapInTransaction();

    // several ways to write the data...
    size_t bytes_needed_to_write_case = case_json.size() + post_case_comma_and_spacing.size();
    ASSERT(bytes_needed_to_write_case > 0);


    // APPROACH 1: insert the case, shifting all cases following this case
    if( insert_case_above_anchor )
    {
        ASSERT(GetNumberCases(CaseIterationCaseStatus::All) != 0);

        GrowFile(anchor_file_position, bytes_needed_to_write_case);

        write_case(anchor_file_position);

        // shift the positions of cases starting at the insertion point
        ShiftIndexPositions(anchor_file_position, bytes_needed_to_write_case);

        // add the case to the index
        AddCaseToIndex(data_case, anchor_file_position, bytes_needed_to_write_case);
    }


    // APPROACH 2: add the case to the end, adding a comma to the previous case (if one exists)
    else if( add_case_to_end )
    {
        EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryLastUuidBytesPosition, m_stmtQueryLastUuidPositionBytes);
        const SQLiteResetOnDestruction query_last_case_rod(*m_stmtQueryLastUuidPositionBytes);

        // if this is the file's first case, completely rewrite the file
        if( m_stmtQueryLastUuidPositionBytes->Step() != SQLITE_ROW )
        {
            SetFilePosition(0);

            WriteData(JsonArrayStart_sv.data(), JsonArrayStart_sv.size(), PostWriteDataAction::Nothing);

            const int64_t write_position = m_filePosition;
            write_case(write_position);

            WriteData(JsonArrayEnd_sv.data(), JsonArrayEnd_sv.size(), PostWriteDataAction::UpdateFileSizeAndTruncate);

            // add the case to the index
            AddCaseToIndex(data_case, write_position, bytes_needed_to_write_case);
        }

        // otherwise read the current last case, add a comma to it, and then write out the new case
        else
        {
            const std::string last_case_uuid = m_stmtQueryLastUuidPositionBytes->GetColumn<std::string>(0);
            const int64_t last_case_file_position = m_stmtQueryLastUuidPositionBytes->GetColumn<int64_t>(1);
            const size_t last_case_bytes_for_case = m_stmtQueryLastUuidPositionBytes->GetColumn<size_t>(2);

            if( m_buffer.size() < last_case_bytes_for_case )
                m_buffer.resize(last_case_bytes_for_case);

            SetFilePosition(last_case_file_position);

            if( fread(m_buffer.data(), 1, last_case_bytes_for_case, m_file) != last_case_bytes_for_case )
                throw DataRepositoryException::GenericReadError();

            // strip any whitespace characters from the last case
            size_t whitespace_chars = 0;

            while( whitespace_chars < last_case_bytes_for_case &&
                   std::isspace(static_cast<unsigned char>(m_buffer[last_case_bytes_for_case - whitespace_chars - 1])) )
            {
                ++whitespace_chars;
            }

            // write the comma to the file
            SetFilePosition(last_case_file_position + last_case_bytes_for_case - whitespace_chars);
            WriteData(JsonArrayCaseSeparatorComma_sv.data(), JsonArrayCaseSeparatorComma_sv.size(), PostWriteDataAction::Nothing);

            // update the last case's size in the index
            EnsureSqlStatementIsPrepared(Sqlite::Commands::ModifyCaseBytes, m_stmtModifyCaseBytes);
            const SQLiteResetOnDestruction modfy_case_bytes_rod(*m_stmtModifyCaseBytes);

            m_stmtModifyCaseBytes->Bind(1, last_case_bytes_for_case - whitespace_chars + JsonArrayCaseSeparatorComma_sv.size())
                                  .Bind(2, last_case_uuid);

            if( m_stmtModifyCaseBytes->Step() != SQLITE_DONE)
                throw DataRepositoryException::SQLiteError();

            // write the new case
            const int64_t write_position = m_filePosition;
            write_case(write_position);

            WriteData(JsonArrayEnd_sv.data(), JsonArrayEnd_sv.size(), PostWriteDataAction::UpdateFileSize);

            // add the case to the index
            AddCaseToIndex(data_case, write_position, bytes_needed_to_write_case);
        }
    }


    // APPROACH 3: replace an existing case at the end of the file
    else if( case_is_last_in_file )
    {
        // write the case
        write_case(anchor_file_position);

        // and then rewrite the end array characters
        WriteData(JsonArrayEnd_sv.data(), JsonArrayEnd_sv.size(), PostWriteDataAction::UpdateFileSizeAndTruncate);

        // modify the case's index entry
        ModifyCaseIndexEntry(data_case, anchor_file_position, bytes_needed_to_write_case);
    }


    // APPROACH 4: replace an existing case, growing the file if the case increased in size, or padding with spaces if it decreased in size
    else
    {
        ASSERT(bytes_for_case_to_replace.has_value());

        const int64_t bytes_differential = static_cast<int64_t>(bytes_needed_to_write_case) - *bytes_for_case_to_replace;

        if( bytes_differential > 0 )
        {
            GrowFile(anchor_file_position + *bytes_for_case_to_replace, static_cast<size_t>(bytes_differential));

            // shift the positions of cases following this case
            ShiftIndexPositions(anchor_file_position + 1, static_cast<size_t>(bytes_differential));
        }

        else if( bytes_differential != 0 )
        {
            const size_t padding = static_cast<size_t>(-1 * bytes_differential);

            // add spacing after the comma (if one is going to be written)
            post_case_comma_and_spacing.insert(post_case_comma_and_spacing.empty() ? 0 : ( post_case_comma_and_spacing.size() - 1 ),
                                               padding, ' ');

            bytes_needed_to_write_case += padding;
        }

        write_case(anchor_file_position);

        // modify the case's index entry
        ModifyCaseIndexEntry(data_case, anchor_file_position, bytes_needed_to_write_case);
    }

    // flush the file to make sure that any changes take effect
    fflush(m_file);

#ifdef _DEBUG
    ASSERT(m_filePosition == PortableFunctions::ftelli64(m_file));
    PortableFunctions::fseeki64(m_file, 0, SEEK_END);
    ASSERT(m_fileSize == PortableFunctions::ftelli64(m_file));
    PortableFunctions::fseeki64(m_file, m_filePosition, SEEK_SET);
#endif
}


void JsonRepository::WriteCaseForBatchOutput(Case& data_case)
{
    ASSERT(m_accessType == DataRepositoryAccess::BatchOutput || m_accessType == DataRepositoryAccess::BatchOutputAppend);

    // preserve the UUID when possible but set one if one does not exist
    data_case.GetOrCreateUuid();

    // convert the case
    const std::string case_json = ConvertCaseToJson(data_case);

    // write the case
    ASSERT(m_endArrayOnFileClose && m_filePosition == PortableFunctions::ftelli64(m_file));

    if( m_writeCommaBeforeNextCase )
        WriteData(JsonArrayCaseSeparatorComma_sv.data(), JsonArrayCaseSeparatorComma_sv.size(), PostWriteDataAction::UpdateFileSize);

    data_case.SetPositionInRepository(static_cast<double>(m_fileSize));

    WriteData(case_json.data(), case_json.size(), PostWriteDataAction::UpdateFileSize);

    m_writeCommaBeforeNextCase = true;
}


void JsonRepository::DeleteCase(const int64_t file_position, const size_t bytes_for_case, const bool deleted, const std::string* /*key_if_known*/)
{
    std::shared_ptr<Case> data_case = GetTemporaryCase();

    ReadCase(*data_case, file_position, bytes_for_case);

    // only modify the case when the deleted status has changed
    if( data_case->GetDeleted() != deleted )
    {
        data_case->SetDeleted(deleted);
        WriteCaseForEntryInputReadWrite(*data_case, file_position, bytes_for_case, false);
    }
}


void JsonRepository::WrapInTransaction()
{
    ASSERT(m_useTransactionManager);

    if( m_numberTransactions == IndexableTextRepository::MaxNumberSqlInsertsInOneTransaction )
        CommitTransactions();

    if( m_numberTransactions > 0 || sqlite3_exec(m_db, Sqlite::Commands::BeginTransaction, nullptr, nullptr, nullptr) == SQLITE_OK )
        ++m_numberTransactions;
}


bool JsonRepository::CommitTransactions()
{
    ASSERT(m_useTransactionManager);

    if( m_numberTransactions > 0 )
    {
        if( sqlite3_exec(m_db, Sqlite::Commands::EndTransaction, nullptr, nullptr, nullptr) != SQLITE_OK )
            throw DataRepositoryException::SQLiteError();

        m_numberTransactions = 0;
    }

    // errors will be thrown
    return true;
}


size_t JsonRepository::GetNumberCases(const CaseIterationCaseStatus case_status, const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    if( case_status == CaseIterationCaseStatus::NotDeletedOnly && start_parameters == nullptr )
        return IndexableTextRepository::GetNumberCases();

    const CaseIteratorSettings iterator_settings(case_status, std::nullopt, std::nullopt, start_parameters);

    SQLiteStatement stmt_query_keys = CreateKeySearchIteratorStatement("COUNT(*)", iterator_settings, 0, SIZE_MAX);

    if( stmt_query_keys.Step() != SQLITE_ROW )
        throw DataRepositoryException::SQLiteError();

    return stmt_query_keys.GetColumn<size_t>(0);
}


std::unique_ptr<CaseIterator> JsonRepository::CreateIterator(const CaseIterationContent iteration_content,
                                                             const CaseIteratorSettings& iterator_settings,
                                                             const size_t offset/* = 0*/, const size_t limit/* = SIZE_MAX*/)
{
    // use a fast batch iterator when possible
    if( m_jsonStream != nullptr &&
        iteration_content == CaseIterationContent::Case &&
        iterator_settings.GetStatus() != CaseIterationCaseStatus::DuplicatesOnly &&
        iterator_settings.GetMethod() == CaseIterationMethod::SequentialOrder &&
        iterator_settings.GetOrder() == CaseIterationOrder::Ascending &&
        iterator_settings.GetParameters() == nullptr &&
        offset == 0 &&
        limit == SIZE_MAX )
    {
        return CreateBatchIterator(iterator_settings.GetStatus());
    }

    else
    {
        const char* const columns_to_query = ( iteration_content == CaseIterationContent::CaseKey )     ? "`position`, `bytes`, `key`" :
                                             ( iteration_content == CaseIterationContent::CaseSummary ) ? "`position`, `bytes`, `key`, `label`, `note`, `deleted`, `verified`, `partial`" :
                                           /*( iteration_content == CaseIterationContent::Case ) */       "`position`, `bytes`";

        SQLiteStatement stmt_query_keys = CreateKeySearchIteratorStatement(columns_to_query, iterator_settings, offset, limit);

        return std::make_unique<JsonRepositoryCaseIterator>(
            *this,
            iteration_content,
            std::move(stmt_query_keys),
            iterator_settings.GetStatus(),
            iterator_settings.GetParameters()
        );
    }
}


std::unique_ptr<CaseIterator> JsonRepository::CreateBatchIterator(const CaseIterationCaseStatus case_status)
{
    try
    {
        // if an object array iterator has already been started, move to the beginning of the stream
        if( m_jsonStreamObjectArrayIterator != nullptr )
            m_jsonStream->RestartStream();

        m_jsonStreamObjectArrayIterator = std::make_unique<JsonStreamObjectArrayIterator>(m_jsonStream->CreateObjectArrayIterator());

        return std::make_unique<JsonRepositoryBatchCaseIterator>(*this, case_status);
    }

    catch( const JsonParseException& exception )
    {
        RethrowJsonParseException(exception);
    }
}


std::string JsonRepository::ConvertCaseToJson(const Case& data_case)
{
    ASSERT(!data_case.GetUuid().empty());

    // create the JSON serializer helper
    if( m_caseJsonWriterSerializerHelper == nullptr )
    {
        m_caseJsonWriterSerializerHelper = std::make_shared<CaseJsonWriterSerializerHelper>();

        m_jsonFormatCompact = m_connectionString.HasProperty(CSProperty::jsonFormat, CSValue::compact);

        if( m_connectionString.HasProperty(CSProperty::verbose, CSValue::true_, true) )
            m_caseJsonWriterSerializerHelper->SetVerbose();

        if( m_connectionString.HasProperty(CSProperty::writeBlankValues, CSValue::true_, true) )
            m_caseJsonWriterSerializerHelper->SetWriteBlankValues();

        if( m_connectionString.HasProperty(CSProperty::writeLabels, CSValue::true_, true) )
            m_caseJsonWriterSerializerHelper->SetWriteLabels();

        if( std::holds_alternative<std::string>(m_binaryDataOutput) )
        {
            m_caseJsonWriterSerializerHelper->SetBinaryDataWriter(
                [&](JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
                {
                    JsonRepositoryBinaryDataIO::WriteBinaryData(*this, json_writer, binary_case_item, index);
                });
        }
    }

    // generate the JSON for the case
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(m_jsonFormatCompact ? JsonFormattingOptions::Compact :
                                                                                                         DefaultJsonFileWriterFormattingOptions);

    const auto case_json_writer_serializer_holder = json_writer->GetSerializerHelper().Register(m_caseJsonWriterSerializerHelper);

    json_writer->Write(data_case);

    return json_writer->ReleaseString();
}


void JsonRepository::ConvertJsonToCase(Case& data_case, const JsonNode& json_node)
{
    ASSERT(m_caseJsonParserHelper != nullptr);
    m_caseJsonParserHelper->ParseJson(data_case, json_node);
}



// --------------------------------------------------------------------------
// JsonRepositoryCaseJsonParserHelper +
// JsonRepositoryBinaryDataIO
// --------------------------------------------------------------------------

JsonRepositoryCaseJsonParserHelper::JsonRepositoryCaseJsonParserHelper(JsonRepository& json_repository)
    :   CaseJsonParserHelper(json_repository.GetSharedCaseAccess()),
        m_jsonRepository(json_repository)
{
}


std::unique_ptr<BinaryContentReader> JsonRepositoryCaseJsonParserHelper::CreateBinaryContentReader(std::optional<uint64_t> /*size*/)
{
    return std::make_unique<JsonRepositoryBinaryDataIO>(m_jsonRepository);
}


JsonRepositoryBinaryDataIO::JsonRepositoryBinaryDataIO(JsonRepository& json_repository)
    :   m_jsonRepository(json_repository)
{
}


const UniqueId* JsonRepositoryBinaryDataIO::GetUniqueId() const
{
    return &m_jsonRepository.m_repositoryId;
}


const std::string& JsonRepositoryBinaryDataIO::EvaluateFilePath(const std::string& signature)
{
    if( m_evaluatedFilePath.empty() )
    {
        if( signature.empty() )
            throw DataRepositoryException::IOError(ErrorText::BinaryDataNoSignature);

        const std::string directory =
            std::holds_alternative<std::string>(m_jsonRepository.m_binaryDataOutput) ? std::get<std::string>(m_jsonRepository.m_binaryDataOutput) :
                                                                                       JsonRepository::GetDefaultBinaryDataDirectory(m_jsonRepository.GetConnectionString());

        if( !directory.empty() )
        {
            // first check if the file exists without an extension
            std::string base_file_path = Path::Combine(directory, signature);

            if( PortableFunctions::FileIsRegular(base_file_path) )
            {
                m_evaluatedFilePath = std::move(base_file_path);
            }

            // if not, see if a single file exists with the signature and any extension
            else
            {
                std::vector<std::string> file_paths = DirectoryLister().SetNameFilter(signature + ".*")
                                                                       .GetPaths(directory);

                if( file_paths.size() == 1 )
                {
                    m_evaluatedFilePath = std::move(file_paths.front());
                }

                else if( !file_paths.empty() )
                {
                    throw DataRepositoryException::IOError(ErrorText::BinaryDataFileTooManyFilesFormatter,
                                                           signature.c_str(), directory.c_str());
                }
            }
        }

        if( m_evaluatedFilePath.empty() )
        {
            throw DataRepositoryException::IOError(ErrorText::BinaryDataFileNotFoundFormatter,
                                                   signature.c_str());
        }
    }

    return m_evaluatedFilePath;
}


uint64_t JsonRepositoryBinaryDataIO::GetSize(const std::string& signature)
{
    const std::string& file_path = EvaluateFilePath(signature);
    const int64_t size = PortableFunctions::FileSize(file_path);

    if( size < 0 )
    {
        ASSERT(false);
        throw DataRepositoryException::GenericReadError();
    }

    return static_cast<uint64_t>(size);
}


BinaryContentCacher::CacheableContent JsonRepositoryBinaryDataIO::GetContentWorker(const std::string& signature)
{
    try
    {
        return *FileIO::Read(EvaluateFilePath(signature));
    }

    catch( const CSProException& exception )
    {
        throw DataRepositoryException::IOError(exception.what());
    }
}


void JsonRepositoryBinaryDataIO::WriteBinaryData(JsonRepository& json_repository, JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
{
    ASSERT(std::holds_alternative<std::string>(json_repository.m_binaryDataOutput));
    const std::string& binary_data_directory = std::get<std::string>(json_repository.m_binaryDataOutput);

    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);

    if( !binary_data_accessor.IsDefined() )
        return;

    // if the data came from this repository, we don't need to write it out again
    const JsonRepositoryBinaryDataIO* const json_repository_binary_data_io = dynamic_cast<const JsonRepositoryBinaryDataIO*>(binary_data_accessor.GetBinaryContentReader());

    if( json_repository_binary_data_io == nullptr || json_repository_binary_data_io->GetUniqueId() != &json_repository.m_repositoryId )
    {
        // otherwise save the file using a filename based on the file's (MD5) signature
        const BinaryData& binary_data = binary_data_accessor.GetBinaryData();
        ASSERT(binary_data_accessor.GetSignature() == PortableFunctions::BinaryMd5(binary_data.GetContent()));

        // only save the file if it does not already exist (with any extension)
        if( DirectoryLister().SetNameFilter(binary_data_accessor.GetSignature() + ".*").GetPaths(binary_data_directory).empty() )
        {
            std::string file_path = MakeFullPath(binary_data_directory, binary_data_accessor.GetSignature());

            // add the file extension when possible
            const std::optional<std::string> extension = binary_data.GetMetadata().GetEvaluatedExtension();

            if( extension.has_value() )
                file_path = PortableFunctions::PathAppendFileExtension(file_path, *extension);

            try
            {
                FileIO::Write(file_path, binary_data.GetContent());
            }

            catch( const CSProException& exception )
            {
                throw DataRepositoryException::IOError(exception.what());
            }
        }
    }

    // write the signature to the case JSON
    json_writer.Write(JK::signature, binary_data_accessor.GetSignature());
}



// --------------------------------------------------------------------------
// JsonRepositoryIndexCreator
// --------------------------------------------------------------------------

JsonRepositoryIndexCreator::JsonRepositoryIndexCreator(JsonRepository& json_repository)
    :   m_jsonRepository(json_repository),
        m_fileBytesRemaining(m_jsonRepository.m_fileSize),
        throwExceptionsOnDuplicateKeys(true),
        m_bufferStartFilePosition(0),
        m_nextReadUnprocessedDataPosition(0),
        m_endOfFileReached(false)
{
    // the data file should come in open with the position at the end of the file
    ASSERT(m_jsonRepository.m_file != nullptr && m_jsonRepository.m_jsonStream == nullptr);
    ASSERT(m_jsonRepository.m_filePosition == m_jsonRepository.m_fileSize);
}


void JsonRepositoryIndexCreator::Initialize(const bool throw_exceptions_on_duplicate_keys)
{
    throwExceptionsOnDuplicateKeys = throw_exceptions_on_duplicate_keys;

    // move to beginning of the file
    PortableFunctions::fseeki64(m_jsonRepository.m_file, 0, SEEK_SET);
    m_jsonRepository.m_filePosition = 0;

    if( FillBuffer() )
    {
        // skip past the BOM if necessary
        const TextEncoding text_encoding(m_buffer.data(), m_buffer.size());
        ASSERT(text_encoding.IsUtf8());
        m_nextReadUnprocessedDataPosition = text_encoding.GetBomLength();

        // read the until the beginning of the JSON array
        if( ReadUntilCharacter("[") != '[' )
            throw DataRepositoryException::IOError(ErrorText::JsonInvalidStart);

        // read until the start of an object, or the end of the JSON array
        ++m_nextReadUnprocessedDataPosition;
        const unsigned char ch = ReadUntilCharacter("{]");

        if( ch == '{' )
        {
            m_case = m_jsonRepository.GetCaseAccess().CreateCase();
            return;
        }

        else if( ch == ']' )
        {
            EnsureAtEndOfFile();
            return;
        }
    }

    throw DataRepositoryException::IOError(ErrorText::JsonCharacterNotFoundFormatter,
                                           "'{'", static_cast<int>(GetUnprocessedDataPosition()) + 1);
}


const char* JsonRepositoryIndexCreator::GetCreateKeyTableSql() const
{
    return Sqlite::Commands::CreateKeyTable;
}


const std::vector<const char*>& JsonRepositoryIndexCreator::GetCreateIndexSqlStatements() const
{
    static const std::vector<const char*> CreateIndexSqlStatements = { Sqlite::Commands::CreateKeyTableKeyIndex,
                                                                       Sqlite::Commands::CreateKeyTablePositionIndex };
    return CreateIndexSqlStatements;
}


int64_t JsonRepositoryIndexCreator::GetFileSize() const
{
    return m_jsonRepository.m_fileSize;
}


int JsonRepositoryIndexCreator::GetPercentRead() const
{
    return CreatePercent(m_jsonRepository.m_filePosition, m_jsonRepository.m_fileSize);
}


bool JsonRepositoryIndexCreator::FillBuffer()
{
    const size_t bytes_from_previous_read = m_buffer.size() - m_nextReadUnprocessedDataPosition;

    // move unprocessed data to the beginning of the buffer
    if( bytes_from_previous_read != 0 )
        memmove(m_buffer.data(), m_buffer.data() + m_nextReadUnprocessedDataPosition, bytes_from_previous_read);

    m_buffer.resize(bytes_from_previous_read);

    const size_t bytes_to_read = static_cast<size_t>(std::min(m_fileBytesRemaining, static_cast<int64_t>(IndexCreatorFillBufferSize)));

    // reserve a decent amount of space in the buffer to avoid many reallocations
    if( bytes_to_read > m_buffer.capacity() )
        m_buffer.reserve(bytes_to_read * 2);

    m_buffer.resize(bytes_from_previous_read + bytes_to_read);

    if( fread(m_buffer.data() + bytes_from_previous_read , 1, bytes_to_read, m_jsonRepository.m_file) != bytes_to_read )
        throw DataRepositoryException::GenericReadError();

    m_fileBytesRemaining -= bytes_to_read;
    m_jsonRepository.m_filePosition += bytes_to_read;
    m_bufferStartFilePosition += m_nextReadUnprocessedDataPosition;
    m_nextReadUnprocessedDataPosition = 0;

    ASSERT(PortableFunctions::ftelli64(m_jsonRepository.m_file) == ( m_bufferStartFilePosition + m_buffer.size() ));

    return ( bytes_to_read > 0 );
}


unsigned char JsonRepositoryIndexCreator::ReadUntilCharacter(const char* const allowable_characters)
{
    do
    {
        for( ; m_nextReadUnprocessedDataPosition < m_buffer.size(); ++m_nextReadUnprocessedDataPosition )
        {
            const unsigned char ch = m_buffer[m_nextReadUnprocessedDataPosition];

            if( !std::isspace(ch) )
            {
                const char* const char_pos = strchr(allowable_characters, ch);
                return ( char_pos != nullptr ) ? *char_pos : 0;
            }
        }

    } while( FillBuffer() );

    return EndOfFile;
}


std::tuple<std::string, size_t> JsonRepositoryIndexCreator::ReadObject()
{
    ASSERT(m_buffer[m_nextReadUnprocessedDataPosition] == '{');
    size_t buffer_pos = m_nextReadUnprocessedDataPosition + 1;

    size_t in_object_count = 1;
    bool in_string = false;
    bool last_char_was_escape = false;

    while( true )
    {
        for( ; buffer_pos < m_buffer.size(); ++buffer_pos )
        {
            const unsigned char ch = m_buffer[buffer_pos];

            if( ch == '"' )
            {
                if( !in_string )
                {
                    in_string = true;
                    ASSERT(!last_char_was_escape);
                }

                else if( !last_char_was_escape )
                {
                    in_string = false;
                }

                else
                {
                    last_char_was_escape = false;
                }
            }

            else if( in_string )
            {
                last_char_was_escape = ( ch == '\\' && !last_char_was_escape );
            }

            else if( ch == '}' )
            {
                if( --in_object_count == 0 )
                {
                    const size_t case_chars = buffer_pos - m_nextReadUnprocessedDataPosition + 1;

                    const std::string_view case_json_sv(m_buffer.data() + m_nextReadUnprocessedDataPosition, case_chars);

                    m_nextReadUnprocessedDataPosition += case_chars;

                    return { std::string(case_json_sv), case_chars };
                }
            }

            else if( ch == '{' )
            {
                ++in_object_count;
            }
        }

        // fill more of the buffer (with FillBuffer moving the data currently being processed to the beginning of the buffer)
        buffer_pos -= m_nextReadUnprocessedDataPosition;

        if( !FillBuffer() )
        {
            throw DataRepositoryException::IOError(ErrorText::JsonCharacterNotFoundFormatter,
                                                   "'}'", static_cast<int>(GetUnprocessedDataPosition()) + 1);
        }

        ASSERT(m_nextReadUnprocessedDataPosition == 0);
    }
}


void JsonRepositoryIndexCreator::EnsureAtEndOfFile()
{
    ASSERT(m_buffer[m_nextReadUnprocessedDataPosition] == ']');
    ++m_nextReadUnprocessedDataPosition;

    if( ReadUntilCharacter("") != EndOfFile )
    {
        throw DataRepositoryException::IOError(ErrorText::JsonCharacterNotFoundFormatter,
                                               "the end of the file", static_cast<int>(GetUnprocessedDataPosition()) + 1);
    }

    m_endOfFileReached = true;
}


bool JsonRepositoryIndexCreator::ReadCaseAndUpdateIndex(IndexableTextRepositoryIndexDetails& index_details)
{
    if( m_endOfFileReached )
        return false;

    const int64_t case_start_pos = GetUnprocessedDataPosition();
    const auto [case_json, utf8_case_json_chars] = ReadObject();

    // parse the case
    try
    {
        ASSERT(m_case != nullptr);
        m_jsonRepository.ConvertJsonToCase(*m_case, Json::Parse(case_json));
    }

    catch( const JsonParseException& exception )
    {
        throw DataRepositoryException::IOError(ErrorText::JsonCaseParseExceptionFormatter,
                                               static_cast<int>(GetUnprocessedDataPosition()) + 1, exception.what());
    }

    // read until the next case, including any whitespace and the array element separator (,) as part of the bytes of this case
    const int64_t whitespace_start_pos = GetUnprocessedDataPosition();
    size_t array_separator_and_whitespace_chars = 0;

    unsigned char ch = ReadUntilCharacter(",]");

    if( ch == ']' )
    {
        EnsureAtEndOfFile();
    }

    else
    {
        if( ch == ',' )
        {
            ++m_nextReadUnprocessedDataPosition;
            ch = ReadUntilCharacter("{");
        }

        if( ch != '{' )
        {
            throw DataRepositoryException::IOError(ErrorText::JsonCharacterNotFoundFormatter,
                                                   "'{'", static_cast<int>(GetUnprocessedDataPosition()) + 1);
        }

        array_separator_and_whitespace_chars = static_cast<size_t>(GetUnprocessedDataPosition() - whitespace_start_pos);
    }

    // update the index
    index_details.key = m_case->GetKey();
    index_details.position = case_start_pos;
    index_details.bytes = utf8_case_json_chars + array_separator_and_whitespace_chars;
    UpdateIndex(index_details);

    return true;
}


void JsonRepositoryIndexCreator::UpdateIndex(IndexableTextRepositoryIndexDetails& index_details)
{
    // check if the UUID already exists in the index
    ASSERT(!m_case->GetUuid().empty());

    ASSERT(m_jsonRepository.m_stmtUuidExists != nullptr);
    const SQLiteResetOnDestruction uuid_exists_rod(*m_jsonRepository.m_stmtUuidExists);

    m_jsonRepository.m_stmtUuidExists->Bind(1, m_case->GetUuid());

    index_details.case_prevents_index_creation = ( m_jsonRepository.m_stmtUuidExists->Step() == SQLITE_ROW );

    if( index_details.case_prevents_index_creation )
    {
        if( throwExceptionsOnDuplicateKeys )
        {
            throw DataRepositoryException::DuplicateCaseWhileCreatingIndex(
                FormatText("An index could not be created for a data file with duplicate case UUIDs, including: '%s'", m_case->GetUuid().c_str()));
        }
    }

    else
    {
        m_jsonRepository.AddCaseToIndex(*m_case, index_details.position, index_details.bytes);
    }
}


void JsonRepositoryIndexCreator::OnSuccessfulCreation()
{
    ASSERT(m_jsonRepository.m_filePosition == PortableFunctions::ftelli64(m_jsonRepository.m_file));
}



// --------------------------------------------------------------------------
// JsonRepositoryCaseIterator
// --------------------------------------------------------------------------

JsonRepositoryCaseIterator::JsonRepositoryCaseIterator(JsonRepository& json_repository, const CaseIterationContent iteration_content, SQLiteStatement stmt_query_keys,
                                                       const CaseIterationCaseStatus case_status, const CaseIteratorParameters* const start_parameters)
    :   m_jsonRepository(json_repository),
        m_iterationContent(iteration_content),
        m_stmtQueryKeys(std::move(stmt_query_keys)),
        m_progressBarParameters(case_status, CreateCopyOfPointerValue(start_parameters)),
        m_casesRead(0)
{
}


bool JsonRepositoryCaseIterator::Step()
{
    if( m_stmtQueryKeys.Step() == SQLITE_ROW )
    {
        ++m_casesRead;
        return true;
    }

    return false;
}


template<typename T>
bool JsonRepositoryCaseIterator::NextCaseForNonCaseReading(T& case_object)
{
    // in the rare event that the CaseKey or CaseSummary is being queried from
    // a case iterator, use a case that can be used to access the case contents
    std::shared_ptr<Case> data_case = m_jsonRepository.GetTemporaryCase();

    if( NextCase(*data_case) )
    {
        case_object = *data_case;
        return true;
    }

    return false;
}


bool JsonRepositoryCaseIterator::NextCaseKey(CaseKey& case_key)
{
    if( m_iterationContent == CaseIterationContent::Case )
    {
        return NextCaseForNonCaseReading(case_key);
    }

    else if( !Step() )
    {
        return false;
    }

    case_key.SetKey(m_stmtQueryKeys.GetColumn<std::string>(2));
    case_key.SetPositionInRepository(m_stmtQueryKeys.GetColumn<double>(0));

    return true;
}


bool JsonRepositoryCaseIterator::NextCaseSummary(CaseSummary& case_summary)
{
    if( m_iterationContent != CaseIterationContent::CaseSummary )
    {
        return NextCaseForNonCaseReading(case_summary);
    }

    else if( !NextCaseKey(case_summary) )
    {
        return false;
    }

    case_summary.SetCaseLabel(m_stmtQueryKeys.GetColumn<std::string>(3));
    case_summary.SetCaseNote(m_stmtQueryKeys.GetColumn<std::string>(4));
    case_summary.SetDeleted(m_stmtQueryKeys.GetColumn<bool>(5));
    case_summary.SetVerified(m_stmtQueryKeys.GetColumn<bool>(6));
    case_summary.SetPartialSaveMode(static_cast<PartialSaveMode>(m_stmtQueryKeys.GetColumn<int>(7)));

    return true;
}


bool JsonRepositoryCaseIterator::NextCase(Case& data_case)
{
    if( !Step() )
        return false;

    const int64_t file_position = m_stmtQueryKeys.GetColumn<int64_t>(0);
    const size_t bytes_for_case = m_stmtQueryKeys.GetColumn<size_t>(1);

    m_jsonRepository.ReadCase(data_case, file_position, bytes_for_case);

    return true;
}


int JsonRepositoryCaseIterator::GetPercentRead() const
{
    // get the number of cases if necessary
    if( !m_percentMultiplier.has_value() )
    {
        const size_t number_cases = m_jsonRepository.GetNumberCases(std::get<0>(m_progressBarParameters), std::get<1>(m_progressBarParameters).get());
        m_percentMultiplier = CreatePercentMultiplier(number_cases);
    }

    return static_cast<int>(m_casesRead * *m_percentMultiplier);
}



// --------------------------------------------------------------------------
// JsonRepositoryBatchCaseIterator
// --------------------------------------------------------------------------

JsonRepositoryBatchCaseIterator::JsonRepositoryBatchCaseIterator(JsonRepository& json_repository, const CaseIterationCaseStatus case_status)
    :   m_jsonRepository(json_repository),
        m_caseStatus(case_status)
{
    ASSERT(m_jsonRepository.m_jsonStreamObjectArrayIterator != nullptr);
    ASSERT(m_caseStatus != CaseIterationCaseStatus::DuplicatesOnly);
}


template<typename T>
bool JsonRepositoryBatchCaseIterator::NextCaseForNonCaseReading(T& case_object)
{
    // in the rare event that the CaseKey or CaseSummary is being queried from
    // a batch iterator, use a case that can be used to access the case contents
    std::shared_ptr<Case> data_case = m_jsonRepository.GetTemporaryCase();

    if( NextCase(*data_case) )
    {
        case_object = *data_case;
        return true;
    }

    return false;
}


bool JsonRepositoryBatchCaseIterator::NextCaseKey(CaseKey& case_key)
{
    return NextCaseForNonCaseReading(case_key);
}


bool JsonRepositoryBatchCaseIterator::NextCaseSummary(CaseSummary& case_summary)
{
    return NextCaseForNonCaseReading(case_summary);
}


bool JsonRepositoryBatchCaseIterator::NextCase(Case& data_case)
{
    try
    {
        while( true )
        {
            const std::optional<JsonNode> json_node = m_jsonRepository.m_jsonStreamObjectArrayIterator->Next();

            if( !json_node.has_value() )
            {
                if( !m_jsonRepository.m_jsonStreamObjectArrayIterator->AtEndOfStream() )
                    throw DataRepositoryException::IOError(ErrorText::JsonInvalidEnd);

                return false;
            }

            m_jsonRepository.ConvertJsonToCase(data_case, *json_node);

            // make sure the case status filter applies
            bool valid;

            if( m_caseStatus == CaseIterationCaseStatus::All )
            {
                valid = true;
            }

            else
            {
                valid = !data_case.GetDeleted();

                if( valid && m_caseStatus == CaseIterationCaseStatus::PartialsOnly )
                    valid = data_case.IsPartial();
            }

            if( valid )
                return true;
        }
    }

    catch( const JsonParseException& exception )
    {
        RethrowJsonParseException(exception);
    }
}


int JsonRepositoryBatchCaseIterator::GetPercentRead() const
{
    return m_jsonRepository.m_jsonStreamObjectArrayIterator->GetPercentRead();
}
