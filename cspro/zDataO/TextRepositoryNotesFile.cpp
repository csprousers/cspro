#include "stdafx.h"
#include "TextRepositoryNotesFile.h"
#include "TextRepository.h"
#include <zToolsO/File.h>
#include <zUtilO/NameShortener.h>


TextRepositoryNotesFile::TextRepositoryNotesFile(const TextRepository& repository, const DataRepositoryOpenFlag open_flag)
    :   m_repository(repository),
        m_filePath(GetNotesFilePath(m_repository.GetConnectionString())),
        m_dictionaryName(m_repository.GetCaseAccess().GetDataDict().GetName()),
        m_hasTransactionsToWrite(false)
{
    // determine the length of the case keys
    m_firstLevelKeyLength = m_repository.m_keyMetadata->key_length;
    m_allLevelsKeyLength = 0;

    for( const CaseLevelMetadata& case_level_metadata : m_repository.GetCaseAccess().GetCaseMetadata().GetCaseLevelsMetadata() )
    {
        m_allLevelsKeyLength += case_level_metadata.GetLevelKeyLength();

        if( case_level_metadata.GetDictLevel().GetLevelNumber() > 0 )
            m_secondaryLevelKeyLengths.emplace_back(m_allLevelsKeyLength - m_firstLevelKeyLength);
    }

    if( PortableFunctions::FileIsRegular(m_filePath) )
    {
        ASSERT(open_flag != DataRepositoryOpenFlag::CreateNew);
        Load();
    }

    // if no notes file exists, see if a pre-7.0 style notes file exists
    else if( open_flag != DataRepositoryOpenFlag::CreateNew )
    {
        std::string old_file_path = PortableFunctions::PathAppendFileExtension(m_repository.GetConnectionString().GetFilePath(), FileExtensions::Old::Data::TextNotes);

        if( PortableFunctions::FileIsRegular(old_file_path) )
        {
            LoadOldFormat(std::move(old_file_path));

            if( !m_repository.IsReadOnly() )
                Save();
        }
    }
}


TextRepositoryNotesFile::~TextRepositoryNotesFile()
{
    ASSERT(!m_hasTransactionsToWrite);
}


std::string TextRepositoryNotesFile::GetNotesFilePath(const ConnectionString& connection_string)
{
     return PortableFunctions::PathAppendFileExtension(connection_string.GetFilePath(), FileExtensions::Data::TextNotes);
}


Note& TextRepositoryNotesFile::AddNote(std::string first_level_key, std::shared_ptr<NamedReference> named_reference, std::string operator_id,
                                       const int64_t modified_date_time, SharableString content)
{
    // add the note object
    if( m_notesMap == nullptr )
        m_notesMap = std::make_unique<std::map<std::string, std::vector<Note>>>();

    auto notes_search = m_notesMap->find(first_level_key);
    std::vector<Note>& notes = ( notes_search != m_notesMap->end() ) ? notes_search->second :
                                                                       m_notesMap->emplace(std::move(first_level_key), std::vector<Note>()).first->second;

    return notes.emplace_back(std::move(content), std::move(named_reference), std::move(operator_id), modified_date_time);
}


void TextRepositoryNotesFile::Load()
{
    try
    {
        FileIO::TextFile csnot_file;
        csnot_file.SetProperties(m_repository.GetConnectionString());

        try
        {
            csnot_file.OpenForTextReading(m_filePath);
        }

        catch( const CSProException& exception )
        {
            throw DataRepositoryException::IOError("There was an error opening the notes file: %s", exception.what());
        }

        // determine the minimum size of a notes line
        const size_t min_line_length = m_allLevelsKeyLength + FieldLength + OperatorIdLength +
                                       ModifiedDateLength + ModifiedTimeLength + 3 * OccurrenceLength + 1; // 1 for the note

        std::string line;

        while( csnot_file.ReadLine(line) )
        {
            // ignore lines without valid notes
            if( SO::WideLength(line) < min_line_length )
                continue;

            // ignore deleted rows
            if( line.front() == TextToCaseConverter::DataFileErasedRecordCharacter )
                continue;

            // turn ␤ -> \n
            NewlineSubstitutor::MakeUnicodeNLToNewline(line);
            ASSERT(SO::WideLength(line) >= min_line_length);

            const char* line_itr = line.data();

            auto process_wide_entity = [&](const size_t wide_length, const bool right_trim_spaces)
            {
                const size_t utf8_length = SO::WideGetOffset(line_itr, wide_length);
                std::string entity(line_itr, utf8_length);
                line_itr += utf8_length;

                return right_trim_spaces ? SO::MakeTrimRightSpace(entity) :
                                           entity;
            };

            std::string first_level_key = process_wide_entity(m_firstLevelKeyLength, false);
            std::string level_key = LoadAdjustLevelKey(process_wide_entity(m_allLevelsKeyLength - m_firstLevelKeyLength, false));

            const std::string field_name = NameShortener::Unshorten(process_wide_entity(FieldLength, true));

            std::string operator_id = process_wide_entity(OperatorIdLength, true);
            std::string modified_date = process_wide_entity(ModifiedDateLength, true);
            std::string modified_time = process_wide_entity(ModifiedTimeLength, true);

            int64_t modified_date_time = 0;

            if( modified_date.length() == ModifiedDateLength && modified_time.length() == ModifiedTimeLength )
            {
                modified_date_time = DateTime::CreateTime(atoi(modified_date.c_str()),
                                                          atoi(modified_time.c_str()));
            }

            size_t occurrences[3];

            for( size_t i = 0; i < _countof(occurrences); ++i )
            {
                const std::string occurrence = process_wide_entity(OccurrenceLength, true);
                occurrences[i] = occurrence.empty() ? 0 : std::max(atoi(occurrence.c_str()) - 1, 0);
            }

            SharableString content(line_itr);
            content.MakeTrimRight();

            AddNote(std::move(first_level_key),
                    CaseConstructionHelpers::CreateNamedReference(m_repository.GetCaseAccess(), std::move(level_key), field_name, occurrences),
                    std::move(operator_id),
                    modified_date_time,
                    std::move(content));
        }

        csnot_file.Close();
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch( const CSProException& exception )
    {
        throw DataRepositoryException::IOError("There was an error reading the notes file: %s", exception.what());
    }
}


std::string TextRepositoryNotesFile::LoadAdjustLevelKey(std::string level_key) const
{
    // make sure that the length of the trimmed level key is valid
    SO::MakeTrimRightSpace(level_key);

    if( !level_key.empty() )
    {
        for( const size_t length : m_secondaryLevelKeyLengths )
        {
            const ptrdiff_t length_difference = SO::WideLength(level_key) - length;

            // if length_difference == 0, this note belongs to this level
            if( length_difference == 0 )
            {
                break;
            }

            // if length_difference < 0, this note belongs to this level but needs to be right-padded with spaces
            else if( length_difference < 0 )
            {
                SO::WideMakeExactLength(level_key, length);
                break;
            }

            // continue processing to a future level
            else
            {
                ASSERT(length < m_secondaryLevelKeyLengths.back());
            }
        }
    }

    return level_key;
}


void TextRepositoryNotesFile::CommitTransactions()
{
    ASSERT(m_repository.m_useTransactionManager);

    if( m_hasTransactionsToWrite )
        Save(true);
}


void TextRepositoryNotesFile::Save(const bool force_write_to_disk/* = false*/)
{
    if( m_repository.m_useTransactionManager && !force_write_to_disk )
    {
        m_hasTransactionsToWrite = true;
        return;
    }

    try
    {
        static_assert(FileIO::TextFile::DefaultWriteNewlineAsCRLF == true);
        FileIO::TextFile csnot_file;
        csnot_file.SetProperties(m_repository.GetConnectionString());

        try
        {
            csnot_file.OpenForTextWritingCreate(m_filePath);
        }

        catch( const CSProException& exception )
        {
            throw DataRepositoryException::IOError("There was an error creating the notes file: %s", exception.what());
        }

        if( m_notesMap != nullptr )
        {
            const std::string formatting_string = FormatText("%%-s%%-s%%-s%%-%d.%ds%%-%d.%ds%%-%d.%ds%%-%d.%ds%%-%d.%ds%%s",
                                                             ModifiedDateLength, ModifiedDateLength,
                                                             ModifiedTimeLength, ModifiedTimeLength,
                                                             OccurrenceLength, OccurrenceLength,
                                                             OccurrenceLength, OccurrenceLength,
                                                             OccurrenceLength, OccurrenceLength);

            auto write_formatted_line = [&](std::string full_key, std::string field_name, std::string operator_id,
                                            const char* const date, const char* const time,
                                            const char* const record_occurrence, const char* const item_occurrence, const char* const subitem_occurrence,
                                            const char* const note_content)
            {
                SO::WideMakeExactLength(full_key, m_allLevelsKeyLength);
                SO::WideMakeExactLength(field_name, FieldLength);
                SO::WideMakeExactLength(operator_id, OperatorIdLength);

                ASSERT(strlen(date) <= ModifiedDateLength);
                ASSERT(strlen(time) <= ModifiedTimeLength);
                ASSERT(strlen(record_occurrence) <= OccurrenceLength);
                ASSERT(strlen(item_occurrence) <= OccurrenceLength);
                ASSERT(strlen(subitem_occurrence) <= OccurrenceLength);

                csnot_file.WriteFormattedLine(formatting_string.c_str(),
                                              full_key.c_str(), field_name.c_str(), operator_id.c_str(),
                                              date, time,
                                              record_occurrence, item_occurrence, subitem_occurrence,
                                              note_content);
            };

            // write the header
            write_formatted_line("~Case IDs", "Field Name", "Operator ID", "Date", "Time", "Rcrd#", "Item#", "Sub #", "Note");

            // write the notes
            for( const auto& [key, notes] : *m_notesMap )
            {
                for( const Note& note : notes )
                {
                    const NamedReference& named_reference = note.GetNamedReference();

                    const std::string full_key = key + named_reference.GetLevelKey();

                    std::string field_name = NameShortener::Shorten(named_reference.GetName(), FieldLength);

                    std::string date;
                    std::string time;

                    if( note.GetModifiedDateTime() > 0 )
                    {
                        const DateTime::Components date_time_components = DateTime::TimeToComponents(note.GetModifiedDateTime());
                        date = FormatText("%04d%02d%02d", date_time_components.year, date_time_components.month, date_time_components.day);
                        time = FormatText("%02d%02d%02d", date_time_components.hour, date_time_components.minute, date_time_components.second);
                    }

                    std::string occurrences[3];

                    if( named_reference.HasOccurrences() )
                    {
                        const std::vector<size_t>& one_based_occurrences = named_reference.GetOneBasedOccurrences();
                        ASSERT(one_based_occurrences.size() == _countof(occurrences));

                        for( int i = 0; i < _countof(occurrences); ++i )
                        {
                            if( one_based_occurrences[i] > 0 )
                                occurrences[i] = FormatText("%*d", OccurrenceLength, static_cast<int>(one_based_occurrences[i]));
                        }
                    }

                    write_formatted_line(NewlineSubstitutor::NewlineToUnicodeNL(full_key),
                                         std::move(field_name),
                                         NewlineSubstitutor::NewlineToUnicodeNL(note.GetOperatorId()),
                                         date.c_str(), time.c_str(),
                                         occurrences[0].c_str(), occurrences[1].c_str(), occurrences[2].c_str(),
                                         NewlineSubstitutor::NewlineToUnicodeNL(note.GetContent()).c_str());
                }
            }
        }

        csnot_file.Close();
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch( const CSProException& exception )
    {
        throw DataRepositoryException::IOError("There was an error writing to the notes file: %s", exception.what());
    }

    m_hasTransactionsToWrite = false;
}


void TextRepositoryNotesFile::LoadOldFormat(std::string file_path)
{
    try
    {
        FileIO::TextFile not_file;

        // make sure that pre-5.0 files without a BOM are read as ANSI (unless overridden in the connection string)
        not_file.SetTextEncoding(TextEncoding::Type::Ansi);
        not_file.SetProperties(m_repository.GetConnectionString());

        try
        {
            not_file.OpenForTextReading(std::move(file_path));
        }

        catch( const CSProException& exception )
        {
            throw DataRepositoryException::IOError("There was an error opening the pre-7.0 notes file: %s", exception.what());
        }

        // determine the minimum size of a notes line
        const size_t note_identifying_portion_length = m_allLevelsKeyLength + FieldLength + 17; // 17 for three occurrences
        const size_t min_length_line = note_identifying_portion_length + 1; // 1 for the note text

        std::string line;
        std::string previous_identifier; // used to combine multiline notes
        Note* previous_note = nullptr;

        while( not_file.ReadLine(line) )
        {
            // ignore lines without valid notes
            if( SO::WideLength(line) < min_length_line )
                continue;

            // potentially combine this line with the previous note in the case of multiline notes
            const std::string_view this_identifier_sv = SO::WideSubstring(line, 0, note_identifying_portion_length);

            if( previous_note != nullptr && previous_identifier == this_identifier_sv )
            {
                std::string content_continuation = SO::Concatenate(previous_note->GetContent(),
                                                                   " ",
                                                                   std::string_view(line).substr(this_identifier_sv.length()));
                previous_note->SetContent(std::move(content_continuation), false);
                continue;
            }

            std::string key(SO::WideSubstring(line, 0, m_allLevelsKeyLength));
            std::string field_name(SO::Trim(SO::WideSubstring(line, m_allLevelsKeyLength, FieldLength)));
            const std::string occurrence1_text(SO::Trim(SO::WideSubstring(line, m_allLevelsKeyLength + FieldLength, 6)));
            const std::string occurrence2_text(SO::Trim(SO::WideSubstring(line, m_allLevelsKeyLength + FieldLength + 6, 5)));
            const std::string occurrence3_text(SO::Trim(SO::WideSubstring(line, m_allLevelsKeyLength + FieldLength + 12, 5)));
            std::string content(SO::Trim(SO::WideSubstring(line, note_identifying_portion_length)));

            ASSERT(occurrence1_text.empty());
            int record_occurrence = 0;
            int item_occurrence = 0;
            int subitem_occurrence = 0;

            // if the field has occurrences, figure out what kind they are; pre-7.0, the only kind that worked were:
            //      1 - singly occurring record     multiply occurring item  _ _ I
            //      2 - multiply occurring record   singly occurring item    _ _ R
            //      3 - multiply occurring record   multiply occurring item  _ R.I
            if( !occurrence3_text.empty() )
            {
                const CaseItem* const case_item = m_repository.GetCaseAccess().LookupCaseItem(field_name);

                if( case_item == nullptr ) // the field is not in the dictionary
                    continue;

                const CDictItem& dict_item = case_item->GetDictItem();
                const CDictRecord& dict_record = *dict_item.GetRecord();

                const int occurrence3 = atoi(occurrence3_text.c_str());

                if( dict_item.GetOccurs() > 1 && dict_record.GetMaxRecs() == 1 ) // case 1
                {
                    ASSERT(occurrence2_text.empty());
                    item_occurrence = occurrence3;
                }

                else if( dict_item.GetOccurs() == 1 && dict_record.GetMaxRecs() > 1 ) // case 2
                {
                    ASSERT(occurrence2_text.empty());
                    record_occurrence = occurrence3;
                }

                else if( dict_item.GetOccurs() > 1 && dict_record.GetMaxRecs() > 1 ) // case 3
                {
                    ASSERT(!occurrence2_text.empty());
                    item_occurrence = occurrence3;
                    record_occurrence = atoi(occurrence2_text.c_str());
                }

                else
                {
                    ASSERT(false);
                }
            }

            // construct the note object
            std::string first_level_key(SO::WideSubstring(key, 0, m_firstLevelKeyLength));
            std::string level_key = LoadAdjustLevelKey(key.substr(first_level_key.length()));

            const size_t occurrences[3] =
            {
                static_cast<size_t>(std::max(record_occurrence - 1, 0)),
                static_cast<size_t>(std::max(item_occurrence - 1, 0)),
                static_cast<size_t>(std::max(subitem_occurrence - 1, 0))
            };

            previous_note = &AddNote(std::move(first_level_key),
                                     CaseConstructionHelpers::CreateNamedReference(m_repository.GetCaseAccess(), std::move(level_key), field_name, occurrences),
                                     std::string(),
                                     0,
                                     std::move(content));

            previous_identifier = this_identifier_sv;
        }

        not_file.Close();
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch( const CSProException& exception )
    {
        throw DataRepositoryException::IOError("There was an error reading the pre-7.0 notes file: %s", exception.what());
    }
}


void TextRepositoryNotesFile::WriteCase(Case& data_case, WriteCaseParameter* const write_case_parameter)
{
    const std::string& key = data_case.GetKey();
    const std::vector<Note>& new_notes = data_case.GetNotes();
    bool modified = false;

    // process case modifications
    if( write_case_parameter != nullptr && write_case_parameter->IsModifyParameter() )
    {
        if( m_notesMap != nullptr )
        {
            // remove the previous notes if the key changed
            if( write_case_parameter->GetKey() != key )
            {
                modified = RemoveEntry(write_case_parameter->GetKey());
            }

            // if there are no notes, remove any existing entries
            else if( new_notes.empty() )
            {
                modified = RemoveEntry(key);
            }
        }

        // quit out if the notes didn't change
        if( !modified && !write_case_parameter->AreNotesModified() )
            return;
    }

    // add any notes
    if( !new_notes.empty() )
    {
        if( m_notesMap == nullptr )
            m_notesMap = std::make_unique<std::map<std::string, std::vector<Note>>>();

        (*m_notesMap)[key] = new_notes;
        modified = true;
    }

    if( modified )
        Save();
}


bool TextRepositoryNotesFile::RemoveEntry(const std::string& key)
{
    ASSERT(m_notesMap != nullptr);

    if( m_notesMap->erase(key) > 0 )
    {
        // when there are no entries, delete the notes map
        if( m_notesMap->empty() )
            m_notesMap.reset();

        return true;
    }

    return false;
}


void TextRepositoryNotesFile::DeleteCase(const std::string& key)
{
    if( m_notesMap != nullptr && RemoveEntry(key) )
        Save();
}
