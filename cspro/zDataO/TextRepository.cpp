#include "stdafx.h"
#include "TextRepository.h"
#include "NullRepositoryIterators.h"
#include "TextRepositoryIndexCreator.h"
#include "TextRepositoryIterators.h"
#include "TextRepositoryNotesFile.h"
#include "TextRepositoryStatusFile.h"
#include <zSql/Commands.h>
#include <zSql/SQLiteHelpers.h>
#include <zUtilO/StdioFileUnicode.h>


namespace Constants
{
    constexpr size_t TextBufferSize    = 128 * 1024;
    constexpr const char* TruncationIOError = "There was an error truncating the file.";
}


namespace Sqlite::Commands
{
    // the keys table was initially set with a UNIQUE constraint on Position but that led to problems when executing ShiftKeys
    // because the order of the updates could not be controlled, leading to SQLITE_CONSTRAINT errors

    constexpr const char* CreateKeyTable = "CREATE TABLE `Keys` (`Key` TEXT PRIMARY KEY UNIQUE NOT NULL, `Position` INTEGER NOT NULL, `Bytes` INTEGER NOT NULL) WITHOUT ROWID;";
    constexpr const char* CreateKeyTablePositionIndex = "CREATE INDEX `KeysPositionIndex` ON `Keys` ( `Position` );";
    constexpr const char* InsertKey = "INSERT INTO `Keys` (`Key`, `Position`, `Bytes`) VALUES( ?, ?, ? );";
    constexpr const char* KeyExists = "SELECT 1 FROM `Keys` WHERE `Key` = ? LIMIT 1;";
    constexpr const char* QueryPositionBytesByKey = "SELECT `Position`, `Bytes` FROM `Keys` WHERE `Key` = ? LIMIT 1;";
    constexpr const char* QueryKeyByPosition = "SELECT `Key` FROM `Keys` WHERE `Position` = ? LIMIT 1;";
    constexpr const char* QueryBytesByPosition = "SELECT `Bytes` FROM `Keys` WHERE `Position` = ? LIMIT 1;";
    constexpr const char* QueryIsLastPosition = "SELECT 1 FROM `Keys` WHERE `Position` > ? LIMIT 1;";
    constexpr const char* QueryPreviousKeyByPosition = "SELECT `Key`, `Position`, `Bytes` FROM `Keys` WHERE `Position` < ? ORDER BY `Position` DESC LIMIT 1;";
    constexpr const char* QueryNextKeyByPosition = "SELECT `Key`, `Position`, `Bytes` FROM `Keys` WHERE `Position` > ? ORDER BY `Position` LIMIT 1;";
    constexpr const char* DeleteKeyByPosition = "DELETE FROM `Keys` WHERE `Position` = ?";
    constexpr const char* ModifyKey = "UPDATE `Keys` SET `Key` = ?, `Position` = ?, `Bytes` = ? WHERE `Key` = ?;";
    constexpr const char* ShiftKeys = "UPDATE `Keys` SET `Position` = `Position` + ? WHERE `Position` >= ?;";
    constexpr const char* CountKeys = "SELECT COUNT(*) FROM `Keys`;";
}



// --------------------------------------------------------------------------
// TextRepository
// --------------------------------------------------------------------------

TextRepository::TextRepository(std::shared_ptr<const CaseAccess> case_access, const DataRepositoryAccess access_type)
    :   IndexableTextRepository(DataRepositoryType::Text, std::move(case_access), access_type),
        m_encoding(Encoding::Utf8),
        m_file(nullptr),
        m_fileSize(0),
        m_utf8AnsiBufferSize(Constants::TextBufferSize),
        m_utf8AnsiBuffer(nullptr),
        m_wideBufferSize(Constants::TextBufferSize),
        m_wideBuffer(nullptr),
        m_wideBufferLineCaseStartLineIndex(0),
        m_fileBytesRemaining(0),
        m_utf8AnsiBufferPosition(nullptr),
        m_utf8AnsiBufferEnd(nullptr),
        m_utf8AnsiBufferActualEnd(nullptr),
        m_wideBufferPosition(nullptr),
        m_wideBufferEnd(nullptr),
        m_ignoreNextCharacterIfNewline(false),
        m_deleteCaseShouldDeleteNotesAndStatuses(true),
        m_useTransactionManager(false),
        m_numberTransactions(0)
{
    ModifyCaseAccess(m_caseAccess);

    m_wideBufferLines.reserve(TextToCaseConverter::BufferLinesInitialCapacity);
}


TextRepository::~TextRepository()
{
    try
    {
        Close();
    }

    catch( const DataRepositoryException::Error& )
    {
        // ignore errors
    }

    delete[] m_utf8AnsiBuffer;
    delete[] m_wideBuffer;
}


void TextRepository::ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access)
{
    m_caseAccess = std::move(case_access);
    m_textToCaseConverter = std::make_unique<TextToCaseConverter>(m_caseAccess->GetCaseMetadata());
    m_keyMetadata = &m_textToCaseConverter->GetKeyMetadata();

    const TextToCaseConverter::TextSpan& last_key_span = m_keyMetadata->key_spans.back();
    m_keyEnd = last_key_span.start + last_key_span.length;

    // warn if some case items are used that the text repository does not support
    if( !IsReadOnly() )
    {
        m_caseAccess->IssueWarningIfUsingUnsupportedCaseItems(
            {
                CaseItem::Type::FixedWidthString,
                CaseItem::Type::FixedWidthNumeric,
                CaseItem::Type::FixedWidthNumericWithStringBuffer
            });
    }
}


void TextRepository::Open(const DataRepositoryOpenFlag open_flag)
{
    const bool can_create_file = ( open_flag == DataRepositoryOpenFlag::CreateNew || open_flag == DataRepositoryOpenFlag::OpenOrCreate );

    if( can_create_file && !PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(m_connectionString.GetFilePath())) )
    {
        throw DataRepositoryException::IOError("The directory does not exist and could not be created: %s",
                                                PortableFunctions::PathGetDirectory(m_connectionString.GetFilePath()).c_str());
    }

    const bool file_exists = PortableFunctions::FileIsRegular(m_connectionString.GetFilePath());

    if( open_flag == DataRepositoryOpenFlag::OpenMustExist && !file_exists )
    {
        throw DataRepositoryException::IOError("The data file does not exist: %s", m_connectionString.GetFilePath().c_str());
    }

    // create a data file if one doesn't exist, if setfile/open used the clear flag,
    // or if opening in batch output mode, to overwrite what is already on the disk
    const bool create_new_file = ( !file_exists || open_flag == DataRepositoryOpenFlag::CreateNew || m_accessType == DataRepositoryAccess::BatchOutput );

    if( create_new_file )
    {
        DeleteRepositoryFiles(m_connectionString);

        FILE* file = PortableFunctions::FileOpen(m_connectionString.GetFilePath(), "wb");
        bool success = ( file != nullptr );

        if( success )
        {
            // write out the UTF-8 BOM
            success = ( fwrite(TextEncoding::Utf8Bom_sv.data(), 1, TextEncoding::Utf8Bom_sv.length(), file) == TextEncoding::Utf8Bom_sv.length() );
            fclose(file);
        }

        if( !success )
            throw DataRepositoryException::IOError("Could not create a new data file.");
    }


    // check the encoding
    if( !GetFileBOM(m_connectionString.GetFilePath(), m_encoding) )
        throw DataRepositoryException::IOError("Could not read the data file's encoding. The file may be open in another program.");

    // if the data file is not UTF-8 but it is writeable, then we need to rewrite it as a UTF-8 file
    if( m_encoding == Encoding::Ansi && !IsReadOnly() )
    {
        if( !CStdioFileUnicode::ConvertAnsiToUTF8(m_connectionString.GetFilePath()) )
            throw DataRepositoryException::IOError("Could not convert the writeable data file from ANSI to UTF-8.");

        m_encoding = Encoding::Utf8;
    }

    else if( m_encoding != Encoding::Utf8 && m_encoding != Encoding::Ansi )
    {
        throw DataRepositoryException::IOError("CSPro does not support the specified text encoding.");
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

    // open the notes and statuses
    if( m_caseAccess->GetUsesNotes() )
        m_notesFile.reset(new TextRepositoryNotesFile(*this, open_flag));

    if( m_caseAccess->GetUsesStatuses() || m_caseAccess->GetUsesCaseLabels() )
        m_statusFile.reset(new TextRepositoryStatusFile(*this, open_flag));
}


void TextRepository::ToggleReadWriteMode()
{
    ASSERT(m_accessType == DataRepositoryAccess::ReadOnly || m_accessType == DataRepositoryAccess::ReadWrite);
    CloseDataFile();

    m_accessType = ( m_accessType == DataRepositoryAccess::ReadOnly ) ? DataRepositoryAccess::ReadWrite :
                                                                        DataRepositoryAccess::ReadOnly;
    OpenDataFile();
}


void TextRepository::Close()
{
    if( m_useTransactionManager )
    {
        CommitTransactions();
        TransactionManager::Deregister(*this);
    }

    CloseDataFile();
    CloseIndex();

    m_notesFile.reset();
    m_statusFile.reset();
}


void TextRepository::CloseDataFile()
{
    if( m_file != nullptr )
    {
        fclose(m_file);
        m_file = nullptr;
    }
}


int TextRepository::GetPercentRead() const
{
    return 100 - CreatePercent(m_fileBytesRemaining, m_fileSize);
}


void TextRepository::DeleteRepository()
{
    Close();

    DeleteRepositoryFiles(m_connectionString);
}


void TextRepository::DeleteRepositoryFiles(const ConnectionString& connection_string)
{
    auto delete_file = [](const std::string& file_path)
    {
        if( PortableFunctions::FileIsRegular(file_path) && !PortableFunctions::FileDelete(file_path) )
            throw DataRepositoryException::DeleteRepositoryError();
    };

    // delete the data, index, notes, and status files
    delete_file(connection_string.GetFilePath());
    delete_file(PortableFunctions::PathAppendFileExtension(connection_string.GetFilePath(), FileExtensions::Data::IndexableTextIndex));
    delete_file(TextRepositoryNotesFile::GetNotesFilePath(connection_string));
    delete_file(TextRepositoryStatusFile::GetStatusFilePath(connection_string));
}


void TextRepository::RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string)
{
    DeleteRepositoryFiles(new_connection_string);

    auto rename_file = [](const std::string& old_file_path, const std::string& new_file_path)
    {
        if( PortableFunctions::FileIsRegular(old_file_path) && !PortableFunctions::FileRename(old_file_path, new_file_path) )
            throw DataRepositoryException::RenameRepositoryError();
    };

    // rename the data, index, notes, and status files
    rename_file(old_connection_string.GetFilePath(), new_connection_string.GetFilePath());

    rename_file(PortableFunctions::PathAppendFileExtension(old_connection_string.GetFilePath(), FileExtensions::Data::IndexableTextIndex),
                PortableFunctions::PathAppendFileExtension(new_connection_string.GetFilePath(), FileExtensions::Data::IndexableTextIndex));

    rename_file(TextRepositoryNotesFile::GetNotesFilePath(old_connection_string), TextRepositoryNotesFile::GetNotesFilePath(new_connection_string));

    rename_file(TextRepositoryStatusFile::GetStatusFilePath(old_connection_string), TextRepositoryStatusFile::GetStatusFilePath(new_connection_string));
}


std::vector<std::string> TextRepository::GetAssociatedFileList(const ConnectionString& connection_string)
{
    return
    {
        connection_string.GetFilePath(),
        TextRepositoryNotesFile::GetNotesFilePath(connection_string),
        TextRepositoryStatusFile::GetStatusFilePath(connection_string)
    };
}


void TextRepository::OpenDataFile()
{
    m_file = PortableFunctions::FileOpen(m_connectionString.GetFilePath(), IsReadOnly() ? "rb" : "rb+");

    if( m_file == nullptr )
        throw DataRepositoryException::IOError("The data file could not be opened.");

    // create memory for the read and write buffers
    if( m_utf8AnsiBuffer == nullptr )
        m_utf8AnsiBuffer = new char[m_utf8AnsiBufferSize];

    if( m_wideBuffer == nullptr )
        m_wideBuffer = new wchar_t[m_wideBufferSize];

    // determine the size of the file
    PortableFunctions::fseeki64(m_file, 0, SEEK_END);
    m_fileSize = PortableFunctions::ftelli64(m_file);

    // move to the end if appending, or...
    if( m_accessType == DataRepositoryAccess::BatchOutputAppend )
    {
        ResetPosition(m_fileSize);
    }

    // ...the beginning of where cases should be in the file
    else
    {
        ResetPositionToBeginning();
    }
}


uint32_t TextRepository::GetIdStructureHashForKeyIndex() const
{
    return m_caseAccess->GetDataDict().GetIdStructureHashForKeyIndex(false, true);
}


std::shared_ptr<IndexableTextRepository::IndexCreator> TextRepository::GetIndexCreator()
{
    static const std::vector<const char*> CreateIndexSqlStatements = { Sqlite::Commands::CreateKeyTablePositionIndex };
    m_indexCreator = std::make_shared<TextRepositoryIndexCreator>(*this, CreateIndexSqlStatements);
    return m_indexCreator;
}


std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>> TextRepository::GetSqlStatementsToPrepare()
{
    return std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>>
    {
        { Sqlite::Commands::InsertKey, m_stmtInsertKey },
        { Sqlite::Commands::KeyExists, m_stmtKeyExists }
    };
}


std::variant<const char*, std::shared_ptr<SQLiteStatement>> TextRepository::GetSqlStatementForQuery(const SqlQueryType type)
{
    switch( type )
    {
        case SqlQueryType::ContainsNotDeletedKey:             return m_stmtKeyExists;
        case SqlQueryType::GetPositionBytesFromNotDeletedKey: return Sqlite::Commands::QueryPositionBytesByKey;
        case SqlQueryType::GetBytesFromPosition:              return Sqlite::Commands::QueryBytesByPosition;
        case SqlQueryType::CountNotDeletedKeys:               return Sqlite::Commands::CountKeys;
    }

    throw ProgrammingErrorException();
}


void TextRepository::ResetPosition(const int64_t file_position)
{
    ASSERT(file_position >= 0 && file_position <= m_fileSize);
    PortableFunctions::fseeki64(m_file, file_position, SEEK_SET);
}


void TextRepository::ResetPositionToBeginning()
{
    const size_t file_position = ( m_encoding == Encoding::Utf8 ) ? TextEncoding::Utf8Bom_sv.length() : 0;
    ResetPosition(file_position);

    // reset the reading buffers
    m_fileBytesRemaining = ( m_fileSize - file_position );
    m_utf8AnsiBufferPosition = m_utf8AnsiBuffer + m_utf8AnsiBufferSize;
    m_utf8AnsiBufferEnd = m_utf8AnsiBufferPosition;
    m_utf8AnsiBufferActualEnd = m_utf8AnsiBufferEnd;
    m_wideBufferPosition = m_wideBuffer + m_wideBufferSize;
    m_wideBufferEnd = m_wideBufferPosition;
    m_ignoreNextCharacterIfNewline = false;
    m_wideBufferLines.clear();
}


bool TextRepository::FillUtf8AnsiTextBufferForKeyChangeReading()
{
    const size_t bytes_not_included_in_previous_read = ( m_utf8AnsiBufferActualEnd - m_utf8AnsiBufferEnd );
    const size_t bytes_to_copy_from_previous_read = ( m_utf8AnsiBufferEnd - m_utf8AnsiBufferPosition ) + bytes_not_included_in_previous_read;
    size_t bytes_to_read = static_cast<size_t>(std::min(m_fileBytesRemaining, static_cast<int64_t>(m_utf8AnsiBufferSize)));
    char* utf8_ansi_buffer_offset = m_utf8AnsiBuffer;

    // if there were any bytes not processed previously, use them
    if( bytes_to_copy_from_previous_read > 0 )
    {
        memmove(m_utf8AnsiBuffer, m_utf8AnsiBufferPosition, bytes_to_copy_from_previous_read);
        utf8_ansi_buffer_offset += bytes_to_copy_from_previous_read;
        bytes_to_read = std::min(bytes_to_read, m_utf8AnsiBufferSize - bytes_to_copy_from_previous_read);
    }

    else if( bytes_to_read == 0 )
    {
        return false;
    }

    // read the bytes
    if( fread(utf8_ansi_buffer_offset, 1, bytes_to_read, m_file) != bytes_to_read )
        throw DataRepositoryException::GenericReadError();

    m_fileBytesRemaining -= bytes_to_read;
    m_utf8AnsiBufferPosition = m_utf8AnsiBuffer;
    m_utf8AnsiBufferEnd = utf8_ansi_buffer_offset + bytes_to_read;
    m_utf8AnsiBufferActualEnd = m_utf8AnsiBufferEnd;

    // because routines that use this buffer make decisions based on newlines, don't return a buffer
    // that ends in a \r character because the next buffer will most likely contain a \n character
    if( m_fileBytesRemaining > 0 && *( m_utf8AnsiBufferEnd - 1 ) == '\r' )
        --m_utf8AnsiBufferEnd;

    // when indexing the file, parse the read bytes, storing information about newlines
    if( m_indexCreator != nullptr )
    {
        TextRepositoryIndexerPositions& indexer_positions = m_indexCreator->GetIndexerPositions();

        const char* buffer_position = utf8_ansi_buffer_offset - bytes_not_included_in_previous_read;

        for( ; buffer_position < m_utf8AnsiBufferEnd; ++buffer_position )
        {
            ++indexer_positions.file_position;

            if( is_crlf(*buffer_position) )
            {
                // treat \r\n as a pair
                if( *buffer_position == '\r' && ( buffer_position + 1 ) < m_utf8AnsiBufferEnd && *( buffer_position + 1 ) == '\n' )
                {
                    ++indexer_positions.file_position;
                    ++buffer_position;
                }

                ++indexer_positions.line_number;

                indexer_positions.AddEntry();
            }
        }
    }

    return true;
}


bool TextRepository::ReadUntilKeyChange()
{
    // a routine for finding the first non-skipped record that can be used to generate the key
    const wchar_t* first_line_key = nullptr;

    auto find_first_line_key = [&]()
    {
        for( auto line_iterator = m_wideBufferLines.cbegin() + m_wideBufferLineCaseStartLineIndex; line_iterator != m_wideBufferLines.cend(); ++line_iterator )
        {
            // skip blank records
            if( line_iterator->length == 0 )
                continue;

            first_line_key = m_firstLineKeyFullLineProcessor.GetLine(m_wideBuffer + line_iterator->offset, line_iterator->length, m_keyEnd);
            return;
        }
    };

    auto this_line_key_matches_first_line_key = [&](const wchar_t* buffer, size_t buffer_length) -> bool
    {
        const wchar_t* const this_line_key = m_currentLineKeyFullLineProcessor.GetLine(buffer, buffer_length, m_keyEnd);

        for( const TextToCaseConverter::TextSpan& key_span : m_keyMetadata->key_spans )
        {
            if( _tmemcmp(first_line_key + key_span.start, this_line_key + key_span.start, key_span.length) != 0 )
                return false;
        }

        return true;
    };


    // m_wideBufferLines will come in empty (for the first read)
    if( m_wideBufferLines.empty() )
    {
        m_wideBufferLineCaseStartLineIndex = 0;
    }

    // or with information for the previously read line (which will be the first line of this case)
    else
    {
        m_wideBufferLineCaseStartLineIndex = m_wideBufferLines.size() - 1;

        find_first_line_key();

        // every so often adjust the size of the wide buffer lines vector
        if( m_wideBufferLineCaseStartLineIndex > TextToCaseConverter::MinBufferLinesResizeCount )
        {
            m_wideBufferLines.erase(m_wideBufferLines.begin(), m_wideBufferLines.begin() + m_wideBufferLineCaseStartLineIndex);
            m_wideBufferLineCaseStartLineIndex = 0;
        }
    }

    // add an entry for the new line
    m_wideBufferLines.emplace_back(TextToCaseConverter::TextBufferLine { static_cast<size_t>(m_wideBufferPosition - m_wideBuffer), 0 });
    TextToCaseConverter::TextBufferLine* current_wide_buffer_line = &m_wideBufferLines.back();
    bool force_wide_buffer_resize_to_accommodate_utf8_characters = false;

    while( true )
    {
        // if we have reached the end of the wide buffer, read in more text
        if( m_wideBufferPosition == m_wideBufferEnd )
        {
            // if at the end of the file, check if a case (with a valid key) has been read
            if( !FillUtf8AnsiTextBufferForKeyChangeReading() )
            {
                // a dummy line will get added when the file ends without a newline because other routines
                // expect there to be one more line than the number that make up this case
                auto add_dummy_line_to_account_for_no_newline_ending = [&]()
                {
                    m_wideBufferLines.emplace_back(TextToCaseConverter::TextBufferLine { static_cast<size_t>(m_wideBufferPosition - m_wideBuffer), 0 });
                };

                if( first_line_key != nullptr )
                {
                    // if the file ended without a newline, potentially add a dummy line
                    const wchar_t last_char = *(m_wideBufferPosition - 1);

                    if( last_char != '\r' && last_char != '\n' )
                    {
                        // check if the last read line's key matches the first line key;
                        // if so, add the dummy line; if not, return without adding the dummy line
                        // because that last line needs to be processed as the next case
                        ASSERT(( m_wideBufferLineCaseStartLineIndex + 1 ) != m_wideBufferLines.size());
                        const TextToCaseConverter::TextBufferLine& last_line = m_wideBufferLines.back();

                        if( this_line_key_matches_first_line_key(m_wideBuffer + last_line.offset, last_line.length) )
                            add_dummy_line_to_account_for_no_newline_ending();
                    }

                    return true;
                }

                // we end up here if the file only contains a single-line case with no ending newline
                // or if a dummy line was added to the previous case
                find_first_line_key();

                if( first_line_key != nullptr )
                {
                    ASSERT(m_wideBufferLineCaseStartLineIndex == 0);
                    add_dummy_line_to_account_for_no_newline_ending();
                    return true;
                }

                return false;
            }

            // if not at the end of the file, add this text to the wide buffer
            else
            {
                ASSERT(m_utf8AnsiBufferPosition == m_utf8AnsiBuffer && m_utf8AnsiBufferPosition < m_utf8AnsiBufferEnd);

                const size_t this_case_buffer_offset = m_wideBufferLines[m_wideBufferLineCaseStartLineIndex].offset;
                const size_t buffer_size_currently_used = ( m_wideBufferPosition - m_wideBuffer ) - this_case_buffer_offset;

                // if the whole buffer is currently being used by a single case, double the size of the buffer
                if( ( buffer_size_currently_used == m_wideBufferSize ) || force_wide_buffer_resize_to_accommodate_utf8_characters )
                {
                    ASSERT(this_case_buffer_offset == 0);

                    m_wideBufferSize *= 2;

                    wchar_t* new_wide_buffer = new wchar_t[m_wideBufferSize];
                    _tmemcpy(new_wide_buffer, m_wideBuffer, buffer_size_currently_used);
                    delete[] m_wideBuffer;

                    m_wideBuffer = new_wide_buffer;

                    force_wide_buffer_resize_to_accommodate_utf8_characters = false;
                }

                // otherwise move any lines in the wide buffer that are currently are part of this
                // case and adjust the line offsets
                else
                {
                    memmove(m_wideBuffer, m_wideBuffer + this_case_buffer_offset, sizeof(wchar_t) * buffer_size_currently_used);

                    for( auto line_iterator = m_wideBufferLines.begin() + m_wideBufferLineCaseStartLineIndex; line_iterator != m_wideBufferLines.end(); ++line_iterator )
                        line_iterator->offset -= this_case_buffer_offset;
                }

                ASSERT(m_wideBufferLines[m_wideBufferLineCaseStartLineIndex].offset == 0);

                const size_t remaining_buffer_size = m_wideBufferSize - buffer_size_currently_used;
                ASSERT(remaining_buffer_size > 0);

                m_utf8AnsiBufferPosition = std::min(m_utf8AnsiBufferEnd, m_utf8AnsiBufferPosition + remaining_buffer_size);

                // if using UTF-8, make sure that the conversion doesn't happen in the middle of a UTF-8 sequence
                if( m_encoding == Encoding::Utf8 && m_utf8AnsiBufferPosition > m_utf8AnsiBuffer )
                {
                    // if the last byte is part of a sequence, go back to just before the beginning of the sequence
                    if( ( *( m_utf8AnsiBufferPosition - 1 ) & 0x80 ) == 0x80 )
                    {
                        do
                        {
                            --m_utf8AnsiBufferPosition;

                        } while( ( m_utf8AnsiBufferPosition > m_utf8AnsiBuffer ) && ( ( *m_utf8AnsiBufferPosition & 0xC0 ) != 0xC0 ) );

                        // the sequence will be handled in the next conversion
                        ASSERT(Constants::TextBufferSize >= 4);

                        // it is exceedingly unlikely, but if a sequence happens at the same time that nearly the whole
                        // wide buffer is filled, then using remaining_buffer_size above will have resulted
                        // in m_utf8AnsiBufferPosition pointing to nothing but the middle of a sequence, so this check
                        // will double the wide buffer size so that this doesn't happen on the next time through this loop
                        if( m_utf8AnsiBufferPosition == m_utf8AnsiBuffer )
                            force_wide_buffer_resize_to_accommodate_utf8_characters = true;
                    }
                }

                // convert from the read UTF-8/ANSI text buffer to the wide buffer
                m_wideBufferPosition = m_wideBuffer + buffer_size_currently_used;

                const size_t utf8_ansi_bytes_to_copy = m_utf8AnsiBufferPosition - m_utf8AnsiBuffer;

                ASSERT(utf8_ansi_bytes_to_copy > 0 || force_wide_buffer_resize_to_accommodate_utf8_characters);

                const  size_t characters_converted = UTF8Convert::EncodedCharsBufferToWideBuffer(m_encoding,
                                                                                                 m_utf8AnsiBuffer, utf8_ansi_bytes_to_copy,
                                                                                                 const_cast<wchar_t*>(m_wideBufferPosition), remaining_buffer_size);

                m_wideBufferEnd = m_wideBufferPosition + characters_converted;

                // reset this value because it may be pointing to memory that was deleted or moved
                find_first_line_key();
            }
        }


        // process the lines
        while( m_wideBufferPosition < m_wideBufferEnd )
        {
            const bool last_character_was_slash_r_newline = m_ignoreNextCharacterIfNewline;
            m_ignoreNextCharacterIfNewline = false;

            // process newline characters
            if( is_crlf(*m_wideBufferPosition) )
            {
                // treat \r\n as a pair
                if( *(m_wideBufferPosition++) == '\r' )
                {
                    m_ignoreNextCharacterIfNewline = true;
                }

                else if( last_character_was_slash_r_newline )
                {
                    ++current_wide_buffer_line->offset;
                    continue;
                }

                if( current_wide_buffer_line->length > 0 )
                {
                    const wchar_t* current_wide_buffer_line_start_position = m_wideBuffer + current_wide_buffer_line->offset;

                    // for erased records, mark the line as having length 0
                    if( *current_wide_buffer_line_start_position == TextToCaseConverter::DataFileErasedRecordCharacter )
                    {
                        current_wide_buffer_line->length = 0;
                    }

                    else
                    {
                        // when at the end of a line, compare the key against the previous key
                        if( first_line_key == nullptr )
                        {
                            find_first_line_key();
                        }

                        else
                        {
                            // if the key doesn't match, we're done reading this case
                            if( !this_line_key_matches_first_line_key(current_wide_buffer_line_start_position, current_wide_buffer_line->length) )
                                return true;
                        }
                    }
                }

                // otherwise add a new entry for the next line
                m_wideBufferLines.emplace_back(TextToCaseConverter::TextBufferLine { static_cast<size_t>(m_wideBufferPosition - m_wideBuffer), 0 });
                current_wide_buffer_line = &m_wideBufferLines.back();
            }

            // if not at the end of a line, update the current line information
            else
            {
                ++current_wide_buffer_line->length;
                ++m_wideBufferPosition;
            }
        }
    }

    return ReturnProgrammingError(false);
}


void TextRepository::PopulateCaseIdentifiers(std::string& key, std::string& uuid, double& position_in_repository)
{
    // search the index by key
    if( !key.empty() )
    {
        position_in_repository = static_cast<double>(std::get<int64_t>(GetPositionBytesFromKey(key)));
    }

    // text files don't have UUIDs
    else if( !uuid.empty() )
    {
        throw DataRepositoryException::CaseNotFound();
    }

    // or by file position
    else
    {
        key = GetKeyFromPosition(static_cast<int64_t>(position_in_repository));
    }

    ASSERT(uuid.empty());
}


DataRepositoryUniqueCaseIdentifer TextRepository::GetUniqueCaseIdentifer(const CaseKey& case_key)
{
    return DataRepositoryUniqueCaseIdentifer(DataRepositoryUniqueCaseIdentifer::Type::Key, case_key.GetKey());
}


std::string TextRepository::GetKeyFromPosition(const int64_t file_position)
{
    EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryKeyByPosition, m_stmtQueryKeyByPosition);
    const SQLiteResetOnDestruction rod(*m_stmtQueryKeyByPosition);

    m_stmtQueryKeyByPosition->Bind(1, file_position);

    if( m_stmtQueryKeyByPosition->Step() != SQLITE_ROW )
        throw DataRepositoryException::CaseNotFound();

    return m_stmtQueryKeyByPosition->GetColumn<std::string>(0);
}


SQLiteStatement TextRepository::CreateKeySearchIteratorStatement(const char* const columns_to_query,
                                                                 const CaseIteratorSettings& iterator_settings,
                                                                 const size_t offset, const size_t limit)
{
    std::string order_by_text;

    if( iterator_settings.GetMethod().has_value() )
    {
        const CaseIterationMethod iteration_method = *iterator_settings.GetMethod();
        const std::optional<CaseIterationOrder>& iteration_order = iterator_settings.GetOrder();

        const char* const column = ( iteration_method == CaseIterationMethod::KeyOrder ) ? "`Key`" :
                                                                                           "`Position`";

        const char* const order = ( !iteration_order.has_value() )                       ? "" :
                                  ( *iteration_order == CaseIterationOrder::Ascending )  ? "ASC" :
                                                                                           "DESC";

        order_by_text = FormatText("ORDER BY %s %s ", column, order);
    }

    const std::string limit_text = FormatText("LIMIT %d OFFSET %d ",
                                              ( limit == SIZE_MAX ) ? -1 : static_cast<int>(limit),
                                              static_cast<int>(offset));

    // process any filters
    const CaseIteratorParameters* const start_parameters = iterator_settings.GetParameters();
    bool use_key_prefix = false;
    bool use_operators = false;
    std::string where_text;

    if( start_parameters != nullptr )
    {
        // use the key prefix if it is set and is not empty
        if( start_parameters->key_prefix.has_value() && !start_parameters->key_prefix->empty() )
        {
            use_key_prefix = true;
            where_text = "WHERE `Key` >= ? AND `Key` < ?";

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
            where_text.append(FormatText(where_text.empty() ? "WHERE %s %s ?" : " AND %s %s ?",
                                         std::holds_alternative<std::string>(start_parameters->first_key_or_position) ? "`Key`" : "`Position`",
                                         ToString(start_parameters->start_type)));
        }
    }

    const std::string sql = FormatText("SELECT %s FROM `Keys` %s %s %s;", columns_to_query,
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


std::optional<CaseKey> TextRepository::FindCaseKey(const CaseIterationMethod iteration_method, const CaseIterationOrder iteration_order,
                                                   const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    const CaseIteratorSettings iterator_settings(CaseIterationCaseStatus::NotDeletedOnly, iteration_method, iteration_order, start_parameters);

    SQLiteStatement stmt_query_keys = CreateKeySearchIteratorStatement("`Key`, `Position`", iterator_settings, 0, 1);

    if( stmt_query_keys.Step() == SQLITE_ROW )
        return CaseKey(stmt_query_keys.GetColumn<std::string>(0), stmt_query_keys.GetColumn<double>(1));

    return std::nullopt;
}


void TextRepository::FillUtf8AnsiTextBufferForCaseReading(const int64_t file_position, const size_t bytes_for_case)
{
    // make sure that the UTF-8/ANSI text buffer is large enough to read this case
    if( bytes_for_case > m_utf8AnsiBufferSize )
    {
        delete[] m_utf8AnsiBuffer;
        m_utf8AnsiBuffer = new char[bytes_for_case];
        m_utf8AnsiBufferSize = bytes_for_case;
    }

    ResetPosition(file_position);

    if( fread(m_utf8AnsiBuffer, 1, bytes_for_case, m_file) != bytes_for_case )
        throw DataRepositoryException::GenericReadError();
}


void TextRepository::SetUpOtherCaseAttributes(Case& data_case, const double file_position) const
{
    data_case.SetPositionInRepository(file_position);

    if( !data_case.GetUuid().empty() )
        data_case.SetUuid(std::string());

    data_case.SetDeleted(false);

    // update the notes
    if( m_notesFile != nullptr )
        m_notesFile->SetUpCase(data_case);

    // update the status information
    if( m_statusFile != nullptr )
        m_statusFile->SetUpCase(data_case);

    data_case.GetVectorClock().clear();
}


void TextRepository::ReadCase(Case& data_case, const int64_t file_position, const size_t bytes_for_case)
{
    // read the case
    FillUtf8AnsiTextBufferForCaseReading(file_position, bytes_for_case);

    if( m_encoding == Encoding::Utf8 )
    {
        m_textToCaseConverter->TextUtf8ToCase(data_case, m_utf8AnsiBuffer, bytes_for_case);
    }

    else
    {
        // if not UTF-8, convert to wide characters
        if( bytes_for_case > m_wideBufferSize )
        {
            delete[] m_wideBuffer;
            m_wideBuffer = new wchar_t[bytes_for_case];
            m_wideBufferSize = bytes_for_case;
        }

        const size_t characters_converted = UTF8Convert::EncodedCharsBufferToWideBuffer(m_encoding,
                                                                                        m_utf8AnsiBuffer, bytes_for_case,
                                                                                        m_wideBuffer, m_wideBufferSize);

        m_textToCaseConverter->TextWideToCase(data_case, m_wideBuffer, characters_converted);
    }

    SetUpOtherCaseAttributes(data_case, static_cast<double>(file_position));
}


void TextRepository::ReadCaseByUuid(Case& /*data_case*/, const std::string& /*uuid*/)
{
    // text files don't have UUIDs
    throw DataRepositoryException::CaseNotFound();
}


void TextRepository::SetUpBatchCase(Case& data_case)
{
    const auto& line_iterator_begin = m_wideBufferLines.cbegin() + m_wideBufferLineCaseStartLineIndex;
    const auto& line_iterator_end = m_wideBufferLines.cend() - 1;

    m_textToCaseConverter->TextWideToCase(data_case, m_wideBuffer, line_iterator_begin, line_iterator_end);

    SetUpOtherCaseAttributes(data_case, -1);
}


void TextRepository::WriteCase(Case& data_case, const WriteCaseParameter* const write_case_parameter/* = nullptr*/)
{
    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    if( data_case.GetDeleted() )
    {
        DeleteCaseViaWriteCase(data_case, write_case_parameter);
        return;
    }

    // update the notes and statues
    if( m_notesFile != nullptr )
        m_notesFile->WriteCase(data_case, write_case_parameter);

    if( m_statusFile != nullptr )
        m_statusFile->WriteCase(data_case, write_case_parameter);

    size_t output_text_length;
    const char* const output_text = m_textToCaseConverter->CaseToTextUtf8(data_case, &output_text_length);

    // quickly write out the case and get out (for batch processing)
    if( m_accessType == DataRepositoryAccess::BatchOutput ||
        m_accessType == DataRepositoryAccess::BatchOutputAppend )
    {
        ASSERT(PortableFunctions::ftelli64(m_file) == m_fileSize);

        if( fwrite(output_text, 1, output_text_length, m_file) != output_text_length )
            throw DataRepositoryException::GenericWriteError();

        data_case.SetPositionInRepository(static_cast<double>(m_fileSize));
        m_fileSize += output_text_length;

        return;
    }

    // otherwise there are four ways that a case can be written out:
    // 1) at the end of the file
    // 2) inserted at a position in the file
    // 3) if the case already exists, replace the case and shift any cases that came after that case
    // 4) if the case already exists, replace the case if there is enough space; otherwise delete it and write it at the end of the file
    ASSERT(m_requiresIndex);

    enum class WriteMethod { EndOfFile, Insert, Replace, ReplaceIfSpace };

    WriteMethod write_method = WriteMethod::Replace;
    const std::string this_key = data_case.GetKey();
    std::string key_to_search;

    // this will be from CSEntry or from Data.writeCase
    if( write_case_parameter != nullptr )
    {
        if( write_case_parameter->IsModifyParameter() )
        {
            key_to_search = write_case_parameter->GetKey();
        }

        else
        {
            ASSERT(write_case_parameter->IsInsertParameter());
            write_method = WriteMethod::Insert;
            key_to_search = GetKeyFromPosition(static_cast<int64_t>(write_case_parameter->GetPositionInRepository()));
        }
    }

    else
    {
        key_to_search = this_key;

        // this will be from a writecase logic function
        if( m_accessType != DataRepositoryAccess::EntryInput )
            write_method = WriteMethod::ReplaceIfSpace;
    }

    // search the index for the key
    const auto [replacement_file_position, bytes_for_replacement_case] = GetPositionBytesFromKey(key_to_search, false);

    // if we are not inserting or replacing a case, put the case at the end of the file
    if( replacement_file_position < 0 )
    {
        write_method = WriteMethod::EndOfFile;
    }

    else
    {
        if( write_method == WriteMethod::Insert )
        {
            GrowOrShrinkFileAndIndex(replacement_file_position, output_text_length);
        }

        else if( write_method == WriteMethod::ReplaceIfSpace )
        {
            if( bytes_for_replacement_case == output_text_length )
            {
                write_method = WriteMethod::Replace;
            }

            // otherwise delete the case and add it to the end
            else
            {
                ASSERT(m_deleteCaseShouldDeleteNotesAndStatuses);
                const RAII::SetValueAndRestoreOnDestruction delete_case_modifier(m_deleteCaseShouldDeleteNotesAndStatuses, false);

                IndexableTextRepository::DeleteCase(this_key);
                write_method = WriteMethod::EndOfFile;
            }
        }

        if( write_method == WriteMethod::Replace )
        {
            bool need_to_update_index = false;
            const int new_size_differential = output_text_length - bytes_for_replacement_case;

            if( new_size_differential != 0 )
            {
                GrowOrShrinkFileAndIndex(replacement_file_position + bytes_for_replacement_case, new_size_differential);
                need_to_update_index = true;
            }

            // update the index if the key has changed
            if( need_to_update_index || this_key != key_to_search )
            {
                if( m_useTransactionManager )
                    WrapInTransaction();

                EnsureSqlStatementIsPrepared(Sqlite::Commands::ModifyKey, m_stmtModifyKey);
                const SQLiteResetOnDestruction rod(*m_stmtModifyKey);

                m_stmtModifyKey->Bind(1, this_key);
                m_stmtModifyKey->Bind(2, replacement_file_position);
                m_stmtModifyKey->Bind(3, bytes_for_replacement_case + new_size_differential);
                m_stmtModifyKey->Bind(4, key_to_search);

                if( m_stmtModifyKey->Step() != SQLITE_DONE )
                    throw DataRepositoryException::SQLiteError();
            }
        }
    }


    // write out the case contents
    const int64_t write_position = ( write_method == WriteMethod::EndOfFile ) ? m_fileSize :
                                                                                replacement_file_position;

    ResetPosition(write_position);

    if( fwrite(output_text, 1, output_text_length, m_file) != output_text_length )
        throw DataRepositoryException::GenericWriteError();

    if( m_accessType == DataRepositoryAccess::EntryInput )
        fflush(m_file);

    // update the index (if it wasn't already done above)
    if( write_method != WriteMethod::Replace )
    {
        if( m_useTransactionManager )
            WrapInTransaction();

        ASSERT(m_stmtInsertKey != nullptr);
        const SQLiteResetOnDestruction rod(*m_stmtInsertKey);

        m_stmtInsertKey->Bind(1, this_key);
        m_stmtInsertKey->Bind(2, write_position);
        m_stmtInsertKey->Bind(3, output_text_length);

        if( m_stmtInsertKey->Step() != SQLITE_DONE )
            throw DataRepositoryException::SQLiteError();

        if( write_method == WriteMethod::EndOfFile )
            m_fileSize += output_text_length;

        data_case.SetPositionInRepository(static_cast<double>(write_position));
    }
}


void TextRepository::DeleteCaseViaWriteCase(const Case& data_case, const WriteCaseParameter* const write_case_parameter)
{
    // the Data.writeCase action can be used to write cases marked as deleted;
    // in repositories that support duplicates, this operation is allowed even if the case does not exist;
    // here we will throw an exception only if the user specifies a specific case (write_case_parameter != nullptr)
    // and that case does not exist
    ASSERT(data_case.GetDeleted());

    if( write_case_parameter == nullptr )
    {
        try
        {
            IndexableTextRepository::DeleteCase(data_case.GetKey());
        }

        catch( const DataRepositoryException::CaseNotFound& )
        {
            // if the case cannot be found, that is fine;
            // only deletion-specific exceptions will be thrown
        }
    }

    else
    {
        ASSERT(write_case_parameter->IsModifyParameter());
        IndexableTextRepository::DeleteCase(write_case_parameter->GetPositionInRepository());
    }
}


void TextRepository::DeleteCase(const int64_t file_position, const size_t bytes_for_case, const bool deleted, const std::string* const key_if_known)
{
    ASSERT(deleted);

    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    // look up the key if needed for updating the notes or status files
    cs::shared_or_raw_ptr key_lookup = key_if_known;

    if( key_lookup == nullptr && ( m_notesFile != nullptr || m_statusFile != nullptr ) )
        key_lookup = std::make_shared<std::string>(GetKeyFromPosition(file_position));

    // check if this is the last case in the file
    EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryIsLastPosition, m_stmtQueryIsLastPosition);
    SQLiteResetOnDestruction last_position_rod(*m_stmtQueryIsLastPosition);

    m_stmtQueryIsLastPosition->Bind(1, file_position);

    const bool is_last_case = ( m_stmtQueryIsLastPosition->Step() != SQLITE_ROW );

    // update the index to remove the case
    if( m_useTransactionManager )
        WrapInTransaction();

    EnsureSqlStatementIsPrepared(Sqlite::Commands::DeleteKeyByPosition, m_stmtDeleteKeyByPosition);
    SQLiteResetOnDestruction delete_key_rod(*m_stmtDeleteKeyByPosition);

    m_stmtDeleteKeyByPosition->Bind(1, file_position);

    if( m_stmtDeleteKeyByPosition->Step() != SQLITE_DONE )
        throw DataRepositoryException::SQLiteError();

    // check a couple conditions:
    // - if this is the last case in the repository, we can truncate the file
    // - if issued from delcase, then we can remove the case with tildes
    //      - if this is the first case in the repository, delete the case and modify the "bytes for case" value of the next case in the repository
    //      - otherwise, delete the case and modify the "bytes for case" value of the previous case in the repository

    if( is_last_case )
    {
        // this is the last case so we can truncate the file
        if( !PortableFunctions::FileTruncate(m_file, file_position) )
            throw DataRepositoryException::IOError(Constants::TruncationIOError);

        m_fileSize = file_position;
    }

    else if( m_accessType != DataRepositoryAccess::EntryInput )
    {
        std::string case_to_combine_key;
        int64_t case_to_combine_file_position;
        size_t case_to_combine_bytes_for_case;

        auto process_statement = [&](SQLiteStatement& stmt_query_key_by_position)
        {
            case_to_combine_key = stmt_query_key_by_position.GetColumn<std::string>(0);
            case_to_combine_file_position = stmt_query_key_by_position.GetColumn<int64_t>(1);
            case_to_combine_bytes_for_case = stmt_query_key_by_position.GetColumn<size_t>(2);
        };

        // check if there is an earlier case to combine with
        EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryPreviousKeyByPosition, m_stmtQueryPreviousKeyByPosition);
        SQLiteResetOnDestruction previous_key_rod(*m_stmtQueryPreviousKeyByPosition);

        m_stmtQueryPreviousKeyByPosition->Bind(1, file_position);

        if( m_stmtQueryPreviousKeyByPosition->Step() == SQLITE_ROW )
        {
            process_statement(*m_stmtQueryPreviousKeyByPosition);
        }

        // if not, there must be a later case
        else
        {
            EnsureSqlStatementIsPrepared(Sqlite::Commands::QueryNextKeyByPosition, m_stmtQueryNextKeyByPosition);
            SQLiteResetOnDestruction next_key_rod(*m_stmtQueryNextKeyByPosition);

            m_stmtQueryNextKeyByPosition->Bind(1, file_position);

            if( m_stmtQueryNextKeyByPosition->Step() != SQLITE_ROW )
                throw DataRepositoryException::SQLiteError();

            process_statement(*m_stmtQueryNextKeyByPosition);
        }

        // add tildes to the records to mark them as deleted
        DeleteCaseInPlace(file_position, bytes_for_case);

        // modify the index of the case that we're combining the deleted case with
        if( m_useTransactionManager )
            WrapInTransaction();

        EnsureSqlStatementIsPrepared(Sqlite::Commands::ModifyKey, m_stmtModifyKey);
        SQLiteResetOnDestruction modify_key_rod(*m_stmtModifyKey);

        m_stmtModifyKey->Bind(1, case_to_combine_key);
        m_stmtModifyKey->Bind(2, std::min(file_position, case_to_combine_file_position));
        m_stmtModifyKey->Bind(3, bytes_for_case + case_to_combine_bytes_for_case);
        m_stmtModifyKey->Bind(4, case_to_combine_key);

        if( m_stmtModifyKey->Step() != SQLITE_DONE )
            throw DataRepositoryException::SQLiteError();
    }

    // a delete coming from the CSEntry user interace, so shrink the file to remove the case
    else
    {
        GrowOrShrinkFileAndIndex(file_position + bytes_for_case, -1 * bytes_for_case);
    }

    if( m_accessType == DataRepositoryAccess::EntryInput )
        fflush(m_file);

    // update the notes and statuses
    if( m_deleteCaseShouldDeleteNotesAndStatuses )
    {
        if( m_notesFile != nullptr )
            m_notesFile->DeleteCase(*key_lookup);

        if( m_statusFile != nullptr )
            m_statusFile->DeleteCase(*key_lookup);
    }
}


void TextRepository::DeleteCaseInPlace(const int64_t file_position, const size_t bytes_for_case)
{
    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    // read the case text
    FillUtf8AnsiTextBufferForCaseReading(file_position, bytes_for_case);

    // and then insert tildes at the beginning of each line
    bool insert_tilde = true;

    const char* buffer_end = m_utf8AnsiBuffer + bytes_for_case;

    for( char* buffer_position = m_utf8AnsiBuffer; buffer_position < buffer_end; ++buffer_position )
    {
        // if this is a newline, insert the tilde at the next non-newline character
        if( is_crlf(*buffer_position) )
        {
            insert_tilde = true;
        }

        // swap the character with a tilde
        else if( insert_tilde )
        {
            *buffer_position = TextToCaseConverter::DataFileErasedRecordCharacter;
            insert_tilde = false;
        }
    }

    ResetPosition(file_position);

    if( fwrite(m_utf8AnsiBuffer, 1, bytes_for_case, m_file) != bytes_for_case )
        throw DataRepositoryException::GenericWriteError();
}


void TextRepository::GrowOrShrinkFileAndIndex(const int64_t file_position, const int bytes_differential)
{
    int64_t bytes_to_shift = m_fileSize - file_position;

    // shrinking the file
    if( bytes_differential < 0 )
    {
        int64_t next_read_position = file_position;

        while( bytes_to_shift > 0 )
        {
            const size_t bytes_to_read = std::min(m_utf8AnsiBufferSize, static_cast<size_t>(bytes_to_shift));

            FillUtf8AnsiTextBufferForCaseReading(next_read_position, bytes_to_read);

            // move back to write out the block
            ResetPosition(next_read_position + bytes_differential);

            if( fwrite(m_utf8AnsiBuffer, 1, bytes_to_read, m_file) != bytes_to_read )
                throw DataRepositoryException::GenericWriteError();

            // move forward to read the next block
            next_read_position += bytes_to_read;
            bytes_to_shift -= bytes_to_read;
        }

        // once the file contents have been shifted, we can truncate the file
        if( !PortableFunctions::FileTruncate(m_file, m_fileSize + bytes_differential) )
            throw DataRepositoryException::IOError(Constants::TruncationIOError);
    }


    // growing the file
    else if( bytes_differential > 0 )
    {
        // unlike the shrinking case, where the data was shifted reading initially from the starting point,
        // in the growing case the data will be initially read from the end of the file
        int64_t next_read_position = m_fileSize;

        while( bytes_to_shift > 0 )
        {
            const size_t bytes_to_read = std::min(m_utf8AnsiBufferSize, static_cast<size_t>(bytes_to_shift));

            next_read_position -= bytes_to_read;

            FillUtf8AnsiTextBufferForCaseReading(next_read_position, bytes_to_read);

            // move forward to write out the block
            PortableFunctions::fseeki64(m_file, next_read_position + bytes_differential, SEEK_SET);

            if( fwrite(m_utf8AnsiBuffer, 1, bytes_to_read, m_file) != bytes_to_read )
                throw DataRepositoryException::GenericWriteError();

            bytes_to_shift -= bytes_to_read;
        }
    }

    // modify the size of the file
    m_fileSize += bytes_differential;

    // update the file positions of all keys following the change
    if( m_useTransactionManager )
        WrapInTransaction();

    EnsureSqlStatementIsPrepared(Sqlite::Commands::ShiftKeys, m_stmtShiftKeys);
    const SQLiteResetOnDestruction rod(*m_stmtShiftKeys);

    m_stmtShiftKeys->Bind(1, bytes_differential);
    m_stmtShiftKeys->Bind(2, file_position);

    if( m_stmtShiftKeys->Step() != SQLITE_DONE )
        throw DataRepositoryException::SQLiteError();
}


void TextRepository::WrapInTransaction()
{
    ASSERT(m_useTransactionManager);

    if( m_numberTransactions == IndexableTextRepository::MaxNumberSqlInsertsInOneTransaction )
        CommitTransactions();

    if( m_numberTransactions > 0 || sqlite3_exec(m_db, Sqlite::Commands::BeginTransaction, nullptr, nullptr, nullptr) == SQLITE_OK )
        ++m_numberTransactions;
}


bool TextRepository::CommitTransactions()
{
    ASSERT(m_useTransactionManager);

    if( m_numberTransactions > 0 )
    {
        if( sqlite3_exec(m_db, Sqlite::Commands::EndTransaction, nullptr, nullptr, nullptr) != SQLITE_OK )
            throw DataRepositoryException::SQLiteError();

        m_numberTransactions = 0;
    }

    // update the notes and statuses
    if( m_notesFile != nullptr )
        m_notesFile->CommitTransactions();

    if( m_statusFile != nullptr )
        m_statusFile->CommitTransactions();

    // errors will be thrown
    return true;
}


size_t TextRepository::GetNumberCases(const CaseIterationCaseStatus case_status, const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    // text repositories cannot have duplicates
    if( case_status == CaseIterationCaseStatus::DuplicatesOnly )
        return 0;

    const bool partials_only = ( case_status == CaseIterationCaseStatus::PartialsOnly );

    // exit easily if no filtering
    if( start_parameters == nullptr )
    {
        if( !partials_only )
        {
            return IndexableTextRepository::GetNumberCases();
        }

        else
        {
            ASSERT(m_statusFile != nullptr);
            return m_statusFile->GetNumberPartials();
        }
    }

    // otherwise we must apply some filter
    const CaseIteratorSettings iterator_settings(case_status, std::nullopt, std::nullopt, start_parameters);
    size_t number_cases = 0;

    // if not filtering on partial saves, we can calculate the number easily
    if( !partials_only )
    {
        SQLiteStatement stmt_query_keys = CreateKeySearchIteratorStatement("COUNT(*)", iterator_settings, 0, SIZE_MAX);

        if( stmt_query_keys.Step() == SQLITE_ROW )
            number_cases = stmt_query_keys.GetColumn<size_t>(0);
    }

    // otherwise, calculate the number by iterating through all the case keys
    else
    {
        const std::unique_ptr<CaseIterator> case_key_iterator = CreateIterator(CaseIterationContent::CaseKey, iterator_settings);

        CaseKey case_key;

        while( case_key_iterator->NextCaseKey(case_key) )
            ++number_cases;
    }

    return number_cases;
}


std::unique_ptr<CaseIterator> TextRepository::CreateIterator(const CaseIterationContent iteration_content,
                                                             const CaseIteratorSettings& iterator_settings,
                                                             const size_t offset/* = 0*/, const size_t limit/* = SIZE_MAX*/)
{
    const bool partials_only = ( iterator_settings.GetStatus() == CaseIterationCaseStatus::PartialsOnly );

    // text repositories don't have duplicates; also if no partials exist, return a null iterator
    if( ( iterator_settings.GetStatus() == CaseIterationCaseStatus::DuplicatesOnly ) ||
        ( partials_only && ( m_statusFile == nullptr || !m_statusFile->ContainsPartials() ) ) )
    {
        return std::make_unique<NullRepositoryCaseIterator>();
    }

    // we can use a fast batch iterator if reading cases in file order (and not using an offset or limit)
    if( iteration_content == CaseIterationContent::Case &&
        iterator_settings.GetMethod() == CaseIterationMethod::SequentialOrder &&
        iterator_settings.GetOrder() == CaseIterationOrder::Ascending &&
        iterator_settings.GetParameters() == nullptr &&
        offset == 0 &&
        limit == SIZE_MAX
        // CR_TODO_ITERATOR eventually remove the next line, but for now, this fast iterator can't be used
        // until we set the repo position to something other than -1 in SetUpBatchCase
        && !m_requiresIndex )
    {
        return std::make_unique<TextRepositoryBatchCaseIterator>(*this, partials_only);
    }

    else
    {
        // partials have to be filtered by the iterator; otherwise, the offset and limit can be used
        SQLiteStatement stmt_query_keys = CreateKeySearchIteratorStatement("`Key`, `Position`, `Bytes`",
                                                                           iterator_settings,
                                                                           partials_only ? 0 : offset,
                                                                           partials_only ? SIZE_MAX : limit);

        return std::make_unique<TextRepositoryCaseIterator>(
            *this,
            std::move(stmt_query_keys),
            iterator_settings.GetStatus(),
            iterator_settings.GetParameters(),
            partials_only ? std::make_optional(std::make_tuple(offset, limit)) : std::nullopt
        );
    }
}



// --------------------------------------------------------------------------
// TextRepositoryIndexerPositions
// --------------------------------------------------------------------------

TextRepositoryIndexerPositions::TextRepositoryIndexerPositions(const int64_t file_bytes_remaining, const int64_t file_size)
    :   line_number(0),
        file_position(file_size - file_bytes_remaining),
        m_fileSize(file_size),
        m_nextLookupPosition(0)
{
    ASSERT(file_position == TextEncoding::Utf8Bom_sv.length() || file_position == 0);
    AddEntry();
}


int64_t TextRepositoryIndexerPositions::LookupEntry(const int64_t lookup_line_number)
{
    const auto& line_search = std::find_if(m_positions.begin() + m_nextLookupPosition, m_positions.end(),
                                           [&](const std::tuple<int64_t, int64_t>& position) { return ( std::get<0>(position) == lookup_line_number ); });
    int64_t lookup_file_position;

    // if there is no newline at the end of the file, the lookup will fail and the
    // position should be the file size
    if( line_search == m_positions.end() )
    {
        ASSERT(lookup_line_number == ( std::get<0>(m_positions.back()) + 1 ));
        lookup_file_position = m_fileSize;
    }

    else
    {
        lookup_file_position = std::get<1>(*line_search);

        m_nextLookupPosition = line_search - m_positions.begin();

        // don't allow the lookup to get too large
        if( m_nextLookupPosition > MinPositionsResizeCount )
        {
            m_positions.erase(m_positions.begin(), m_positions.begin() + m_nextLookupPosition);
            m_nextLookupPosition = 0;
        }
    }

    return lookup_file_position;
}



// --------------------------------------------------------------------------
// TextRepositoryIndexCreator
// --------------------------------------------------------------------------

TextRepositoryIndexCreator::TextRepositoryIndexCreator(TextRepository& text_repository, const std::vector<const char*>& create_index_sql_statements)
    :   m_textRepository(text_repository),
        m_createIndexSqlStatements(create_index_sql_statements),
        throwExceptionsOnDuplicateKeys(true),
        m_indexerPositions(m_textRepository.m_fileBytesRemaining, m_textRepository.m_fileSize),
        m_keyLength(m_textRepository.m_keyMetadata->key_length),
        m_processedLineNumbers(0)
{
}


void TextRepositoryIndexCreator::Initialize(const bool throw_exceptions_on_duplicate_keys)
{
    throwExceptionsOnDuplicateKeys = throw_exceptions_on_duplicate_keys;
}


const char* TextRepositoryIndexCreator::GetCreateKeyTableSql() const
{
    return Sqlite::Commands::CreateKeyTable;
}


const std::vector<const char*>& TextRepositoryIndexCreator::GetCreateIndexSqlStatements() const
{
    return m_createIndexSqlStatements;
}


int64_t TextRepositoryIndexCreator::GetFileSize() const
{
    return m_textRepository.m_fileSize;
}


int TextRepositoryIndexCreator::GetPercentRead() const
{
    return m_textRepository.GetPercentRead();
}


bool TextRepositoryIndexCreator::ReadCaseAndUpdateIndex(IndexableTextRepositoryIndexDetails& index_details)
{
    if( !m_textRepository.ReadUntilKeyChange() )
        return false;

    // generate the key finding a non-skipped record up to the second-to-last line (because the last line is for the next case)
    const auto& line_iterator_begin = m_textRepository.m_wideBufferLines.cbegin() + m_textRepository.m_wideBufferLineCaseStartLineIndex;
    const auto& line_iterator_end = m_textRepository.m_wideBufferLines.cend() - 1;

    for( auto line_iterator = line_iterator_begin; line_iterator != line_iterator_end; ++line_iterator )
    {
        // skip blank records
        if( line_iterator->length == 0 )
            continue;

        const wchar_t* first_line_key = m_textRepository.m_firstLineKeyFullLineProcessor.GetLine(m_textRepository.m_wideBuffer + line_iterator->offset,
                                                                                                 line_iterator->length, m_textRepository.m_keyEnd);

#ifdef UTF8_TODO // this was the old code (for reference)
        index_details.key.resize(m_keyLength);
        wchar_t* key_iterator = index_details.key.data();

        for( const auto& key_span : m_textRepository.m_keyMetadata->key_spans )
        {
            _tmemcpy(key_iterator, first_line_key + key_span.start, key_span.length);
            key_iterator += key_span.length;
        }
#else
        auto wide_key = std::make_unique_for_overwrite<wchar_t[]>(m_keyLength);
        wchar_t* key_iterator = wide_key.get();

        for( const auto& key_span : m_textRepository.m_keyMetadata->key_spans )
        {
            _tmemcpy(key_iterator, first_line_key + key_span.start, key_span.length);
            key_iterator += key_span.length;
        }

        index_details.key = UTF8_TODO::GetUtf8(wstring_view(wide_key.get(), m_keyLength));

        ASSERT(SO::WideLength(index_details.key) == m_keyLength);
#endif

        break;
    }

    // turn ␤ -> \n
    NewlineSubstitutor::MakeUnicodeNLToNewline(index_details.key);

#ifdef _DEBUG
    // check that the key generated is the same as what is generated by TextToCaseConverter
    Case index_case(m_textRepository.m_caseAccess->GetCaseMetadata());
    m_textRepository.SetUpBatchCase(index_case);
    ASSERT(index_case.GetKey() == index_details.key);
#endif

    // get the information on the line number and file position
    index_details.line_number = m_processedLineNumbers + 1;

    index_details.position = m_indexerPositions.LookupEntry(m_processedLineNumbers);

    m_processedLineNumbers += ( line_iterator_end - line_iterator_begin );
    int64_t file_position_next_case = m_indexerPositions.LookupEntry(m_processedLineNumbers);

    index_details.bytes = static_cast<size_t>(file_position_next_case - index_details.position);

    // update the index
    UpdateIndex(index_details);

    return true;
}


void TextRepositoryIndexCreator::OnSuccessfulCreation()
{
    m_textRepository.m_indexCreator.reset();

    m_textRepository.ResetPositionToBeginning();
}


void TextRepositoryIndexCreator::UpdateIndex(IndexableTextRepositoryIndexDetails& index_details)
{
    // check if the key already exists in the index
    ASSERT(m_textRepository.m_stmtKeyExists != nullptr);
    SQLiteResetOnDestruction key_exists_rod(*m_textRepository.m_stmtKeyExists);

    m_textRepository.m_stmtKeyExists->Bind(1, index_details.key);

    index_details.case_prevents_index_creation = ( m_textRepository.m_stmtKeyExists->Step() == SQLITE_ROW );

    if( index_details.case_prevents_index_creation )
    {
        if( throwExceptionsOnDuplicateKeys )
        {
            throw DataRepositoryException::DuplicateCaseWhileCreatingIndex("An index could not be created for a data file with duplicate case IDs, including: '%s'",
                                                                            index_details.key.c_str());
        }
    }

    else
    {
        // insert the key information into the index
        ASSERT(m_textRepository.m_stmtInsertKey != nullptr);
        SQLiteResetOnDestruction insert_key_rod(*m_textRepository.m_stmtInsertKey);

        m_textRepository.m_stmtInsertKey->Bind(1, index_details.key)
                                         .Bind(2, index_details.position)
                                         .Bind(3, index_details.bytes);

        if( m_textRepository.m_stmtInsertKey->Step() != SQLITE_DONE )
            throw DataRepositoryException::SQLiteError();
    }
}



// --------------------------------------------------------------------------
// TextRepositoryCaseIterator
// --------------------------------------------------------------------------

TextRepositoryCaseIterator::TextRepositoryCaseIterator(TextRepository& text_repository, SQLiteStatement stmt_query_keys,
                                                       const CaseIterationCaseStatus case_status, const CaseIteratorParameters* const start_parameters,
                                                       std::optional<std::tuple<size_t, size_t>> partials_offset_and_limit)
    :   m_textRepository(text_repository),
        m_stmtQueryKeys(std::move(stmt_query_keys)),
        m_notesFile(text_repository.m_notesFile.get()),
        m_statusFile(text_repository.m_statusFile.get()),
        m_progressBarParameters(case_status, CreateCopyOfPointerValue(start_parameters)),
        m_casesRead(0),
        m_partialsOffsetLimit(std::move(partials_offset_and_limit))
{
    ASSERT(m_statusFile != nullptr || !m_partialsOffsetLimit.has_value());
}


bool TextRepositoryCaseIterator::Step()
{
    // when filtering on partials, quit out if the correct number of cases has been read
    if( m_partialsOffsetLimit.has_value() && m_casesRead == std::get<1>(*m_partialsOffsetLimit) )
        return false;

    // get the next case
    while( m_stmtQueryKeys.Step() == SQLITE_ROW )
    {
        // potentially filter on partials
        if( m_partialsOffsetLimit.has_value() )
        {
            if( !m_statusFile->IsPartial(m_stmtQueryKeys.GetColumn<std::string>(0)) )
                continue;

            // process the offset
            if( std::get<0>(*m_partialsOffsetLimit) > 0 )
            {
                --std::get<0>(*m_partialsOffsetLimit);
                continue;
            }
        }

        ++m_casesRead;
        return true;
    }

    return false;
}


bool TextRepositoryCaseIterator::NextCaseKey(CaseKey& case_key)
{
    if( !Step() )
        return false;

    case_key.SetKey(m_stmtQueryKeys.GetColumn<std::string>(0));
    case_key.SetPositionInRepository(m_stmtQueryKeys.GetColumn<double>(1));

    return true;
}


bool TextRepositoryCaseIterator::NextCaseSummary(CaseSummary& case_summary)
{
    if( !NextCaseKey(case_summary) )
        return false;

    case_summary.SetDeleted(false);

    // update the case note
    if( RequiresCaseNote() && m_notesFile != nullptr )
        m_notesFile->SetUpCaseNote(case_summary);

    // update the status information
    if( m_statusFile != nullptr )
        m_statusFile->SetUpCaseSummary(case_summary);

    return true;
}


bool TextRepositoryCaseIterator::NextCase(Case& data_case)
{
    if( !Step() )
        return false;

    const int64_t file_position = m_stmtQueryKeys.GetColumn<int64_t>(1);
    const size_t bytes_for_case = m_stmtQueryKeys.GetColumn<size_t>(2);

    m_textRepository.TextRepository::ReadCase(data_case, file_position, bytes_for_case);

    return true;
}


int TextRepositoryCaseIterator::GetPercentRead() const
{
    // get the number of cases if necessary
    if( !m_percentMultiplier.has_value() )
    {
        const size_t number_cases = m_textRepository.GetNumberCases(std::get<0>(m_progressBarParameters), std::get<1>(m_progressBarParameters).get());
        m_percentMultiplier = CreatePercentMultiplier(number_cases);
    }

    return static_cast<int>(m_casesRead * *m_percentMultiplier);
}



// --------------------------------------------------------------------------
// TextRepositoryBatchCaseIterator
// --------------------------------------------------------------------------

TextRepositoryBatchCaseIterator::TextRepositoryBatchCaseIterator(TextRepository& text_repository, const bool partials_only)
    :   m_textRepository(text_repository),
        m_partialsOnly(partials_only)
{
    m_textRepository.ResetPositionToBeginning();

#ifdef _DEBUG
    m_lastFileBytesRemaining = m_textRepository.m_fileBytesRemaining;
#endif
}


template<typename T>
bool TextRepositoryBatchCaseIterator::NextCaseForNonCaseReading(T& case_object)
{
    // in the rare event that the CaseKey or CaseSummary is being queried from
    // a batch iterator, create a case that can be used to access the case contents
    if( m_case == nullptr )
        m_case = m_textRepository.GetCaseAccess().CreateCase();

    if( NextCase(*m_case) )
    {
        case_object = *m_case;
        return true;
    }

    return false;
}


bool TextRepositoryBatchCaseIterator::NextCaseKey(CaseKey& case_key)
{
    return NextCaseForNonCaseReading(case_key);
}


bool TextRepositoryBatchCaseIterator::NextCaseSummary(CaseSummary& case_summary)
{
    return NextCaseForNonCaseReading(case_summary);
}


bool TextRepositoryBatchCaseIterator::NextCase(Case& data_case)
{
    while( true )
    {
        // ensure that the file position hasn't moved
#ifdef _DEBUG
        ASSERT(m_lastFileBytesRemaining == m_textRepository.m_fileBytesRemaining);
#endif

        const bool case_read = m_textRepository.ReadUntilKeyChange();

#ifdef _DEBUG
        m_lastFileBytesRemaining = m_textRepository.m_fileBytesRemaining;
#endif

        if( !case_read )
            return false;

        m_textRepository.SetUpBatchCase(data_case);

        // potentially filter on partials
        if( !m_partialsOnly || data_case.IsPartial() )
            return true;
    }
}


int TextRepositoryBatchCaseIterator::GetPercentRead() const
{
    return m_textRepository.GetPercentRead();
}
