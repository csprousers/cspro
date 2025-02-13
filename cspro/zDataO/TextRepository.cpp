#include "stdafx.h"
#include "TextRepository.h"
#include "NullRepositoryIterators.h"
#include "TextRepositoryIndexCreator.h"
#include "TextRepositoryIterators.h"
#include "TextRepositoryNotesFile.h"
#include "TextRepositoryStatusFile.h"
#include <SQLite/SQLiteHelpers.h>
#include <zUtilO/StdioFileUnicode.h>


namespace Constants
{
    constexpr size_t TextBufferSize    = 128 * 1024;
    constexpr const TCHAR* TruncationIOError = _T("There was an error truncating the file.");
}


namespace SqlStatements
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


TextRepository::TextRepository(std::shared_ptr<const CaseAccess> case_access, DataRepositoryAccess access_type)
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


void TextRepository::Open(DataRepositoryOpenFlag open_flag)
{
    const bool can_create_file = ( open_flag == DataRepositoryOpenFlag::CreateNew || open_flag == DataRepositoryOpenFlag::OpenOrCreate );

    if( can_create_file && !PortableFunctions::PathMakeDirectories(PortableFunctions::PathGetDirectory(m_connectionString.GetFilename())) )
    {
        throw DataRepositoryException::IOError(FormatText(_T("The directory does not exist and could not be created: %s"),
                                                          PortableFunctions::PathGetDirectory(m_connectionString.GetFilename()).c_str()));
    }

    const bool file_exists = PortableFunctions::FileIsRegular(m_connectionString.GetFilename());

    if( open_flag == DataRepositoryOpenFlag::OpenMustExist && !file_exists )
    {
        throw DataRepositoryException::IOError(FormatText(_T("The data file does not exist: %s"), m_connectionString.GetFilename().c_str()));
    }

    // create a data file if one doesn't exist, if setfile/open used the clear flag,
    // or if opening in batch output mode, to overwrite what is already on the disk
    const bool create_new_file = ( !file_exists || open_flag == DataRepositoryOpenFlag::CreateNew || m_accessType == DataRepositoryAccess::BatchOutput );

    if( create_new_file )
    {
        DeleteRepositoryFiles(m_connectionString);

        FILE* file = PortableFunctions::FileOpen(m_connectionString.GetFilename(), _T("wb"));
        bool success = ( file != nullptr );

        if( success )
        {
            // write out the UTF-8 BOM
            success = ( fwrite(Utf8BOM_sv.data(), 1, Utf8BOM_sv.length(), file) == Utf8BOM_sv.length() );
            fclose(file);
        }

        if( !success )
            throw DataRepositoryException::IOError(_T("Could not create a new data file."));
    }


    // check the encoding
    if( !GetFileBOM(m_connectionString.GetFilename(), m_encoding) )
        throw DataRepositoryException::IOError(_T("Could not read the data file's encoding. The file may be open in another program."));

    // if the data file is not UTF-8 but it is writeable, then we need to rewrite it as a UTF-8 file
    if( m_encoding == Encoding::Ansi && !IsReadOnly() )
    {
        if( !CStdioFileUnicode::ConvertAnsiToUTF8(m_connectionString.GetFilename()) )
            throw DataRepositoryException::IOError(_T("Could not convert the writeable data file from ANSI to UTF-8."));

        m_encoding = Encoding::Utf8;
    }

    else if( m_encoding != Encoding::Utf8 && m_encoding != Encoding::Ansi )
    {
        throw DataRepositoryException::IOError(_T("CSPro does not support the specified text encoding."));
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
    auto delete_file = [](const std::wstring& filename)
    {
        if( PortableFunctions::FileIsRegular(filename) && !PortableFunctions::FileDelete(filename) )
            throw DataRepositoryException::DeleteRepositoryError();
    };

    // delete the data, index, notes, and status files
    delete_file(connection_string.GetFilename());
    delete_file(connection_string.GetFilename() + FileExtensions::Data::WithDot::IndexableTextIndex);
    delete_file(TextRepositoryNotesFile::GetNotesFilename(connection_string));
    delete_file(TextRepositoryStatusFile::GetStatusFilename(connection_string));
}


void TextRepository::RenameRepository(const ConnectionString& old_connection_string, const ConnectionString& new_connection_string)
{
    DeleteRepositoryFiles(new_connection_string);

    auto rename_file = [](const std::wstring& old_filename, const std::wstring& new_filename)
    {
        if( PortableFunctions::FileIsRegular(old_filename) && !PortableFunctions::FileRename(old_filename, new_filename) )
            throw DataRepositoryException::RenameRepositoryError();
    };

    // rename the data, index, notes, and status files
    rename_file(old_connection_string.GetFilename(), new_connection_string.GetFilename());
    rename_file(old_connection_string.GetFilename() + FileExtensions::Data::WithDot::IndexableTextIndex, new_connection_string.GetFilename() + FileExtensions::Data::WithDot::IndexableTextIndex);
    rename_file(TextRepositoryNotesFile::GetNotesFilename(old_connection_string), TextRepositoryNotesFile::GetNotesFilename(new_connection_string));
    rename_file(TextRepositoryStatusFile::GetStatusFilename(old_connection_string), TextRepositoryStatusFile::GetStatusFilename(new_connection_string));
}


std::vector<std::wstring> TextRepository::GetAssociatedFileList(const ConnectionString& connection_string)
{
    return
    {
        connection_string.GetFilename(),
        TextRepositoryNotesFile::GetNotesFilename(connection_string),
        TextRepositoryStatusFile::GetStatusFilename(connection_string)
    };
}


void TextRepository::OpenDataFile()
{
    m_file = PortableFunctions::FileOpen(m_connectionString.GetFilename(), IsReadOnly() ? _T("rb") : _T("rb+"));

    if( m_file == nullptr )
        throw DataRepositoryException::IOError(_T("The data file could not be opened."));

    // create memory for the read and write buffers
    if( m_utf8AnsiBuffer == nullptr )
        m_utf8AnsiBuffer = new char[m_utf8AnsiBufferSize];

    if( m_wideBuffer == nullptr )
        m_wideBuffer = new TCHAR[m_wideBufferSize];

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


size_t TextRepository::GetIdStructureHashForKeyIndex() const
{
    return m_caseAccess->GetDataDict().GetIdStructureHashForKeyIndex(false, true);
}


std::shared_ptr<IndexableTextRepository::IndexCreator> TextRepository::GetIndexCreator()
{
    static std::vector<const char*> CreateIndexSqlStatements = { SqlStatements::CreateKeyTablePositionIndex };
    m_indexCreator = std::make_shared<TextRepositoryIndexCreator>(*this, SqlStatements::CreateKeyTable, CreateIndexSqlStatements);
    return m_indexCreator;
}


std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>> TextRepository::GetSqlStatementsToPrepare()
{
    return std::vector<std::tuple<const char*, std::shared_ptr<SQLiteStatement>&>>
    {
        { SqlStatements::InsertKey, m_stmtInsertKey },
        { SqlStatements::KeyExists, m_stmtKeyExists }
    };
}


std::variant<const char*, std::shared_ptr<SQLiteStatement>> TextRepository::GetSqlStatementForQuery(SqlQueryType type)
{
    switch( type )
    {
        case SqlQueryType::ContainsNotDeletedKey:             return m_stmtKeyExists;
        case SqlQueryType::GetPositionBytesFromNotDeletedKey: return SqlStatements::QueryPositionBytesByKey;
        case SqlQueryType::GetBytesFromPosition:              return SqlStatements::QueryBytesByPosition;
        case SqlQueryType::CountNotDeletedKeys:               return SqlStatements::CountKeys;
    }

    throw ProgrammingErrorException();
}


void TextRepository::ResetPosition(int64_t file_position)
{
    ASSERT(file_position >= 0 && file_position <= m_fileSize);
    PortableFunctions::fseeki64(m_file, file_position, SEEK_SET);
}


void TextRepository::ResetPositionToBeginning()
{
    const size_t file_position = ( m_encoding == Encoding::Utf8 ) ? Utf8BOM_sv.length() : 0;
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
    const TCHAR* first_line_key = nullptr;

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

    auto this_line_key_matches_first_line_key = [&](const TCHAR* buffer, size_t buffer_length) -> bool
    {
        const TCHAR* const this_line_key = m_currentLineKeyFullLineProcessor.GetLine(buffer, buffer_length, m_keyEnd);

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
                    const TCHAR last_char = *(m_wideBufferPosition - 1);

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

                    TCHAR* new_wide_buffer = new TCHAR[m_wideBufferSize];
                    _tmemcpy(new_wide_buffer, m_wideBuffer, buffer_size_currently_used);
                    delete[] m_wideBuffer;

                    m_wideBuffer = new_wide_buffer;

                    force_wide_buffer_resize_to_accommodate_utf8_characters = false;
                }

                // otherwise move any lines in the wide buffer that are currently are part of this
                // case and adjust the line offsets
                else
                {
                    memmove(m_wideBuffer, m_wideBuffer + this_case_buffer_offset, sizeof(TCHAR) * buffer_size_currently_used);

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
                                                                                                 const_cast<TCHAR*>(m_wideBufferPosition), remaining_buffer_size);

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
                    const TCHAR* current_wide_buffer_line_start_position = m_wideBuffer + current_wide_buffer_line->offset;

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


void TextRepository::PopulateCaseIdentifiers(CString& key, CString& uuid, double& position_in_repository) const
{
    // search the index by key
    if( !key.IsEmpty() )
    {
        position_in_repository = static_cast<double>(std::get<int64_t>(GetPositionBytesFromKey(key)));
    }

    // text files don't have UUIDs
    else if( !uuid.IsEmpty() )
    {
        throw DataRepositoryException::CaseNotFound();
    }

    // or by file position
    else
    {
        key = GetKeyFromPosition(static_cast<int64_t>(position_in_repository));
    }

    ASSERT(uuid.IsEmpty());
}


CString TextRepository::GetKeyFromPosition(int64_t file_position) const
{
    EnsureSqlStatementIsPrepared(SqlStatements::QueryKeyByPosition, m_stmtQueryKeyByPosition);
    SQLiteResetOnDestruction rod(*m_stmtQueryKeyByPosition);

    m_stmtQueryKeyByPosition->Bind(1, file_position);

    if( m_stmtQueryKeyByPosition->Step() != SQLITE_ROW )
        throw DataRepositoryException::CaseNotFound();

    return m_stmtQueryKeyByPosition->GetColumn<CString>(0);
}


SQLiteStatement TextRepository::CreateKeySearchIteratorStatement(const TCHAR* columns_to_query,
                                                                 size_t offset, size_t limit,
                                                                 const std::optional<CaseIterationMethod>& iteration_method,
                                                                 const std::optional<CaseIterationOrder>& iteration_order,
                                                                 const CaseIteratorParameters* start_parameters) const
{
    CString order_by_text;

    if( iteration_method.has_value() )
    {
        order_by_text.Format(_T("ORDER BY %s %s "),
            ( iteration_method == CaseIterationMethod::KeyOrder ) ? _T("`Key`") : _T("`Position`"),
            ( ( iteration_order == CaseIterationOrder::Ascending )  ? _T("ASC") :
              ( iteration_order == CaseIterationOrder::Descending ) ? _T("DESC") :
                                                                      _T("") ));
    }

    const CString limit_text = FormatText(_T("LIMIT %d OFFSET %d "), ( limit == SIZE_MAX ) ? -1 : static_cast<int>(limit), static_cast<int>(offset));

    CString where_text;
    bool use_key_prefix = false;
    bool use_operators = false;

    // process any filters
    if( start_parameters != nullptr )
    {
        // use the key prefix if it is set and and is not empty
        if( start_parameters->key_prefix.has_value() && !start_parameters->key_prefix->IsEmpty() )
        {
            use_key_prefix = true;
            where_text = _T("WHERE `Key` >= ? AND `Key` < ?");

            use_operators = std::holds_alternative<CString>(start_parameters->first_key_or_position) ?
                !std::get<CString>(start_parameters->first_key_or_position).IsEmpty() :
                ( std::get<double>(start_parameters->first_key_or_position) != -1 );
        }

        else
        {
            use_operators = true;
        }

        if( use_operators )
        {
            const TCHAR* const comparison_operator =
                ( start_parameters->start_type == CaseIterationStartType::LessThan )          ?   _T("<") :
                ( start_parameters->start_type == CaseIterationStartType::LessThanEquals )    ?   _T("<=") :
                ( start_parameters->start_type == CaseIterationStartType::GreaterThanEquals ) ?   _T(">=") :
              /*( start_parameters->start_type == CaseIterationStartType::GreaterThan )       ?*/ _T(">");

            where_text.AppendFormat(where_text.IsEmpty() ? _T("WHERE %s %s ?") : _T(" AND %s %s ?"),
                std::holds_alternative<CString>(start_parameters->first_key_or_position) ? _T("`Key`") : _T("`Position`"), comparison_operator);
        }
    }

    const CString sql = FormatText(_T("SELECT %s FROM `Keys` %s %s %s;"), columns_to_query,
                                                                          where_text.GetString(),
                                                                          order_by_text.GetString(),
                                                                          limit_text.GetString());

    SQLiteStatement stmt_query_keys = PrepareSqlStatementForQuery(sql);

    if( use_key_prefix )
    {
        const std::string key_prefix = UTF8Convert::WideToUTF8(*start_parameters->key_prefix);
        stmt_query_keys.Bind(1, key_prefix)
                       .Bind(2, SQLiteHelpers::GetTextPrefixBoundary(key_prefix));
    }

    if( use_operators )
    {
        const int operator_argument_index = use_key_prefix ? 3 : 1;

        if( std::holds_alternative<CString>(start_parameters->first_key_or_position) )
        {
            stmt_query_keys.Bind(operator_argument_index, std::get<CString>(start_parameters->first_key_or_position));
        }

        else
        {
            stmt_query_keys.Bind(operator_argument_index, static_cast<int64_t>(std::get<double>(start_parameters->first_key_or_position)));
        }
    }

    return stmt_query_keys;
}


std::optional<CaseKey> TextRepository::FindCaseKey(CaseIterationMethod iteration_method, CaseIterationOrder iteration_order,
                                                   const CaseIteratorParameters* start_parameters/* = nullptr*/) const
{
    SQLiteStatement stmt_query_keys = CreateKeySearchIteratorStatement(_T("`Key`, `Position`"), 0, 1, iteration_method, iteration_order, start_parameters);

    if( stmt_query_keys.Step() == SQLITE_ROW )
        return CaseKey(stmt_query_keys.GetColumn<CString>(0), stmt_query_keys.GetColumn<double>(1));

    return std::nullopt;
}


void TextRepository::FillUtf8AnsiTextBufferForCaseReading(int64_t file_position, size_t bytes_for_case)
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


void TextRepository::SetupOtherCaseAttributes(Case& data_case, double file_position) const
{
    data_case.SetPositionInRepository(file_position);
    data_case.SetUuid(std::wstring());
    data_case.SetDeleted(false);

    // update the notes
    if( m_notesFile != nullptr )
        m_notesFile->SetupCase(data_case);

    // update the status information
    if( m_statusFile != nullptr )
        m_statusFile->SetupCase(data_case);

    data_case.GetVectorClock().clear();
}


void TextRepository::ReadCase(Case& data_case, int64_t file_position, size_t bytes_for_case)
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
            m_wideBuffer = new TCHAR[bytes_for_case];
            m_wideBufferSize = bytes_for_case;
        }

        const size_t characters_converted = UTF8Convert::EncodedCharsBufferToWideBuffer(m_encoding,
                                                                                        m_utf8AnsiBuffer, bytes_for_case,
                                                                                        m_wideBuffer, m_wideBufferSize);

        m_textToCaseConverter->TextWideToCase(data_case, m_wideBuffer, characters_converted);
    }

    SetupOtherCaseAttributes(data_case, static_cast<double>(file_position));
}


void TextRepository::SetupBatchCase(Case& data_case)
{
    const auto& line_iterator_begin = m_wideBufferLines.cbegin() + m_wideBufferLineCaseStartLineIndex;
    const auto& line_iterator_end = m_wideBufferLines.cend() - 1;

    m_textToCaseConverter->TextWideToCase(data_case, m_wideBuffer, line_iterator_begin, line_iterator_end);

    SetupOtherCaseAttributes(data_case, -1);
}


void TextRepository::WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter/* = nullptr*/)
{
    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    // update the notes and statues
    if( m_notesFile != nullptr )
        m_notesFile->WriteCase(data_case, write_case_parameter);

    if( m_statusFile != nullptr )
        m_statusFile->WriteCase(data_case, write_case_parameter);

    size_t output_text_length;
    const char* output_text = m_textToCaseConverter->CaseToTextUtf8(data_case, &output_text_length);

    // quickly write out the case and get out (for batch processing)
    if( m_accessType == DataRepositoryAccess::BatchOutput || m_accessType == DataRepositoryAccess::BatchOutputAppend )
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
    const CString this_key = data_case.GetKey();
    CString key_to_search;

    // this will be from CSEntry
    if( write_case_parameter != nullptr )
    {
        if( write_case_parameter->IsModifyParameter() )
        {
            key_to_search = write_case_parameter->GetKey();
        }

        else
        {
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
            if( need_to_update_index || this_key.Compare(key_to_search) != 0 )
            {
                if( m_useTransactionManager )
                    WrapInTransaction();

                EnsureSqlStatementIsPrepared(SqlStatements::ModifyKey, m_stmtModifyKey);
                SQLiteResetOnDestruction rod(*m_stmtModifyKey);

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
    int64_t write_position = ( write_method == WriteMethod::EndOfFile ) ? m_fileSize :
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
        SQLiteResetOnDestruction rod(*m_stmtInsertKey);

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


void TextRepository::DeleteCase(int64_t file_position, size_t bytes_for_case, bool deleted, const CString* key_if_known)
{
    ASSERT(deleted);

    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    // look up the key if needed for updating the notes or status files
    std::unique_ptr<CString> key_lookup;

    if( key_if_known == nullptr && ( m_notesFile != nullptr || m_statusFile != nullptr ) )
    {
        key_lookup = std::make_unique<CString>(GetKeyFromPosition(file_position));
        key_if_known = key_lookup.get();
    }

    // check if this is the last case in the file
    EnsureSqlStatementIsPrepared(SqlStatements::QueryIsLastPosition, m_stmtQueryIsLastPosition);
    SQLiteResetOnDestruction last_position_rod(*m_stmtQueryIsLastPosition);

    m_stmtQueryIsLastPosition->Bind(1, file_position);

    const bool is_last_case = ( m_stmtQueryIsLastPosition->Step() != SQLITE_ROW );

    // update the index to remove the case
    if( m_useTransactionManager )
        WrapInTransaction();

    EnsureSqlStatementIsPrepared(SqlStatements::DeleteKeyByPosition, m_stmtDeleteKeyByPosition);
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
        CString case_to_combine_key;
        int64_t case_to_combine_file_position;
        size_t case_to_combine_bytes_for_case;

        auto process_statement = [&](SQLiteStatement& stmt_query_key_by_position)
        {
            case_to_combine_key = stmt_query_key_by_position.GetColumn<CString>(0);
            case_to_combine_file_position = stmt_query_key_by_position.GetColumn<int64_t>(1);
            case_to_combine_bytes_for_case = stmt_query_key_by_position.GetColumn<size_t>(2);
        };

        // check if there is an earlier case to combine with
        EnsureSqlStatementIsPrepared(SqlStatements::QueryPreviousKeyByPosition, m_stmtQueryPreviousKeyByPosition);
        SQLiteResetOnDestruction previous_key_rod(*m_stmtQueryPreviousKeyByPosition);

        m_stmtQueryPreviousKeyByPosition->Bind(1, file_position);

        if( m_stmtQueryPreviousKeyByPosition->Step() == SQLITE_ROW )
        {
            process_statement(*m_stmtQueryPreviousKeyByPosition);
        }

        // if not, there must be a later case
        else
        {
            EnsureSqlStatementIsPrepared(SqlStatements::QueryNextKeyByPosition, m_stmtQueryNextKeyByPosition);
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

        EnsureSqlStatementIsPrepared(SqlStatements::ModifyKey, m_stmtModifyKey);
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
    if( m_notesFile != nullptr )
        m_notesFile->DeleteCase(*key_if_known);

    if( m_statusFile != nullptr )
        m_statusFile->DeleteCase(*key_if_known);
}


void TextRepository::DeleteCaseInPlace(int64_t file_position, size_t bytes_for_case)
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


void TextRepository::GrowOrShrinkFileAndIndex(int64_t file_position, int bytes_differential)
{
    int64_t bytes_to_shift = m_fileSize - file_position;

    // shrinking the file
    if( bytes_differential < 0 )
    {
        int64_t next_read_position = file_position;

        while( bytes_to_shift > 0 )
        {
            size_t bytes_to_read = std::min(m_utf8AnsiBufferSize, static_cast<size_t>(bytes_to_shift));

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
            size_t bytes_to_read = std::min(m_utf8AnsiBufferSize, static_cast<size_t>(bytes_to_shift));

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

    EnsureSqlStatementIsPrepared(SqlStatements::ShiftKeys, m_stmtShiftKeys);
    SQLiteResetOnDestruction rod(*m_stmtShiftKeys);

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

    if( m_numberTransactions > 0 || sqlite3_exec(m_db, SqlStatements::BeginTransaction, nullptr, nullptr, nullptr) == SQLITE_OK )
        ++m_numberTransactions;
}


bool TextRepository::CommitTransactions()
{
    ASSERT(m_useTransactionManager);

    if( m_numberTransactions > 0 )
    {
        if( sqlite3_exec(m_db, SqlStatements::EndTransaction, nullptr, nullptr, nullptr) != SQLITE_OK )
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


size_t TextRepository::GetNumberCases(CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters/* = nullptr*/) const
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
    size_t number_cases = 0;

    // if not filtering on partial saves, we can calculate the number easily
    if( !partials_only )
    {
        SQLiteStatement stmt_query_keys = CreateKeySearchIteratorStatement(_T("COUNT(*)"), 0, SIZE_MAX, std::nullopt, std::nullopt, start_parameters);

        if( stmt_query_keys.Step() == SQLITE_ROW )
            number_cases = stmt_query_keys.GetColumn<size_t>(0);
    }

    // otherwise, calculate the number by iterating through all the case keys
    else
    {
        CaseKey case_key;
        auto case_key_iterator = const_cast<TextRepository*>(this)->CreateIterator(CaseIterationContent::CaseKey,
                                                                                   case_status, std::nullopt, std::nullopt, start_parameters);

        while( case_key_iterator->NextCaseKey(case_key) )
            ++number_cases;
    }

    return number_cases;
}


std::unique_ptr<CaseIterator> TextRepository::CreateIterator(CaseIterationContent iteration_content, CaseIterationCaseStatus case_status,
                                                             std::optional<CaseIterationMethod> iteration_method, std::optional<CaseIterationOrder> iteration_order,
                                                             const CaseIteratorParameters* start_parameters/* = nullptr*/, size_t offset/* = 0*/, size_t limit/* = SIZE_MAX*/)
{
    const bool partials_only = ( case_status == CaseIterationCaseStatus::PartialsOnly );

    // text repositories don't have duplicates; also if no partials exist, return a null iterator
    if( ( case_status == CaseIterationCaseStatus::DuplicatesOnly ) ||
        ( partials_only && ( m_statusFile == nullptr || !m_statusFile->ContainsPartials() ) ) )
    {
        return std::make_unique<NullRepositoryCaseIterator>();
    }

    // we can use a fast batch iterator if reading cases in file order (and not using an offset or limit)
    if( iteration_content == CaseIterationContent::Case && iteration_method == CaseIterationMethod::SequentialOrder &&
        iteration_order == CaseIterationOrder::Ascending && start_parameters == nullptr && offset == 0 && limit == SIZE_MAX
        // CR_TODO_ITERATOR eventually remove the next line, but for now, this fast iterator can't be used
        // until we set the repo position to something other than -1 in SetupBatchCase
        && !m_requiresIndex )
    {
        return std::make_unique<TextRepositoryBatchCaseIterator>(*this, partials_only);
    }

    else
    {
        // partials have to be filtered by the iterator; otherwise, the offset and limit can be used
        SQLiteStatement stmt_query_keys = CreateKeySearchIteratorStatement(_T("`Key`, `Position`, `Bytes`"),
                                                                           partials_only ? 0 : offset, partials_only ? SIZE_MAX : limit,
                                                                           iteration_method, iteration_order, start_parameters);

        return std::make_unique<TextRepositoryCaseIterator>(*this, std::move(stmt_query_keys), case_status, start_parameters,
                                                            partials_only ? std::make_optional(std::make_tuple(offset, limit)) : std::nullopt);
    }
}
