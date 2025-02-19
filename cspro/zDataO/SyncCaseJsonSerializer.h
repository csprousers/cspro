#pragma once

class BinaryCaseItem;
class CaseJsonParserHelper;
class CaseJsonWriterSerializerHelper;
class NumericCaseItem;
class StringCaseItem;
class SyncCaseV2JsonParser;
class SyncCaseV2JsonWriter;


// --------------------------------------------------------------------------
// SyncCaseJsonSerializer
//
// SyncCaseJsonSerializer's subclasses are used to convert cases to JSON
// for syncing. These classes are not intended to be used directly, but
// should instead be accessed via a SyncCaseSerializer object.
// --------------------------------------------------------------------------

class SyncCaseJsonSerializer
{
protected:
    SyncCaseJsonSerializer(std::shared_ptr<const CaseAccess> case_access, std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper);

public:
    virtual ~SyncCaseJsonSerializer() { }

    // Writes syncable JSON for the case in a format determined by a subclass.
    virtual void WriteCase(JsonWriter& json_writer, const Case& data_case) = 0;

    // Parses syncable JSON for the case in a format determined by a subclass.
    virtual void ParseCase(Case& data_case, const JsonNode& json_node) = 0;

    // Writes extra details for syncing binary items (to an object already started).
    static void WriteBinaryCaseItemSyncableDetails(JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index, const char* size_key);

protected:
    std::shared_ptr<CaseJsonParserHelper> m_caseJsonParserHelper; // non-null
};



// --------------------------------------------------------------------------
// SyncCaseV3JsonSerializer
// --------------------------------------------------------------------------

class SyncCaseV3JsonSerializer : public SyncCaseJsonSerializer
{
public:
    // If case_json_parser_helper is null, it will be set to an object that creates SyncWithDataBinaryContentReader objects when parsing binary data.
    SyncCaseV3JsonSerializer(std::shared_ptr<const CaseAccess> case_access, std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper = nullptr);
    ~SyncCaseV3JsonSerializer();

    void WriteCase(JsonWriter& json_writer, const Case& data_case) override;
    void ParseCase(Case& data_case, const JsonNode& json_node) override;

private:
    std::shared_ptr<CaseJsonWriterSerializerHelper> m_caseJsonWriterSerializerHelper;
};



// --------------------------------------------------------------------------
// SyncCaseV2JsonSerializer
// --------------------------------------------------------------------------

class SyncCaseV2JsonSerializer : public SyncCaseJsonSerializer
{
public:
    // If case_json_parser_helper is null, it will be set to an object that creates SyncWithDataBinaryContentReader objects when parsing binary data.
    SyncCaseV2JsonSerializer(std::shared_ptr<const CaseAccess> case_access, bool write_case_as_text_blob, std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper);

    SyncCaseV2JsonWriter& GetSyncCaseV2JsonWriter() { return *m_syncCaseV2JsonWriter; }

    void WriteCase(JsonWriter& json_writer, const Case& data_case) override;
    void ParseCase(Case& data_case, const JsonNode& json_node) override;

    static std::string GetJsonKeyForCaseLevel(size_t level_number);

private:
    std::unique_ptr<SyncCaseV2JsonWriter> m_syncCaseV2JsonWriter;
    std::unique_ptr<SyncCaseV2JsonParser> m_syncCaseV2JsonParser;
};



// --------------------------------------------------------------------------
// SyncCaseV2JsonWriter
// --------------------------------------------------------------------------

class ZDATAO_API SyncCaseV2JsonWriter
{
public:
    SyncCaseV2JsonWriter(std::shared_ptr<const CaseAccess> case_access, bool write_case_as_text_blob);
    ~SyncCaseV2JsonWriter();

    // if true, the data for the case levels, records, and items will be serialized as a single string
    void SetStringifyLevelData(bool flag) { m_stringifyLevelData = flag; }

    void WriteCase(JsonWriter& json_writer, const Case& data_case) const;

private:
    bool WritingBackwardsCompatibleWithCSPro74() const { return ( m_textToCaseConverter != nullptr ); }

    static void WriteNamedReference(JsonWriter& json_writer, const NamedReference& named_reference);

    static void WriteNotes(JsonWriter& json_writer, const std::vector<Note>& notes);

    void WritePartialSave(JsonWriter& json_writer, PartialSaveMode partial_save_mode, const CaseItemReference* partial_save_case_item_reference) const;

    // for case data <= CSPro 7.4
    void WriteCaseDataAsTextLines(JsonWriter& json_writer, const Case& data_case) const;

    // for case data > CSPro 7.4
    void WriteCaseData(JsonWriter& json_writer, const Case& data_case) const;
    static void WriteCaseLevel(JsonWriter& json_writer, const CaseLevel& case_level);
    static void WriteCaseRecord(JsonWriter& json_writer, const CaseRecord& case_record);
    static void WriteCaseItem(JsonWriter& json_writer, const CaseItem& case_item, CaseItemIndex& index);
    static void WriteCaseItemValue(JsonWriter& json_writer, const NumericCaseItem& numeric_case_item, const CaseItemIndex& index);
    static void WriteCaseItemValue(JsonWriter& json_writer, const StringCaseItem& string_case_item, const CaseItemIndex& index);
    static void WriteCaseItemValue(JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index);

private:
    std::unique_ptr<TextToCaseConverter> m_textToCaseConverter; // used for <= CSPro 7.4
    bool m_stringifyLevelData;
};



// --------------------------------------------------------------------------
// SyncCaseV2JsonParser
// --------------------------------------------------------------------------

class ZDATAO_API SyncCaseV2JsonParser
{
public:
    SyncCaseV2JsonParser(std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper);
    ~SyncCaseV2JsonParser();

    void ParseCase(Case& data_case, const JsonNode& json_node);

private:
    template<typename T = NamedReference>
    std::unique_ptr<T> ParseNamedReference(const JsonNode& named_reference_node) const;

    Note ParseNote(const JsonNode& note_node) const;

    void ParsePartialSave(const JsonNode& json_node, Case& data_case) const;

    // for case data <= CSPro 7.4
    void ParseCaseDataAsTextLines(const JsonNode& data_node, Case& data_case);

    // for case data > CSPro 7.4
    void ParseCaseData(const JsonNode& level_1_node, Case& data_case) const;
    void ParseCaseLevel(const JsonNode& case_level_node, CaseLevel& case_level) const;
    void ParseCaseRecords(const JsonNode& case_records_node, CaseRecord& case_record) const;
    void ParseCaseItemsOnCaseRecord(const JsonNode& case_record_node, CaseRecord& case_record, CaseItemIndex& index) const;
    void ParseCaseItem(const JsonNode& case_item_node, const CaseItem& case_item, CaseItemIndex& index) const;
    void ParseNumericCaseItem(const JsonNode& case_item_node, const NumericCaseItem& numeric_case_item, CaseItemIndex& index) const;
    void ParseStringCaseItem(const JsonNode& case_item_node, const StringCaseItem& string_case_item, CaseItemIndex& index) const;

private:
    std::shared_ptr<CaseJsonParserHelper> m_caseJsonParserHelper;
    std::unique_ptr<TextToCaseConverter> m_textToCaseConverter;
};
