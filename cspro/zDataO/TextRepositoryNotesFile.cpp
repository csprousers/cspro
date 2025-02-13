#include "stdafx.h"
#include "TextRepositoryNotesFile.h"
#include "TextRepository.h"
#include <zUtilO/Specfile.h>
#include <zUtilO/NameShortener.h>

#pragma warning(disable:4996) // hide secure warnings related to the use of _timezone


namespace
{
    constexpr int TextRepositoryFieldLength = 32;
    constexpr int TextRepositoryOperatorIdLength = TextRepositoryFieldLength;
    constexpr int TextRepositoryModifiedDateLength = 8;
    constexpr int TextRepositoryModifiedTimeLength = 6;
    constexpr int TextRepositoryOccurrenceLength = 5;
}


TextRepositoryNotesFile::TextRepositoryNotesFile(const TextRepository& repository, DataRepositoryOpenFlag open_flag)
    :   m_filename(GetNotesFilename(repository.GetConnectionString())),
        m_dictionaryName(repository.GetCaseAccess()->GetDataDict().GetName()),
        m_useTransactionManager(repository.m_useTransactionManager),
        m_hasTransactionsToWrite(false)
{
    // determine the length of the case keys
    m_firstLevelKeyLength = repository.m_keyMetadata->key_length;
    m_allLevelsKeyLength = 0;

    for( const CaseLevelMetadata* case_level_metadata : repository.GetCaseAccess()->GetCaseMetadata().GetCaseLevelsMetadata() )
    {
        m_allLevelsKeyLength += case_level_metadata->GetLevelKeyLength();

        if( case_level_metadata->GetDictLevel().GetLevelNumber() > 0 )
            m_secondaryLevelKeyLengths.emplace_back(m_allLevelsKeyLength - m_firstLevelKeyLength);
    }

    if( PortableFunctions::FileIsRegular(m_filename) )
    {
        ASSERT(open_flag != DataRepositoryOpenFlag::CreateNew);
        Load(repository);
    }

    // if no notes file exists, see if a pre-7.0 style notes file exists
    else if( open_flag != DataRepositoryOpenFlag::CreateNew )
    {
        std::wstring old_filename = repository.GetConnectionString().GetFilename() + FileExtensions::Old::Data::WithDot::TextNotes;

        if( PortableFunctions::FileIsRegular(old_filename) )
        {
            LoadOldFormat(repository, old_filename);
            Save();
        }
    }
}


TextRepositoryNotesFile::~TextRepositoryNotesFile()
{
    ASSERT(!m_hasTransactionsToWrite);
}


std::wstring TextRepositoryNotesFile::GetNotesFilename(const ConnectionString& connection_string)
{
     return connection_string.GetFilename() + FileExtensions::Data::WithDot::TextNotes;
}


Note& TextRepositoryNotesFile::AddNote(const CString& first_level_key, std::shared_ptr<NamedReference> named_reference, const CString& operator_id,
                                       const time_t& modified_date_time, const CString& content)
{
    // add the note object
    if( m_notesMap == nullptr )
        m_notesMap = std::make_unique<std::map<CString, std::vector<Note>>>();

    auto notes_search = m_notesMap->find(first_level_key);
    std::vector<Note>& notes = ( notes_search != m_notesMap->end() ) ? notes_search->second :
                                                                       m_notesMap->emplace(first_level_key, std::vector<Note>()).first->second;

    return notes.emplace_back(content, std::move(named_reference), operator_id, modified_date_time);
}


void TextRepositoryNotesFile::Load(const TextRepository& repository)
{
    try
    {
        CSpecFile csnot_file;

        if( !csnot_file.Open(m_filename.c_str(), CFile::modeRead) )
            throw DataRepositoryException::IOError(_T("There was an error opening the notes file."));

        // determine the minimum size of a notes line
        const int min_line_length = m_allLevelsKeyLength + TextRepositoryFieldLength + TextRepositoryOperatorIdLength +
                                    TextRepositoryModifiedDateLength + TextRepositoryModifiedTimeLength + 3 * TextRepositoryOccurrenceLength + 1; // 1 for the note

        CString line;

        while( csnot_file.ReadString(line) )
        {
            // ignore lines without valid notes
            if( line.GetLength() < min_line_length )
                continue;

            // ignored deleted rows
            if( line[0] == _T('~') ) 
                continue;

            // turn ␤ -> \n
            NewlineSubstitutor::MakeUnicodeNLToNewline(line);

            CString first_level_key = line.Left(m_firstLevelKeyLength);
            CIMSAString level_key = line.Mid(m_firstLevelKeyLength, m_allLevelsKeyLength - m_firstLevelKeyLength);

            // make sure that the length of the trimmed level key is valid
            level_key.TrimRight();

            if( !level_key.IsEmpty() )
            {
                for( const size_t length : m_secondaryLevelKeyLengths )
                {
                    if( level_key.GetLength() <= static_cast<int>(length) )
                    {
                        level_key.MakeExactLength(length);
                        break;
                    }
                }
            }

            int current_pos = m_allLevelsKeyLength;

            CString field_name = line.Mid(current_pos, TextRepositoryFieldLength).Trim();
            field_name = CSProNameShortener::UnicodeToCSPro(field_name);
            current_pos += TextRepositoryFieldLength;

            CString operator_id = line.Mid(current_pos, TextRepositoryOperatorIdLength).Trim();
            current_pos += TextRepositoryOperatorIdLength;

            CString modified_date = line.Mid(current_pos, TextRepositoryModifiedDateLength).Trim();
            current_pos += TextRepositoryModifiedDateLength;

            CString modified_time = line.Mid(current_pos, TextRepositoryModifiedTimeLength).Trim();
            current_pos += TextRepositoryModifiedTimeLength;

            time_t modified_date_time = 0;

            if( modified_date.GetLength() == TextRepositoryModifiedDateLength && modified_time.GetLength() == TextRepositoryModifiedTimeLength )
            {
                const int yyyy_mm_dd = _ttoi(modified_date);
                const int hh_mm_ss = _ttoi(modified_time);

                std::tm tm;
                ReadableTimeToTm(&tm, yyyy_mm_dd, hh_mm_ss);

                modified_date_time = mktime(&tm) - _timezone;
            }

            size_t occurrences[3];

            for( size_t i = 0; i < _countof(occurrences); ++i )
            {
                CString occurrence = line.Mid(current_pos, TextRepositoryOccurrenceLength).Trim();
                current_pos += TextRepositoryOccurrenceLength;

                occurrences[i] = occurrence.IsEmpty() ? 0 : std::max(_ttoi(occurrence) - 1, 0);
            }

            CString content = line.Mid(current_pos).Trim();

            std::shared_ptr<NamedReference> named_reference = CaseConstructionHelpers::CreateNamedReference(*repository.GetCaseAccess(), level_key, field_name, occurrences);
            AddNote(first_level_key, std::move(named_reference), operator_id, modified_date_time, content);
        }

        csnot_file.Close();
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch(...)
    {
        throw DataRepositoryException::IOError(_T("There was an error reading the notes file."));
    }
}


void TextRepositoryNotesFile::CommitTransactions()
{
    ASSERT(m_useTransactionManager);

    if( m_hasTransactionsToWrite )
        Save(true);
}


void TextRepositoryNotesFile::Save(bool force_write_to_disk/* = false*/)
{
    if( m_useTransactionManager && !force_write_to_disk )
    {
        m_hasTransactionsToWrite = true;
        return;
    }

    try
    {
        CSpecFile csnot_file;

        if( !csnot_file.Open(m_filename.c_str(), CFile::modeWrite) )
            throw DataRepositoryException::IOError(_T("There was an error creating the notes file."));

        if( m_notesMap != nullptr )
        {
            const CString formatting_string = FormatText(_T("%%-%d.%ds%%-%d.%ds%%-%d.%ds%%-%d.%ds%%-%d.%ds%%-%d.%ds%%-%d.%ds%%-%d.%ds%%s"),
                                                         m_allLevelsKeyLength, m_allLevelsKeyLength,
                                                         TextRepositoryFieldLength, TextRepositoryFieldLength,
                                                         TextRepositoryOperatorIdLength, TextRepositoryOperatorIdLength,
                                                         TextRepositoryModifiedDateLength, TextRepositoryModifiedDateLength,
                                                         TextRepositoryModifiedTimeLength, TextRepositoryModifiedTimeLength,
                                                         TextRepositoryOccurrenceLength, TextRepositoryOccurrenceLength,
                                                         TextRepositoryOccurrenceLength, TextRepositoryOccurrenceLength,
                                                         TextRepositoryOccurrenceLength, TextRepositoryOccurrenceLength);

            // write the header
            csnot_file.WriteFormattedLine(formatting_string,
                                          _T("~Case IDs"), _T("Field Name"), _T("Operator ID"),
                                          _T("Date"), _T("Time"), _T("Rcrd#"), _T("Item#"), _T("Sub #"), _T("Note"));

            // write the notes
            for( const auto& [key, notes] : *m_notesMap )
            {
                for( const Note& note : notes )
                {
                    const NamedReference& named_reference = note.GetNamedReference();

                    CString full_key = key + named_reference.GetLevelKey();

                    CString field_name = CSProNameShortener::CSProToUnicode(named_reference.GetName(), TextRepositoryFieldLength);

                    CString date;
                    CString time;

                    if( note.GetModifiedDateTime() > 0 )
                    {
                        time_t modified_date_time = note.GetModifiedDateTime();
                        std::tm* tm = gmtime(&modified_date_time);

                        int year;
                        int month;
                        int day;
                        int hour;
                        int minute;
                        int second;
                        TmToReadableTime(tm, &year, &month, &day, &hour, &minute, &second);

                        date.Format(_T("%04d%02d%02d"), year, month, day);
                        time.Format(_T("%02d%02d%02d"), hour, minute, second);
                    }

                    CString occurrences[3];

                    if( named_reference.HasOccurrences() )
                    {
                        const std::vector<size_t>& one_based_occurrences = named_reference.GetOneBasedOccurrences();
                        ASSERT(one_based_occurrences.size() == _countof(occurrences));

                        for( int i = 0; i < _countof(occurrences); ++i )
                        {
                            if( one_based_occurrences[i] > 0 )
                                occurrences[i].Format(_T("%*d"), TextRepositoryOccurrenceLength, static_cast<int>(one_based_occurrences[i]));
                        }
                    }

                    csnot_file.WriteFormattedLine(formatting_string,
                                                  NewlineSubstitutor::NewlineToUnicodeNL(full_key).GetString(),
                                                  field_name.GetString(),
                                                  NewlineSubstitutor::NewlineToUnicodeNL(note.GetOperatorId()).GetString(),
                                                  date.GetString(), time.GetString(),
                                                  occurrences[0].GetString(), occurrences[1].GetString(), occurrences[2].GetString(),
                                                  NewlineSubstitutor::NewlineToUnicodeNL(note.GetContent()).GetString());
                }
            }
        }

        csnot_file.Close();
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch(...)
    {
        throw DataRepositoryException::IOError(_T("There was an error writing to the notes file."));
    }

    m_hasTransactionsToWrite = false;
}


void TextRepositoryNotesFile::LoadOldFormat(const TextRepository& repository, NullTerminatedString filename)
{
    try
    {
        CSpecFile not_file;

        if( !not_file.Open(filename.c_str(), CFile::modeRead) )
            throw DataRepositoryException::IOError(_T("There was an error opening the pre-7.0 notes file."));

        // determine the minimum size of a notes line
        const int note_identifying_portion_length = m_allLevelsKeyLength + TextRepositoryFieldLength + 17; // 17 for three occurrences
        const int min_length_line = note_identifying_portion_length + 1; // 1 for the note text

        CString line;
        CString previous_identifier; // used to combine multiline notes
        Note* previous_note = nullptr;

        while( not_file.ReadString(line) )
        {
            // ignore lines without valid notes
            if( line.GetLength() < min_length_line )
                continue;

            // potentially combine this line with the previous note in the case of multiline notes
            CString this_identifier = line.Left(note_identifying_portion_length);

            if( previous_note != nullptr && this_identifier.Compare(previous_identifier) == 0 )
            {
                CString content_continuation = FormatText(_T("%s %s"),
                                                          previous_note->GetContent().GetString(),
                                                          line.Mid(note_identifying_portion_length).GetString());
                previous_note->SetContent(content_continuation);
                continue;
            }

            CString key = line.Left(m_allLevelsKeyLength);
            CString field_name = line.Mid(m_allLevelsKeyLength, TextRepositoryFieldLength).Trim();
            CString occurrence1_text = line.Mid(m_allLevelsKeyLength + TextRepositoryFieldLength, 6).Trim();
            CString occurrence2_text = line.Mid(m_allLevelsKeyLength + TextRepositoryFieldLength + 6, 5).Trim();
            CString occurrence3_text = line.Mid(m_allLevelsKeyLength + TextRepositoryFieldLength + 12, 5).Trim();
            CString content = line.Mid(note_identifying_portion_length).Trim();
            ASSERT(occurrence1_text.GetLength() == 0);

            int record_occurrence = 0;
            int item_occurrence = 0;
            int subitem_occurrence = 0;

            // if the field has occurrences, figure out what kind they are; pre-7.0, the only kind that worked were:
            //      1 - singly occurring record     multiply occurring item  _ _ I
            //      2 - multiply occurring record   singly occurring item    _ _ R
            //      3 - multiply occurring record   multiply occurring item  _ R.I
            if( occurrence3_text.GetLength() > 0 )
            {
                const CaseItem* case_item = repository.GetCaseAccess()->LookupCaseItem(field_name);

                if( case_item == nullptr ) // the field is not in the dictionary
                    continue;

                const CDictItem& dictionary_item = case_item->GetDictionaryItem();
                const CDictRecord& dictionary_record = *dictionary_item.GetRecord();

                const int occurrence3 = _ttoi(occurrence3_text);

                if( dictionary_item.GetOccurs() > 1 && dictionary_record.GetMaxRecs() == 1 ) // case 1
                {
                    ASSERT(occurrence2_text.GetLength() == 0);
                    item_occurrence = occurrence3;
                }

                else if( dictionary_item.GetOccurs() == 1 && dictionary_record.GetMaxRecs() > 1 ) // case 2
                {
                    ASSERT(occurrence2_text.GetLength() == 0);
                    record_occurrence = occurrence3;
                }

                else if( dictionary_item.GetOccurs() > 1 && dictionary_record.GetMaxRecs() > 1 ) // case 3
                {
                    ASSERT(occurrence2_text.GetLength() > 0);
                    item_occurrence = occurrence3;
                    record_occurrence = _ttoi(occurrence2_text);
                }

                else
                {
                    ASSERT(false);
                }
            }

            // construct the note object
            CString first_level_key = key.Left(m_firstLevelKeyLength);
            CString level_key = key.Mid(m_firstLevelKeyLength);

            const size_t occurrences[3] =
            {
                static_cast<size_t>(std::max(record_occurrence - 1, 0)),
                static_cast<size_t>(std::max(item_occurrence - 1, 0)),
                static_cast<size_t>(std::max(subitem_occurrence - 1, 0))
            };

            std::shared_ptr<NamedReference> named_reference = CaseConstructionHelpers::CreateNamedReference(*repository.GetCaseAccess(), level_key, field_name, occurrences);
            previous_note = &AddNote(first_level_key, std::move(named_reference), CString(), 0, content);
            previous_identifier = this_identifier;
        }

        not_file.Close();
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch(...)
    {
        throw DataRepositoryException::IOError(_T("There was an error reading the pre-7.0 notes file."));
    }
}


void TextRepositoryNotesFile::WriteCase(Case& data_case, WriteCaseParameter* write_case_parameter)
{
    const CString& key = data_case.GetKey();
    const std::vector<Note>& new_notes = data_case.GetNotes();
    bool modified = false;

    // process case modifications
    if( write_case_parameter != nullptr && write_case_parameter->IsModifyParameter() )
    {
        if( m_notesMap != nullptr )
        {
            // remove the previous notes if the key changed
            if( write_case_parameter->GetKey().Compare(key) != 0 )
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
            m_notesMap = std::make_unique<std::map<CString, std::vector<Note>>>();

        (*m_notesMap)[key] = new_notes;
        modified = true;
    }

    if( modified )
        Save();
}


bool TextRepositoryNotesFile::RemoveEntry(const CString& key)
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


void TextRepositoryNotesFile::DeleteCase(const CString& key)
{
    if( m_notesMap != nullptr && RemoveEntry(key) )
        Save();
}
