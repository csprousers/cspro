#pragma once

#include <zCaseO/zCaseO.h>
#include <zCaseO/Case.h>
#include <zCaseO/FullLineProcessor.h>


class ZCASEO_API TextToCaseConverter
{
public:
    static constexpr TCHAR DataFileErasedRecordCharacter = '~';
    static constexpr size_t BufferLinesInitialCapacity   = 1024;
    static constexpr size_t MinBufferLinesResizeCount    = 8 * 1024;

    TextToCaseConverter(const CaseMetadata& case_metadata);
    ~TextToCaseConverter();

    struct TextBufferLine
    {
        size_t offset;
        size_t length;
    };

    struct TextSpan
    {
        size_t start;
        size_t length;
    };

    struct TextBasedKeyMetadata
    {
        std::vector<TextSpan> key_spans; // places where there are key values
        size_t key_length = 0;
    };

    // Returns the key metadata for the root level.
    const TextBasedKeyMetadata& GetKeyMetadata() const { return m_textBasedKeyMetadata.front(); }

    // Converts the case to a null-terminated wide string (whose memory this class controls).
    // An optional parameter gets the length of the string (not counting the null terminator).
    // It is expected that access to the returned buffer is complete before any other method
    // of this class is called.
    const TCHAR* CaseToTextWide(const Case& input_case, size_t* output_text_length = nullptr);

    // Converts the case to a null-terminated UTF-8 string (whose memory this class controls).
    // An optional parameter gets the length of the string (not counting the null terminator).
    // It is expected that access to the returned buffer is complete before any other method
    // of this class is called.
    const char* CaseToTextUtf8(const Case& input_case, size_t* output_text_length = nullptr);


    // Converts the wide text to a case. The input text buffer should contain only the contents
    // for a single case, meaning that the root level key should be the same for every line in
    // the input text buffer. The text buffer lines provided should have any escaped records
    // marked as zero length spans.
    void TextWideToCase(Case& output_case, const TCHAR* text_buffer, 
                        const std::vector<TextBufferLine>::const_iterator& line_iterator_begin,
                        const std::vector<TextBufferLine>::const_iterator& line_iterator_end);

    // Converts the wide text to a case. The input text buffer should contain only the contents
    // for a single case, meaning that the root level key should be the same for every line in
    // the input text buffer.
    void TextWideToCase(Case& output_case, const TCHAR* text_buffer, size_t text_length);

    // Converts the UTF-8 text to a case. The input text buffer should contain only the contents
    // for a single case, meaning that the root level key should be the same for every line in
    // the input text buffer.
    void TextUtf8ToCase(Case& output_case, const char* text_buffer, size_t text_length);

private:
    struct TextBasedCaseItemMetadata
    {
        const CaseItem* case_item;
        bool numeric;
        size_t start;
        size_t end; // the end of all occurrences
        size_t length;
        size_t occurs;
    };

    struct TextBasedCaseRecordMetadata
    {
        const CaseRecordMetadata* case_record_metadata;
        size_t max_records;
        size_t record_length;
        size_t key_length;            // the length needed for the key and record type
        size_t key_and_spaces_length; // the length needed for the key, record type, and spaces
        CString record_type;
        std::vector<TextBasedCaseItemMetadata> case_items_metadata; // only the items needed for input/output (no subitems)
        std::vector<TextSpan> space_spans; // places where there are no values
    };

private:
    void CalculateConstructionVariables();

    void IncreaseWideBufferSize(size_t minimum_buffer_size_needed = 0);

    void CaseLevelToText(const CaseLevel& case_level);
    void CaseRecordToText(const CaseRecord& case_record, bool outputting_key);

    void TextToCaseRecord(CaseRecord& case_record, size_t record_occurrence, const std::vector<TextBasedCaseItemMetadata>& case_items_metadata);

    CaseLevel* GetCaseLevelFromKey(Case& output_case, size_t level_number);

private:
    // metadata
    const CaseMetadata& m_caseMetadata;
    
    size_t m_recordTypeLength;
    size_t m_recordTypeStart;
    size_t m_recordTypeEnd;
    TCHAR* m_recordTypeLookup;

    std::vector<TextBasedKeyMetadata> m_textBasedKeyMetadata;
    std::vector<TextBasedCaseRecordMetadata> m_textBasedCaseRecordsMetadata;

    size_t m_wideBufferSize;
    TCHAR* m_wideBuffer;
    const TCHAR* m_wideBufferEnd;
    TCHAR* m_currentWideBufferPosition;

    size_t m_utf8BufferSize;
    char* m_utf8Buffer;

    // output variables
    TCHAR* m_lastRecordOutputPosition;

    // input variables
    FullLineProcessor m_fullLineProcessor;
    const TCHAR* m_lastReadRecordType;
    const TCHAR* m_currentRecordLine;
    size_t m_currentRecordLineLength;
    std::vector<std::tuple<CString, CaseLevel*>>* m_caseLevelList;

    std::vector<TextBufferLine> m_bufferLines;

    std::wstring m_unicodeNLToNewlineBuffer;


    // CR_TODO remove the below when Pre74_Case is removed
public:
    std::optional<bool> suppress_using_case_construction_reporter;
};
