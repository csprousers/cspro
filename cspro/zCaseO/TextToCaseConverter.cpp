#include "stdafx.h"
#include "TextToCaseConverter.h"
#include "FixedWidthNumericCaseItem.h"
#include "FixedWidthStringCaseItem.h"
#include <zToolsO/NewlineSubstitutor.h>


namespace
{
    constexpr size_t TextBufferIncrementSize = 8 * 1024;
}


TextToCaseConverter::TextToCaseConverter(const CaseMetadata& case_metadata)
    :   m_caseMetadata(case_metadata),
        m_recordTypeLength(0),
        m_recordTypeStart(0),
        m_recordTypeEnd(0),
        m_recordTypeLookup(nullptr),
        m_wideBufferSize(0),
        m_wideBuffer(nullptr),
        m_wideBufferEnd(nullptr),
        m_utf8BufferSize(0),
        m_utf8Buffer(nullptr),
        m_currentWideBufferPosition(nullptr),
        m_lastRecordOutputPosition(nullptr),
        m_lastReadRecordType(nullptr),
        m_currentRecordLine(nullptr),
        m_currentRecordLineLength(0),
        m_caseLevelList(nullptr)
{
    CalculateConstructionVariables();

    m_bufferLines.reserve(BufferLinesInitialCapacity);
}


TextToCaseConverter::~TextToCaseConverter()
{
    delete[] m_recordTypeLookup;
    delete[] m_wideBuffer;
    delete[] m_utf8Buffer;
    delete m_caseLevelList;
}


void TextToCaseConverter::CalculateConstructionVariables()
{
    const CDataDict& dictionary = m_caseMetadata.GetDictionary();

    // setup the record type lookup
    size_t record_type_and_key_length = 0;
    size_t last_id_start = 0;

    m_recordTypeLength = dictionary.GetRecTypeLen();

    if( m_recordTypeLength > 0 )
    {
        m_recordTypeStart = dictionary.GetRecTypeStart() - 1;
        m_recordTypeEnd = m_recordTypeStart + m_recordTypeLength;
        record_type_and_key_length = m_recordTypeStart + m_recordTypeLength;

        // setup the record type lookup string for quick lookups; because ID records are part
        // of the number of records, initialize this lookup with ~ characters because ~
        // can never be a valid record type and thus will not match with any real record types
        size_t record_type_lookup_length = m_caseMetadata.GetTotalNumberRecords() * m_recordTypeLength;

        m_recordTypeLookup = new TCHAR[record_type_lookup_length + 1];
        _tmemset(m_recordTypeLookup, DataFileErasedRecordCharacter, record_type_lookup_length);
        m_recordTypeLookup[record_type_lookup_length] = 0;

        m_lastReadRecordType = m_recordTypeLookup;
    }

    // process each case level
    for( const CaseLevelMetadata* case_level_metadata : m_caseMetadata.GetCaseLevelsMetadata() )
    {
        auto process_case_record_metadata = [&](const CaseRecordMetadata* case_record_metadata, bool processing_key) -> void
        {
            if( processing_key )
                m_textBasedKeyMetadata.emplace_back();

            TextBasedCaseRecordMetadata& text_record_metadata = m_textBasedCaseRecordsMetadata.emplace_back();

            text_record_metadata.case_record_metadata = case_record_metadata;
            text_record_metadata.max_records = case_record_metadata->GetDictionaryRecord().GetMaxRecs();
            text_record_metadata.record_length = record_type_and_key_length;
            text_record_metadata.key_and_spaces_length = record_type_and_key_length;

            size_t next_expected_dictionary_item_start = 0;

            auto add_space_span = [&](size_t start)
            {
                TextSpan* space_span = nullptr;

                // calculate the gaps in the record (where spaces will have to be copied)
                while( start > next_expected_dictionary_item_start )
                {
                    const size_t original_next_expected_dictionary_item_start = next_expected_dictionary_item_start;

                    // make sure that the expected start position isn't used by the record type
                    if( m_recordTypeLength > 0 && next_expected_dictionary_item_start == m_recordTypeStart )
                        next_expected_dictionary_item_start += m_recordTypeLength;

                    // make sure that the expected start position isn't used by an ID item
                    for( const TextBasedKeyMetadata& key_metadata : m_textBasedKeyMetadata )
                    {
                        const auto& key_span_search = std::find_if(key_metadata.key_spans.cbegin(), key_metadata.key_spans.cend(),
                            [next_expected_dictionary_item_start](const TextSpan& key_span)
                            {
                                // although it would seem that we would just need to check if next_expected_dictionary_item_start equals
                                // key_span.start, because second level IDs can overlap with first level IDs, we need to check if the
                                // expect start position is anywhere in the range of the key span
                                return ( ( next_expected_dictionary_item_start >= key_span.start ) &&
                                         ( next_expected_dictionary_item_start < ( key_span.start + key_span.length ) ) );
                            });

                        if( key_span_search != key_metadata.key_spans.cend() )
                        {
                            next_expected_dictionary_item_start = key_span_search->start + key_span_search->length;
                            break;
                        }
                    }

                    // if nothing was adjusted, add the space span, incrementing it one by one
                    // as an easy way to account for gaps while processing an absolute positioning dictionary
                    if( original_next_expected_dictionary_item_start == next_expected_dictionary_item_start )
                    {
                        if( space_span == nullptr )
                        {
                            text_record_metadata.space_spans.emplace_back(TextSpan { next_expected_dictionary_item_start, 0 });
                            space_span = &text_record_metadata.space_spans.back();
                        }

                        ++space_span->length;
                        ++next_expected_dictionary_item_start;
                        text_record_metadata.key_and_spaces_length = std::max(text_record_metadata.key_and_spaces_length, next_expected_dictionary_item_start);
                    }

                    else
                    {
                        space_span = nullptr;
                    }
                }
            };

            for( const CaseItem* case_item : case_record_metadata->GetCaseItems() )
            {
                // skip over case items that are not of fixed width
                if( !case_item->IsTypeFixed() )
                    continue;

                ASSERT(( case_item->GetType() == CaseItem::Type::FixedWidthString ) ||
                       ( case_item->GetType() == CaseItem::Type::FixedWidthNumeric ) ||
                       ( case_item->GetType() == CaseItem::Type::FixedWidthNumericWithStringBuffer ));

                const CDictItem& dictionary_item = case_item->GetDictionaryItem();

                // no need to add subitems as the parent item will cover any processing needed for its children
                if( dictionary_item.GetItemType() == ItemType::Subitem )
                    continue;

                text_record_metadata.case_items_metadata.emplace_back(TextBasedCaseItemMetadata
                    {
                        case_item,
                        case_item->IsTypeNumeric(),
                        dictionary_item.GetStart() - 1,
                        dictionary_item.GetStart() - 1 + ( dictionary_item.GetLen() * dictionary_item.GetOccurs() ),
                        dictionary_item.GetLen(),
                        dictionary_item.GetOccurs()
                    });

                const size_t dictionary_item_start = dictionary_item.GetStart() - 1;
                ASSERT(dictionary_item_start >= next_expected_dictionary_item_start);

                if( processing_key )
                {
                    // update text spans where the key is located
                    TextBasedKeyMetadata& key_metadata = m_textBasedKeyMetadata.back();
                    std::vector<TextSpan>& key_spans = key_metadata.key_spans;

                    // if this is an ID that follows the previous ID, join the spans
                    if( !key_spans.empty() && dictionary_item_start == next_expected_dictionary_item_start )
                    {
                        key_spans.back().length += dictionary_item.GetLen();
                    }

                    else
                    {
                        key_spans.emplace_back(TextSpan { dictionary_item_start, dictionary_item.GetLen() });
                    }

                    key_metadata.key_length += dictionary_item.GetLen();
                    last_id_start = dictionary_item_start;
                }

                else
                {
                    add_space_span(dictionary_item_start);
                }

                next_expected_dictionary_item_start = dictionary_item_start + dictionary_item.GetLen() * dictionary_item.GetOccurs();
                text_record_metadata.record_length = std::max(text_record_metadata.record_length, next_expected_dictionary_item_start);
            }

            if( processing_key )
            {
                record_type_and_key_length = text_record_metadata.record_length;
            }

            else
            {
                // create a fully spaced out copy of the record type
                if( m_recordTypeLength > 0 )
                {
                    text_record_metadata.record_type = CIMSAString::MakeExactLength(case_record_metadata->GetDictionaryRecord().GetRecTypeVal(), m_recordTypeLength);
                    _tmemcpy(m_recordTypeLookup + case_record_metadata->GetTotalRecordIndex() * m_recordTypeLength, text_record_metadata.record_type, m_recordTypeLength);
                }

                // add a space span if there is a record type or ID item after the record
                const size_t last_record_type_or_key_start = std::max(m_recordTypeStart, last_id_start);
                add_space_span(last_record_type_or_key_start);
            }

            text_record_metadata.key_length = record_type_and_key_length;
        };

        // process the ID record and then each record
        process_case_record_metadata(case_level_metadata->GetIdCaseRecordMetadata(), true);

        for( const CaseRecordMetadata* case_record_metadata : case_level_metadata->GetCaseRecordsMetadata() )
            process_case_record_metadata(case_record_metadata, false);
    }

    // setup the level map for multiple level applications
    if( m_textBasedKeyMetadata.size() > 1 )
        m_caseLevelList = new std::vector<std::tuple<CString, CaseLevel*>>;
}


void TextToCaseConverter::IncreaseWideBufferSize(size_t minimum_buffer_size_needed/* = 0*/)
{
    // check if the size actually has to be increased
    if( ( m_currentWideBufferPosition + minimum_buffer_size_needed ) <= m_wideBufferEnd )
        return;

    const size_t current_position = ( m_currentWideBufferPosition - m_wideBuffer );

    m_wideBufferSize += std::max(TextBufferIncrementSize, minimum_buffer_size_needed);

    // allocate larger memory and adjust the position offsets
    TCHAR* new_wide_buffer = new TCHAR[m_wideBufferSize];

    m_currentWideBufferPosition = new_wide_buffer + current_position;

    if( m_lastRecordOutputPosition != nullptr )
        m_lastRecordOutputPosition += ( new_wide_buffer - m_wideBuffer );
    
    if( m_wideBuffer != nullptr )
    {
        _tmemcpy(new_wide_buffer, m_wideBuffer, current_position);
        delete[] m_wideBuffer;
    }

    m_wideBuffer = new_wide_buffer;
    m_wideBufferEnd = m_wideBuffer + m_wideBufferSize;
}


const TCHAR* TextToCaseConverter::CaseToTextWide(const Case& input_case, size_t* output_text_length/* = nullptr*/)
{
    m_currentWideBufferPosition = m_wideBuffer;
    m_lastRecordOutputPosition = nullptr;

    CaseLevelToText(input_case.GetRootCaseLevel());

    // no case should be completely empty
    ASSERT(m_currentWideBufferPosition > m_wideBuffer);

    if( output_text_length != nullptr )
        *output_text_length = m_currentWideBufferPosition - m_wideBuffer;

    // add a null terminator
    *m_currentWideBufferPosition = 0;

    return m_wideBuffer;
}


void TextToCaseConverter::CaseLevelToText(const CaseLevel& case_level)
{
    // first output this level's records
    bool first_record_on_level = true;

    for( size_t record_number = 0; record_number < case_level.GetNumberCaseRecords(); ++record_number )
    {
        const CaseRecord& case_record = case_level.GetCaseRecord(record_number);

        if( !case_record.HasOccurrences() )
            continue;

        // make sure that there is enough room in the buffer for the entire record; the + 2 is for \r\n and + 1 is for the null terminator
        const TextBasedCaseRecordMetadata& text_record_metadata = m_textBasedCaseRecordsMetadata[case_record.GetCaseRecordMetadata().GetTotalRecordIndex()];
        const size_t buffer_size_needed = ( case_record.GetNumberOccurrences() * ( text_record_metadata.record_length + 2 ) ) + 1;
        IncreaseWideBufferSize(buffer_size_needed);

        // if this is the first record on the level, output the key
        if( first_record_on_level )
        {
            CaseRecordToText(case_level.GetIdCaseRecord(), true);
            first_record_on_level = false;
        }

        CaseRecordToText(case_record, false);
    }

    // and then output any children levels
    for( size_t level_index = 0; level_index < case_level.GetNumberChildCaseLevels(); ++level_index )
        CaseLevelToText(case_level.GetChildCaseLevel(level_index));
}


void TextToCaseConverter::CaseRecordToText(const CaseRecord& case_record, bool outputting_key)
{
    ASSERT(case_record.HasOccurrences());

    const TextBasedCaseRecordMetadata& text_record_metadata = m_textBasedCaseRecordsMetadata[case_record.GetCaseRecordMetadata().GetTotalRecordIndex()];
   
    // if the key has already been generated (on a previous record), copy it to the current buffer
    if( m_lastRecordOutputPosition != m_currentWideBufferPosition && m_lastRecordOutputPosition != nullptr )
        _tmemcpy(m_currentWideBufferPosition, m_lastRecordOutputPosition, text_record_metadata.key_length);

    TCHAR* this_record_last_output_position = nullptr;
    bool need_to_add_record_type_and_spaces = !outputting_key;

    CaseItemIndex index = case_record.GetCaseItemIndex();

    for( ; index.GetRecordOccurrence() < case_record.GetNumberOccurrences(); index.IncrementRecordOccurrence() )
    {
        m_lastRecordOutputPosition = m_currentWideBufferPosition;

        // if a record of this type has already been output, copy it (to get the key and, sometimes, the record type and spaces)
        if( this_record_last_output_position != nullptr )
            _tmemcpy(m_currentWideBufferPosition, this_record_last_output_position, text_record_metadata.key_and_spaces_length);

        // add the record type and space fill any empty parts of this record
        if( need_to_add_record_type_and_spaces )
        {
            if( m_recordTypeLength > 0 )
                _tmemcpy(m_currentWideBufferPosition + m_recordTypeStart, text_record_metadata.record_type, m_recordTypeLength);

            for( const TextSpan& space_span : text_record_metadata.space_spans )
                _tmemset(m_currentWideBufferPosition + space_span.start, _T(' '), space_span.length);

            need_to_add_record_type_and_spaces = false;
        }

        // copy each case item
        for( const TextBasedCaseItemMetadata& text_case_item_metadata : text_record_metadata.case_items_metadata )
        {
            TCHAR* case_item_output_position = m_currentWideBufferPosition + text_case_item_metadata.start;

            for( index.ResetItemOccurrence(); index.GetItemOccurrence() < text_case_item_metadata.occurs; index.IncrementItemOccurrence() )
            {
                dynamic_cast<const FixedWidthCaseItem*>(text_case_item_metadata.case_item)->OutputFixedValue(index, case_item_output_position);

                // turn \n -> ␤
                for( const TCHAR* const case_item_output_end = case_item_output_position + text_case_item_metadata.length;
                    case_item_output_position < case_item_output_end;
                    ++case_item_output_position )
                {
                    if( *case_item_output_position == '\n' )
                        *case_item_output_position = NewlineSubstitutor::UnicodeNL;
                }
            }
        }

        // if outputting the key, then we are done
        if( outputting_key )
            return;

        // otherwise, advance the position and remove any excessive spaces (unless the spaces are part of the record type or key)
        this_record_last_output_position = m_currentWideBufferPosition;

        TCHAR* trimmed_last_character_position = m_currentWideBufferPosition + text_record_metadata.record_length - 1;
        const TCHAR* const last_eligible_character_for_trimming_position = m_currentWideBufferPosition + text_record_metadata.key_length;

        while( trimmed_last_character_position >= last_eligible_character_for_trimming_position && *trimmed_last_character_position == _T(' ') )
            --trimmed_last_character_position;

        m_currentWideBufferPosition = trimmed_last_character_position + 1;

        // because this record's length may no longer be complete, we need to potentially add the record type and
        // spaces for any subsequent records because this record can no longer be fully copied
        if( ( m_currentWideBufferPosition - this_record_last_output_position ) < (int)text_record_metadata.key_and_spaces_length )
            need_to_add_record_type_and_spaces = true;

        // add the newline and advance the position
        *(m_currentWideBufferPosition++) = '\r';
        *(m_currentWideBufferPosition++) = '\n';
    }
}


void TextToCaseConverter::TextWideToCase(Case& output_case, const TCHAR* text_buffer,
                                         const std::vector<TextBufferLine>::const_iterator& line_iterator_begin,
                                         const std::vector<TextBufferLine>::const_iterator& line_iterator_end)
{
    const bool using_case_construction_reporter = ( output_case.GetCaseConstructionReporter() != nullptr );

    if( using_case_construction_reporter )
    {
        if( suppress_using_case_construction_reporter != true ) // CR_TODO remove this line
            output_case.GetCaseConstructionReporter()->IncrementCaseLevelCount(0);
    }

    // reset the root case level
    CaseLevel* current_case_level = &output_case.GetRootCaseLevel();
    current_case_level->Reset();

    CaseRecord* first_level_id_case_record_for_construction = &current_case_level->GetIdCaseRecord();

    if( m_caseLevelList != nullptr )
        m_caseLevelList->clear();

    for( auto line_iterator = line_iterator_begin; line_iterator != line_iterator_end; ++line_iterator )
    {
        if( using_case_construction_reporter )
        {
            if( suppress_using_case_construction_reporter != true ) // CR_TODO remove this line
                output_case.GetCaseConstructionReporter()->IncrementRecordCount();
        }

        m_currentRecordLine = text_buffer + line_iterator->offset;
        m_currentRecordLineLength = line_iterator->length;

        // skip non-blank lines
        if( m_currentRecordLineLength == 0 )
        {
            if( using_case_construction_reporter && *m_currentRecordLine == DataFileErasedRecordCharacter )
                output_case.GetCaseConstructionReporter()->IncrementErasedRecordCount();

            continue;
        }

        ASSERT(*m_currentRecordLine != DataFileErasedRecordCharacter);

        // process the initial key
        if( first_level_id_case_record_for_construction != nullptr )
        {
            TextToCaseRecord(*first_level_id_case_record_for_construction, 0, m_textBasedCaseRecordsMetadata.front().case_items_metadata);
            first_level_id_case_record_for_construction = nullptr;
        }

        TextBasedCaseRecordMetadata* text_record_metadata;

        if( m_recordTypeLength == 0 )
        {
            ASSERT(m_textBasedCaseRecordsMetadata.size() == 2);
            text_record_metadata = &m_textBasedCaseRecordsMetadata.back();
        }

        // process the record type if one is used
        else
        {
            // space fill the record line, if necessary, to account for the record type
            m_currentRecordLine = m_fullLineProcessor.GetLine(m_currentRecordLine, &m_currentRecordLineLength, m_recordTypeEnd);

            const TCHAR* record_type_position = m_currentRecordLine + m_recordTypeStart;

            // generally each line read will have a record type equal to or occurring after the previously read one, so
            // instead of searching from the beginning of the list each time, we will start from where we left off previously
            ASSERT(m_lastReadRecordType >= m_recordTypeLookup);

            // search forward
            const TCHAR* record_type_iterator;

            for( record_type_iterator = m_lastReadRecordType; *record_type_iterator != 0; record_type_iterator += m_recordTypeLength )
            {
                if( _tmemcmp(record_type_iterator, record_type_position, m_recordTypeLength) == 0 )
                    goto record_type_found;
            }

            // if not found, restart the search from the beginning
            if( m_lastReadRecordType > m_recordTypeLookup )
            {
                // the search can actually start from the second entry in the lookup because the
                // first entry is the ID item record (which doesn't have a record type)
                ASSERT(*m_recordTypeLookup == DataFileErasedRecordCharacter);

                for( record_type_iterator = m_recordTypeLookup + m_recordTypeLength; record_type_iterator != m_lastReadRecordType; record_type_iterator += m_recordTypeLength )
                {
                    if( _tmemcmp(record_type_iterator, record_type_position, m_recordTypeLength) == 0 )
                        goto record_type_found;
                }
            }

            // issue a warning upon a bad record type
            if( using_case_construction_reporter )
            {
                output_case.GetCaseConstructionReporter()->IncrementBadRecordCount();

                output_case.GetCaseConstructionReporter()->BadRecordType(output_case,
                    CString(record_type_position, m_recordTypeLength), CString(m_currentRecordLine, m_currentRecordLineLength));
            }

            goto end_record_line_processing;

            // at this point, the record type has been identified as a valid record
record_type_found:
            m_lastReadRecordType = record_type_iterator;

            const size_t record_index = ( m_lastReadRecordType - m_recordTypeLookup ) / m_recordTypeLength;
            text_record_metadata = &m_textBasedCaseRecordsMetadata[record_index];

            ASSERT(_tmemcmp(text_record_metadata->record_type, record_type_position, m_recordTypeLength) == 0);
        }

        // if using multiple levels, make sure that a valid level exists for the record with the given key
        if( m_caseLevelList != nullptr )
        {
            // space fill the record line, if necessary, to account for at least all of the IDs
            m_currentRecordLine = m_fullLineProcessor.GetLine(m_currentRecordLine, &m_currentRecordLineLength, text_record_metadata->key_length);

            const size_t level_number = text_record_metadata->case_record_metadata->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber();
            current_case_level = GetCaseLevelFromKey(output_case, level_number);
        }

        // this block allows the above goto statements above to skip past variable initializations
        {
            CaseRecord& new_case_record = current_case_level->GetCaseRecord(text_record_metadata->case_record_metadata->GetRecordIndex());

            // check if there are too too many of this record
            const size_t new_record_occurrence = new_case_record.GetNumberOccurrences();

            if( new_record_occurrence == text_record_metadata->max_records )
            {
                // issue a warning when there are too many records
                if( using_case_construction_reporter )
                {
                    output_case.GetCaseConstructionReporter()->TooManyRecordOccurrences(output_case,
                        text_record_metadata->case_record_metadata->GetDictionaryRecord().GetName(), text_record_metadata->max_records);
                }

                goto end_record_line_processing;
            }

            // add the record
            new_case_record.SetNumberOccurrences(new_record_occurrence + 1);

            // process the record
            TextToCaseRecord(new_case_record, new_record_occurrence, text_record_metadata->case_items_metadata);
        }

end_record_line_processing:
        ;
    }
}


void TextToCaseConverter::TextWideToCase(Case& output_case, const TCHAR* text_buffer, size_t text_length)
{
    // every so often adjust the size of the buffer lines vector to keep it from growing too large
    if( m_bufferLines.size() > MinBufferLinesResizeCount )
        m_bufferLines.clear();

    // the buffer vector lines vector gets reused upon subsequent calls to this method
    size_t buffer_line_index = 0;

    // parse the text for newlines
    const TCHAR* const end_position = text_buffer + text_length;

    const TCHAR* position = text_buffer;
    const TCHAR* start_line_position = position;

    while( true )
    {
        const bool end_of_buffer = ( position == end_position );

        // process the end of buffer as if it were a newline
        if( end_of_buffer || is_crlf(*position) )
        {
            const size_t line_length = position - start_line_position;

            // treat \r\n as a pair
            if( !end_of_buffer && ( *(position++) == '\r' && position < end_position && *position == '\n' ) )
                ++position;

            // don't process blank lines
            if( line_length > 0 )
            {
                // if necessary, resize the buffer lines to account for all new lines
                if( buffer_line_index == m_bufferLines.size() )
                    m_bufferLines.resize(m_bufferLines.size() + BufferLinesInitialCapacity);

                // mark erased lines as having length 0
                m_bufferLines[buffer_line_index].length = ( *start_line_position == DataFileErasedRecordCharacter ) ? 0 :
                                                                                                                      line_length;
                    
                m_bufferLines[buffer_line_index].offset = start_line_position - text_buffer;

                ++buffer_line_index;
            }

            // end processing or start a new line
            if( end_of_buffer )
                break;

            start_line_position = position;
        }

        // a normal character
        else
        {
            ++position;
        }
    }

    TextWideToCase(output_case, text_buffer, m_bufferLines.cbegin(), m_bufferLines.cbegin() + buffer_line_index);
}


void TextToCaseConverter::TextToCaseRecord(CaseRecord& case_record, size_t record_occurrence, const std::vector<TextBasedCaseItemMetadata>& case_items_metadata)
{
    CaseItemIndex index = case_record.GetCaseItemIndex(record_occurrence);

    // don't process any items that start after the record line
    auto case_item_iterator = case_items_metadata.crbegin();
    auto case_item_crend = case_items_metadata.crend();

    for( ; case_item_iterator != case_item_crend; ++case_item_iterator )
    {
        if( case_item_iterator->start < m_currentRecordLineLength )
        {
            m_currentRecordLine = m_fullLineProcessor.GetLine(m_currentRecordLine, &m_currentRecordLineLength, case_item_iterator->end);
            break;
        }
    }

    for( ; case_item_iterator != case_item_crend; ++case_item_iterator )
    {
        const TextBasedCaseItemMetadata& text_case_item_metadata = *case_item_iterator;
        const TCHAR* case_item_input_position = m_currentRecordLine + text_case_item_metadata.start;

        for( index.ResetItemOccurrence(); index.GetItemOccurrence() < text_case_item_metadata.occurs; index.IncrementItemOccurrence() )
        {
            const TCHAR* const case_item_input_start = case_item_input_position;
            const TCHAR* value_text = case_item_input_start;

            // potentially turn ␤ -> \n
            for( const TCHAR* const case_item_input_end = case_item_input_start + text_case_item_metadata.length;
                 case_item_input_position < case_item_input_end;
                 ++case_item_input_position )
            {
                if( *case_item_input_position == NewlineSubstitutor::UnicodeNL )
                {
                    m_unicodeNLToNewlineBuffer.assign(case_item_input_start, text_case_item_metadata.length);
                    NewlineSubstitutor::MakeUnicodeNLToNewline(m_unicodeNLToNewlineBuffer);
                    value_text = m_unicodeNLToNewlineBuffer.c_str();
                    case_item_input_position = case_item_input_end;
                    break;
                }
            }

            ASSERT(case_item_input_position == ( case_item_input_start + text_case_item_metadata.length ));

            if( text_case_item_metadata.numeric )
            {
                assert_cast<const FixedWidthNumericCaseItem*>(text_case_item_metadata.case_item)->SetValueFromTextInput(index, value_text);
            }

            else
            {
                assert_cast<const FixedWidthStringCaseItem*>(text_case_item_metadata.case_item)->SetFixedWidthValue(index, value_text);
            }
        }
    }
}


CaseLevel* TextToCaseConverter::GetCaseLevelFromKey(Case& output_case, size_t level_number)
{
    // because multiple level applications are rare, little effort has been made to make this level matching routine quick
    ASSERT(m_caseLevelList != nullptr);

    CaseLevel* case_level = &output_case.GetRootCaseLevel();
    CString key;

    // generate a key for each level (beyond the first) and make sure that a level exists for that key
    for( size_t level_counter = 1; level_counter <= level_number; ++level_counter )
    {
        // generate the key
        const TextBasedKeyMetadata& key_metadata = m_textBasedKeyMetadata[level_counter];

        const size_t old_key_length = key.GetLength();
        const size_t new_key_length = old_key_length + key_metadata.key_length;
        TCHAR* key_iterator = key.GetBufferSetLength(new_key_length) + old_key_length;

        for( const TextSpan& key_span : key_metadata.key_spans )
        {
            _tmemcpy(key_iterator, m_currentRecordLine + key_span.start, key_span.length);
            key_iterator += key_span.length;
        }

        key.ReleaseBuffer(new_key_length);

        // see if a case level with this key already exists
        const auto& key_search = std::find_if(m_caseLevelList->begin(), m_caseLevelList->end(),
                                              [&](const std::tuple<CString, CaseLevel*>& key_case_level_tuple) { return ( std::get<0>(key_case_level_tuple) == key ); });

        // if not found, add a case for this level (case_level is currently the parent case level)
        if( key_search == m_caseLevelList->end() )
        {
            case_level = &case_level->AddChildCaseLevel();
            m_caseLevelList->emplace_back(key, case_level);

            if( output_case.GetCaseConstructionReporter() != nullptr )
            {
                if( suppress_using_case_construction_reporter != true ) // CR_TODO remove this line
                    output_case.GetCaseConstructionReporter()->IncrementCaseLevelCount(case_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber());
            }

            // process the level IDs
            CaseRecord& id_case_record = case_level->GetIdCaseRecord();
            TextToCaseRecord(id_case_record, 0, m_textBasedCaseRecordsMetadata[id_case_record.GetCaseRecordMetadata().GetTotalRecordIndex()].case_items_metadata);
        }

        else
        {
            case_level = std::get<1>(*key_search);
        }
    }

    return case_level;
}


const char* TextToCaseConverter::CaseToTextUtf8(const Case& input_case, size_t* output_text_length/* = nullptr*/)
{
    size_t wide_output_text_length;
    CaseToTextWide(input_case, &wide_output_text_length);

    // one wide character can map to four UTF-8 characters, so ensure that the buffer is
    // large enough for this (plus the null terminator)
    const size_t utf8_buffer_size_needed = ( wide_output_text_length * 4 ) + 1;

    if( m_utf8BufferSize < utf8_buffer_size_needed )
    {
        m_utf8BufferSize = std::max(TextBufferIncrementSize, utf8_buffer_size_needed);
        delete[] m_utf8Buffer;
        m_utf8Buffer = new char[m_utf8BufferSize];
    }

    const size_t utf8_output_text_length = UTF8Convert::WideBufferToUTF8Buffer(m_wideBuffer, wide_output_text_length + 1, m_utf8Buffer, m_utf8BufferSize);
    ASSERT(m_utf8Buffer[utf8_output_text_length - 1] == 0);

    if( output_text_length != nullptr )
        *output_text_length = utf8_output_text_length - 1;

    return m_utf8Buffer;
}


void TextToCaseConverter::TextUtf8ToCase(Case& output_case, const char* text_buffer, size_t text_length)
{
    // make sure that the wide character buffer is large enough to hold the converted contents
    m_currentWideBufferPosition = m_wideBuffer;
    IncreaseWideBufferSize(text_length);

    const size_t wide_text_length = UTF8Convert::UTF8BufferToWideBuffer(text_buffer, text_length, m_wideBuffer, m_wideBufferSize);

    TextWideToCase(output_case, m_wideBuffer, wide_text_length);
}
