#include "stdafx.h"
#include "SyncCaseJsonSerializer.h"
#include "SyncWithDataBinaryContentReader.h"
#include <zCaseO/CaseConstructionHelpers.h>
#include <zCaseO/CaseJsonSerializer.h>
#include <zCaseO/TextToCaseConverter.h>


CREATE_JSON_KEY(caseid)
CREATE_JSON_KEY(caseids)
CREATE_JSON_KEY(field)
CREATE_JSON_KEY(itemOccurrence)
CREATE_JSON_KEY(recordOccurrence)
CREATE_JSON_KEY(subitemOccurrence)


// --------------------------------------------------------------------------
// SyncWithDataCaseJsonParserHelper
// --------------------------------------------------------------------------

class SyncWithDataCaseJsonParserHelper : public CaseJsonParserHelper
{
public:
    using CaseJsonParserHelper::CaseJsonParserHelper;

    std::unique_ptr<BinaryContentReader> CreateBinaryContentReader(std::optional<uint64_t> size) override
    {
        return std::make_unique<SyncWithDataBinaryContentReader>(std::move(size));
    }
};



// --------------------------------------------------------------------------
// SyncCaseJsonSerializer
// --------------------------------------------------------------------------

SyncCaseJsonSerializer::SyncCaseJsonSerializer(std::shared_ptr<const CaseAccess> case_access, std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper)
    :   m_caseJsonParserHelper(std::move(case_json_parser_helper))
{
    ASSERT(case_access != nullptr);

    if( m_caseJsonParserHelper == nullptr )
        m_caseJsonParserHelper = std::make_unique<SyncWithDataCaseJsonParserHelper>(std::move(case_access));

    ASSERT(m_caseJsonParserHelper->GetCaseAccess() != nullptr);
}


void SyncCaseJsonSerializer::WriteBinaryCaseItemSyncableDetails(JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index, const char* const size_key)
{
    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
    ASSERT(binary_data_accessor.IsDefined());

    try
    {
        json_writer.Write(JK::signature, binary_data_accessor.GetSignature())
                   .Write(size_key, binary_data_accessor.GetBinaryDataSize());
    }

    catch(...)
    {
        ASSERT(false);
    }
}



// --------------------------------------------------------------------------
// SyncCaseV3JsonSerializer
// --------------------------------------------------------------------------

class SyncCaseV3JsonWriterSerializer : public CaseJsonWriterSerializerHelper
{
public:
    SyncCaseV3JsonWriterSerializer()
    {
        SetBinaryDataWriter(
            [&](JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
            {
                SyncCaseJsonSerializer::WriteBinaryCaseItemSyncableDetails(json_writer, binary_case_item, index, JK::size);
            });
    }

    bool GetWriteCaseNote() const override    { return true; }
    bool GetWriteVectorClock() const override { return true; }
};


SyncCaseV3JsonSerializer::SyncCaseV3JsonSerializer(std::shared_ptr<const CaseAccess> case_access, std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper/* = nullptr*/)
    :   SyncCaseJsonSerializer(std::move(case_access), std::move(case_json_parser_helper)),
        m_caseJsonWriterSerializerHelper(std::make_unique<SyncCaseV3JsonWriterSerializer>())
{
}


SyncCaseV3JsonSerializer::~SyncCaseV3JsonSerializer()
{
}


void SyncCaseV3JsonSerializer::WriteCase(JsonWriter& json_writer, const Case& data_case)
{
    const auto case_json_writer_serializer_holder = json_writer.GetSerializerHelper().Register(m_caseJsonWriterSerializerHelper);
    json_writer.Write(data_case);
}


void SyncCaseV3JsonSerializer::ParseCase(Case& data_case, const JsonNode& json_node)
{
    m_caseJsonParserHelper->ParseJson_noexcept(data_case, json_node);

    data_case.SetPositionInRepository(json_node.GetOrDefault<double>(JK::position, -1));
}



// --------------------------------------------------------------------------
// SyncCaseV2JsonSerializer
// --------------------------------------------------------------------------

SyncCaseV2JsonSerializer::SyncCaseV2JsonSerializer(std::shared_ptr<const CaseAccess> case_access, const bool write_case_as_text_blob,
                                                   std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper/* = nullptr*/)
    :   SyncCaseJsonSerializer(case_access, std::move(case_json_parser_helper)),
        m_syncCaseV2JsonWriter(std::make_unique<SyncCaseV2JsonWriter>(std::move(case_access), write_case_as_text_blob)),
        m_syncCaseV2JsonParser(std::make_unique<SyncCaseV2JsonParser>(m_caseJsonParserHelper))
{
}


void SyncCaseV2JsonSerializer::WriteCase(JsonWriter& json_writer, const Case& data_case)
{
    m_syncCaseV2JsonWriter->WriteCase(json_writer, data_case);
}


void SyncCaseV2JsonSerializer::ParseCase(Case& data_case, const JsonNode& json_node)
{
    m_syncCaseV2JsonParser->ParseCase(data_case, json_node);
}


std::string SyncCaseV2JsonSerializer::GetJsonKeyForCaseLevel(const size_t level_number)
{
    return FormatText("level-%d", static_cast<int>(level_number) + 1);
}



// --------------------------------------------------------------------------
// SyncCaseV2JsonWriter
// --------------------------------------------------------------------------

SyncCaseV2JsonWriter::SyncCaseV2JsonWriter(const std::shared_ptr<const CaseAccess> case_access, const bool write_case_as_text_blob)
    :   m_stringifyLevelData(true)
{
     if( write_case_as_text_blob )
     {
         ASSERT(case_access != nullptr);
         m_textToCaseConverter = std::make_unique<TextToCaseConverter>(case_access->GetCaseMetadata());
     }
}


SyncCaseV2JsonWriter::~SyncCaseV2JsonWriter()
{
}


void SyncCaseV2JsonWriter::WriteCase(JsonWriter& json_writer, const Case& data_case) const
{
    json_writer.BeginObject();

    json_writer.Write(JK::id, data_case.GetUuid());

    if( !data_case.GetCaseLabel().empty() || WritingBackwardsCompatibleWithCSPro74() )
        json_writer.Write(JK::label, data_case.GetCaseLabel());

    json_writer.Write(JK::caseids, data_case.GetKey());

    if( data_case.GetDeleted() || WritingBackwardsCompatibleWithCSPro74() )
        json_writer.Write(JK::deleted, data_case.GetDeleted());

    if( data_case.GetVerified() || WritingBackwardsCompatibleWithCSPro74() )
        json_writer.Write(JK::verified, data_case.GetVerified());

    if( !data_case.GetNotes().empty() || WritingBackwardsCompatibleWithCSPro74() )
        WriteNotes(json_writer, data_case.GetNotes());

    json_writer.Write(JK::clock, data_case.GetVectorClock());

    if( data_case.IsPartial() )
        WritePartialSave(json_writer, data_case.GetPartialSaveMode(), data_case.GetPartialSaveCaseItemReference());

    if( WritingBackwardsCompatibleWithCSPro74() )
    {
        WriteCaseDataAsTextLines(json_writer, data_case);
    }

    else
    {
        WriteCaseData(json_writer, data_case);
    }

    json_writer.EndObject();
}


void SyncCaseV2JsonWriter::WriteNamedReference(JsonWriter& json_writer, const NamedReference& named_reference)
{
    json_writer.BeginObject()
               .Write(JK::name, named_reference.GetName())
               .Write(JK::levelKey, named_reference.GetLevelKey());

    // named references don't always have occurrences but at the moment they must be serialized for CSWeb
    const std::vector<size_t>& one_based_occurrences = named_reference.GetOneBasedOccurrences();
    ASSERT(one_based_occurrences.empty() || one_based_occurrences.size() == 3);

    json_writer.Write(JK::recordOccurrence, one_based_occurrences.empty() ? 0 : one_based_occurrences[0])
               .Write(JK::itemOccurrence, one_based_occurrences.empty() ? 0 : one_based_occurrences[1])
               .Write(JK::subitemOccurrence, one_based_occurrences.empty() ? 0 : one_based_occurrences[2])
               .EndObject();
}


void SyncCaseV2JsonWriter::WriteNotes(JsonWriter& json_writer, const std::vector<Note>& notes)
{
    json_writer.WriteObjects(JK::notes, notes,
        [&](const Note& note)
        {
            json_writer.Write(JK::content, note.GetContent());

            json_writer.Key(JK::field);
            WriteNamedReference(json_writer, note.GetNamedReference());

            json_writer.Write(JK::operatorId, note.GetOperatorId())
                       .WriteDate(JK::modifiedTime, note.GetModifiedDateTime());
        });
}


void SyncCaseV2JsonWriter::WritePartialSave(JsonWriter& json_writer, const PartialSaveMode partial_save_mode, const CaseItemReference* const partial_save_case_item_reference) const
{
    ASSERT(partial_save_mode != PartialSaveMode::None);

    json_writer.BeginObject(JK::partialSave);
    json_writer.Write(JK::mode, partial_save_mode);

    if( partial_save_case_item_reference != nullptr )
    {
        json_writer.Key(JK::field);
        WriteNamedReference(json_writer, *partial_save_case_item_reference);
    }

    else if( WritingBackwardsCompatibleWithCSPro74() )
    {
        // CSWeb requires a field, so give it a dummy field
        json_writer.Key(JK::field);
        WriteNamedReference(json_writer, NamedReference(std::string(), std::string()));
    }

    json_writer.EndObject();
}


void SyncCaseV2JsonWriter::WriteCaseDataAsTextLines(JsonWriter& json_writer, const Case& data_case) const
{
    ASSERT(m_textToCaseConverter != nullptr);

    size_t case_data_length;
    const char* const case_data_text = m_textToCaseConverter->CaseToTextUtf8(data_case, &case_data_length);

    // the data is written out as an array of lines so that the newlines in the case data do not mess up JSON parsing
    json_writer.BeginArray(JK::data);

    SO::ForeachLine(std::string_view(case_data_text, case_data_length), false,
        [&](const std::string_view line_sv)
        {
            json_writer.Write(line_sv);
        });

    json_writer.EndArray();
}


void SyncCaseV2JsonWriter::WriteCaseData(JsonWriter& json_writer, const Case& data_case) const
{
    ASSERT(data_case.GetRootCaseLevel().GetCaseLevelMetadata().GetDictLevel().GetLevelNumber() == 0);

    json_writer.Key(SyncCaseV2JsonSerializer::GetJsonKeyForCaseLevel(0));

    if( m_stringifyLevelData )
    {
        const std::unique_ptr<JsonStringWriter> case_data_json_writer = Json::CreateStringWriter(JsonFormattingOptions::Compact);
        WriteCaseLevel(*case_data_json_writer, data_case.GetRootCaseLevel());
        json_writer.Write(case_data_json_writer->GetString());
    }

    else
    {
        WriteCaseLevel(json_writer, data_case.GetRootCaseLevel());
    }
}


void SyncCaseV2JsonWriter::WriteCaseLevel(JsonWriter& json_writer, const CaseLevel& case_level)
{
    json_writer.BeginObject();

    json_writer.Key(JK::id);
    WriteCaseRecord(json_writer, case_level.GetIdCaseRecord());

    for( size_t record_number = 0; record_number < case_level.GetNumberCaseRecords(); ++record_number )
    {
        const CaseRecord& case_record = case_level.GetCaseRecord(record_number);

        if( case_record.HasOccurrences() )
        {
            json_writer.Key(case_record.GetCaseRecordMetadata().GetDictRecord().GetName());
            WriteCaseRecord(json_writer, case_level.GetCaseRecord(record_number));
        }
    }

    if( case_level.GetNumberChildCaseLevels() > 0 )
    {
        const size_t child_case_level_number = case_level.GetCaseLevelMetadata().GetDictLevel().GetLevelNumber() + 1;
        json_writer.BeginArray(SyncCaseV2JsonSerializer::GetJsonKeyForCaseLevel(child_case_level_number));

        for( size_t level_index = 0; level_index < case_level.GetNumberChildCaseLevels(); ++level_index )
        {
            const CaseLevel& child_case_level = case_level.GetChildCaseLevel(level_index);
            ASSERT(child_case_level_number == child_case_level.GetCaseLevelMetadata().GetDictLevel().GetLevelNumber());

            WriteCaseLevel(json_writer, child_case_level);
        }

        json_writer.EndArray();
    }

    json_writer.EndObject();
}


void SyncCaseV2JsonWriter::WriteCaseRecord(JsonWriter& json_writer, const CaseRecord& case_record)
{
    // write multiply-occurring records as arrays
    const bool write_as_array = ( case_record.GetCaseRecordMetadata().GetDictRecord().GetMaxRecs() > 1 );

    if( write_as_array )
        json_writer.BeginArray();

    for( CaseItemIndex index = case_record.GetCaseItemIndex();
         index.GetRecordOccurrence() < case_record.GetNumberOccurrences();
         index.IncrementRecordOccurrence() )
    {
        json_writer.BeginObject();

        for( const CaseItem* const case_item : case_record.GetCaseItems() )
            WriteCaseItem(json_writer, *case_item, index);

        json_writer.EndObject();
    }

    if( write_as_array )
        json_writer.EndArray();
}


void SyncCaseV2JsonWriter::WriteCaseItem(JsonWriter& json_writer, const CaseItem& case_item, CaseItemIndex& index)
{
    // determine how many occurrences are defined
    for( index.SetItemSubitemOccurrence(case_item, case_item.GetTotalNumberItemSubitemOccurrences() - 1);
         index.GetItemSubitemOccurrence(case_item) < case_item.GetTotalNumberItemSubitemOccurrences();
         index.DecrementItemSubitemOccurrence(case_item) )
    {
        if( !case_item.IsBlank(index) )
            break;
    }

    const size_t defined_occurrences = index.GetItemSubitemOccurrence(case_item) + 1;

    if( defined_occurrences == 0 )
        return;

    json_writer.Key(case_item.GetDictItem().GetName());

    // write multiply-occurring items as arrays
    const bool write_as_array = ( case_item.GetTotalNumberItemSubitemOccurrences() > 1 );

    if( write_as_array )
        json_writer.BeginArray();

    for( index.SetItemSubitemOccurrence(case_item, 0);
         index.GetItemSubitemOccurrence(case_item) < defined_occurrences;
         index.IncrementItemSubitemOccurrence(case_item) )
    {
        switch( case_item.GetDataType() )
        {
            case DataType::Numeric:
                WriteCaseItemValue(json_writer, assert_cast<const NumericCaseItem&>(case_item), index);
                break;

            case DataType::String:
                WriteCaseItemValue(json_writer, assert_cast<const StringCaseItem&>(case_item), index);
                break;

            case DataType::Binary:
                WriteCaseItemValue(json_writer, assert_cast<const BinaryCaseItem&>(case_item), index);
                break;

            default:
                ASSERT(false);
        }
    }

    if( write_as_array )
        json_writer.EndArray();
}


void SyncCaseV2JsonWriter::WriteCaseItemValue(JsonWriter& json_writer, const NumericCaseItem& numeric_case_item, const CaseItemIndex& index)
{
    const double value = numeric_case_item.GetValueForOutput(index);

    if( IsSpecial(value) )
    {
        // write NOTAPPL as an empty string
        json_writer.Write(( value == NOTAPPL ) ? "" : SpecialValues::ValueToString(value));
    }

    else if( numeric_case_item.GetDictItem().GetDecimal() > 0 )
    {
        json_writer.Write(value);
    }

    else
    {
        json_writer.Write(static_cast<int64_t>(value));
    }
}


void SyncCaseV2JsonWriter::WriteCaseItemValue(JsonWriter& json_writer, const StringCaseItem& string_case_item, const CaseItemIndex& index)
{
    ASSERT(string_case_item.IsFixedWidth());

    const std::string& value = string_case_item.GetValue(index);
    json_writer.Write(SO::TrimRightSpace(value));
}


void SyncCaseV2JsonWriter::WriteCaseItemValue(JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
{
    json_writer.BeginObject();

    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);

    if( binary_data_accessor.IsDefined() )
    {
        json_writer.Write(JK::metadata, binary_data_accessor.GetBinaryDataMetadata())
                   .Write(JK::caseid, index.GetCase().GetUuid());

        SyncCaseJsonSerializer::WriteBinaryCaseItemSyncableDetails(json_writer, binary_case_item, index, JK::length);
    }

    json_writer.EndObject();
}



// --------------------------------------------------------------------------
// SyncCaseV2JsonParser
// --------------------------------------------------------------------------

SyncCaseV2JsonParser::SyncCaseV2JsonParser(std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper)
    :   m_caseJsonParserHelper(std::move(case_json_parser_helper))
{
    ASSERT(m_caseJsonParserHelper != nullptr && m_caseJsonParserHelper->GetCaseAccess() != nullptr);
}


SyncCaseV2JsonParser::~SyncCaseV2JsonParser()
{
}


void SyncCaseV2JsonParser::ParseCase(Case& data_case, const JsonNode& json_node)
{
    // set case values
    data_case.SetPositionInRepository(json_node.GetOrDefault<double>(JK::position, -1));
    data_case.SetUuid(json_node.Get<std::string>(JK::id));
    data_case.SetCaseLabel(json_node.GetOrConstruct<std::string>(JK::label));
    data_case.SetDeleted(json_node.GetOrDefault(JK::deleted, false));
    data_case.SetVerified(json_node.GetOrDefault(JK::verified, false));

    // notes
    {
        std::vector<Note>& notes = data_case.GetNotes();
        notes.clear();

        if( json_node.Contains(JK::notes) )
        {
            for( const JsonNode& note_node : json_node.GetArray(JK::notes) )
                notes.emplace_back(ParseNote(note_node));
        }
    }

    // clock
    data_case.SetVectorClock(json_node.Get<VectorClock>(JK::clock));

    // partial save status
    ParsePartialSave(json_node, data_case);

    // case data
    if( json_node.Contains(JK::data) )
    {
        ParseCaseDataAsTextLines(json_node.Get(JK::data), data_case);
    }

    else
    {
        ParseCaseData(json_node.Get(SyncCaseV2JsonSerializer::GetJsonKeyForCaseLevel(0)), data_case);
    }

    ASSERT(data_case.GetKey() == json_node.Get<std::string>(JK::caseids));
}


template<typename T/* = NamedReference*/>
std::unique_ptr<T> SyncCaseV2JsonParser::ParseNamedReference(const JsonNode& named_reference_node) const
{
    const std::string name = named_reference_node.Get<std::string>(JK::name);
    std::string level_key = named_reference_node.GetOrConstruct<std::string>(JK::levelKey);

    // occurrences are written as one-based, though 0 will be written for item/subitem occurrences that do not apply
    auto get_occurrence = [&](const std::string_view key_sv) -> size_t
    {
        const std::optional<size_t> occurrence = named_reference_node.GetOptional<size_t>(key_sv);
        return ( occurrence.has_value() && *occurrence >= 1 ) ? ( *occurrence - 1 ) : 0;
    };

    const size_t occurrences[3] =
    {
        get_occurrence(JK::recordOccurrence),
        get_occurrence(JK::itemOccurrence),
        get_occurrence(JK::subitemOccurrence)
    };

    if constexpr(std::is_same_v<T, CaseItemReference>)
    {
        return CaseConstructionHelpers::CreateCaseItemReference(*m_caseJsonParserHelper->GetCaseAccess(), std::move(level_key), name, occurrences);
    }

    else
    {
        return CaseConstructionHelpers::CreateNamedReference(*m_caseJsonParserHelper->GetCaseAccess(), std::move(level_key), name, occurrences);
    }
}


Note SyncCaseV2JsonParser::ParseNote(const JsonNode& note_node) const
{
    return Note(note_node.Get<std::string>(JK::content),
                ParseNamedReference(note_node.Get(JK::field)),
                note_node.Get<std::string>(JK::operatorId),
                note_node.GetDate(JK::modifiedTime));
}


void SyncCaseV2JsonParser::ParsePartialSave(const JsonNode& json_node, Case& data_case) const
{
    PartialSaveMode partial_save_mode = PartialSaveMode::None;
    std::shared_ptr<CaseItemReference> partial_save_case_item_reference;

    if( json_node.Contains(JK::partialSave) )
    {
        const JsonNode partial_save_node = json_node.Get(JK::partialSave);
        partial_save_mode = partial_save_node.GetOrDefault(JK::mode, partial_save_mode);

        if( partial_save_mode != PartialSaveMode::None )
            partial_save_case_item_reference = ParseNamedReference<CaseItemReference>(partial_save_node.Get(JK::field));
    }

    data_case.SetPartialSaveStatus(partial_save_mode, std::move(partial_save_case_item_reference));
}


void SyncCaseV2JsonParser::ParseCaseDataAsTextLines(const JsonNode& data_node, Case& data_case)
{
    std::string case_lines;

    for( const JsonNode& line_node : data_node.GetArrayOrEmpty() )
        SO::AppendWithSeparator(case_lines, line_node.Get<std::string>(), "\r\n");

    if( m_textToCaseConverter == nullptr )
        m_textToCaseConverter = std::make_unique<TextToCaseConverter>(data_case.GetCaseMetadata());

    m_textToCaseConverter->TextUtf8ToCase(data_case, case_lines.c_str(), case_lines.length());
}


void SyncCaseV2JsonParser::ParseCaseData(const JsonNode& level_1_node, Case& data_case) const
{
    CaseLevel& root_case_level = data_case.GetRootCaseLevel();
    root_case_level.Reset();

    if( level_1_node.IsString() )
    {
        ParseCaseLevel(Json::Parse(level_1_node.Get<std::string>()), root_case_level);
    }

    else
    {
        ParseCaseLevel(level_1_node, root_case_level);
    }
}


void SyncCaseV2JsonParser::ParseCaseLevel(const JsonNode& case_level_node, CaseLevel& case_level) const
{
    ASSERT(case_level.GetNumberChildCaseLevels() == 0);

    if( case_level_node.IsEmpty() )
        return;

    CaseConstructionReporter* const case_construction_reporter = case_level.GetCase().GetCaseConstructionReporter();

    if( case_construction_reporter != nullptr )
        case_construction_reporter->IncrementCaseLevelCount(case_level.GetCaseLevelMetadata().GetDictLevel().GetLevelNumber());

    // parse the IDs
    CaseRecord& id_case_record = case_level.GetIdCaseRecord();
    CaseItemIndex id_index = id_case_record.GetCaseItemIndex();
    ParseCaseItemsOnCaseRecord(case_level_node.Get(JK::id), id_case_record, id_index);

    // parse the records
    for( size_t record_number = 0; record_number < case_level.GetNumberCaseRecords(); ++record_number )
    {
        CaseRecord& case_record = case_level.GetCaseRecord(record_number);

        const JsonNode case_records_node = case_level_node.GetOrEmpty(case_record.GetCaseRecordMetadata().GetDictRecord().GetName());
        ParseCaseRecords(case_records_node, case_record);
    }

    // parse any child levels
    const CaseLevelMetadata* const child_case_level_metadata = case_level.GetCaseLevelMetadata().GetChildCaseLevelMetadata();

    if( child_case_level_metadata != nullptr )
    {
        const size_t child_case_level_number = child_case_level_metadata->GetDictLevel().GetLevelNumber();

        for( const JsonNode& child_case_level_node_array_element : case_level_node.GetArrayOrEmpty(SyncCaseV2JsonSerializer::GetJsonKeyForCaseLevel(child_case_level_number)) )
        {
            CaseLevel& child_case_level = case_level.AddChildCaseLevel();
            ParseCaseLevel(child_case_level_node_array_element, child_case_level);
        }
    }
}


void SyncCaseV2JsonParser::ParseCaseRecords(const JsonNode& case_records_node, CaseRecord& case_record) const
{
    ASSERT(case_record.GetNumberOccurrences() == 0);

    if( case_records_node.IsEmpty() )
        return;

    CaseConstructionReporter* const case_construction_reporter = case_record.GetCaseLevel().GetCase().GetCaseConstructionReporter();

    const size_t max_records = case_record.GetCaseRecordMetadata().GetDictRecord().GetMaxRecs();
    CaseItemIndex index = case_record.GetCaseItemIndex();
    size_t records_added = 0;

    auto process_record = [&](const JsonNode& case_record_node)
    {
        // issue a warning (and return ) when there are too many records
        if( records_added == max_records )
        {
            if( case_construction_reporter != nullptr )
            {
                case_construction_reporter->TooManyRecordOccurrences(case_record.GetCaseLevel().GetCase(),
                                                                     case_record.GetCaseRecordMetadata().GetDictRecord().GetName(),
                                                                     max_records);
            }

            return;
        }

        // add the record occurrence
        case_record.SetNumberOccurrences(++records_added);

        if( records_added != 1 )
            index.IncrementRecordOccurrence();

        if( case_construction_reporter != nullptr )
            case_construction_reporter->IncrementRecordCount();

        // parse the record's items
        ParseCaseItemsOnCaseRecord(case_record_node, case_record, index);
    };

    if( case_records_node.IsArray() )
    {
        for( const JsonNode& case_record_node_array_element : case_records_node.GetArray() )
            process_record(case_record_node_array_element);
    }

    else
    {
        process_record(case_records_node);
    }
}


void SyncCaseV2JsonParser::ParseCaseItemsOnCaseRecord(const JsonNode& case_record_node, CaseRecord& case_record, CaseItemIndex& index) const
{
    ASSERT(( index.GetRecordOccurrence() + 1 ) == case_record.GetNumberOccurrences());

    for( const CaseItem* const case_item : case_record.GetCaseItems() )
    {
        const CDictItem& dict_item = case_item->GetDictItem();

        const JsonNode case_item_node = case_record_node.GetOrEmpty(dict_item.GetName());

        if( case_item_node.IsEmpty() )
            continue;

        // allow values to be stored in an array, even for items without occurrences
        if( case_item_node.IsArray() )
        {
            size_t occurrences_processed = 0;
            size_t max_occurrences = dict_item.GetItemSubitemOccurs();

            for( const JsonNode& case_item_node_array_element : case_item_node.GetArray() )
            {
                index.SetItemSubitemOccurrence(*case_item, occurrences_processed);
                ParseCaseItem(case_item_node_array_element, *case_item, index);

                if( ++occurrences_processed == max_occurrences )
                    break;
            }
        }

        else
        {
            index.SetItemSubitemOccurrence(*case_item, 0);
            ParseCaseItem(case_item_node, *case_item, index);
        }
    }
}


void SyncCaseV2JsonParser::ParseCaseItem(const JsonNode& case_item_node, const CaseItem& case_item, CaseItemIndex& index) const
{
    // the case item should be blank (unless it is a subitem that already has a value from the parent being set)
    ASSERT(case_item.IsBlank(index) || case_item.GetDictItem().GetParentItem() != nullptr);

    if( IsNumeric(case_item.GetDataType()) )
    {
        ParseNumericCaseItem(case_item_node, assert_cast<const NumericCaseItem&>(case_item), index);
    }

    else if( IsString(case_item.GetDataType()) )
    {
        ParseStringCaseItem(case_item_node, assert_cast<const StringCaseItem&>(case_item), index);
    }

    else if( IsBinary(case_item.GetDataType()) )
    {
        m_caseJsonParserHelper->ParseBinaryCaseItem(assert_cast<const BinaryCaseItem&>(case_item), index, case_item_node);
    }

    else
    {
        ASSERT(false);
    }
}


void SyncCaseV2JsonParser::ParseNumericCaseItem(const JsonNode& case_item_node, const NumericCaseItem& numeric_case_item, CaseItemIndex& index) const
{
    std::optional<double> numeric_value = case_item_node.GetOptional<double>();

    if( numeric_value.has_value() )
    {
        numeric_case_item.SetValueFromInput(index, *numeric_value);
    }

    else
    {
        const std::string string_value = case_item_node.Get<std::string>();

        if( string_value.empty() )
        {
            numeric_case_item.SetValue(index, NOTAPPL);
        }

        else
        {
            numeric_value = SpecialValues::StringIsSpecial<std::optional<double>>(string_value);
            numeric_case_item.SetValue(index, numeric_value.value_or(DEFAULT));
        }
    }
}


void SyncCaseV2JsonParser::ParseStringCaseItem(const JsonNode& case_item_node, const StringCaseItem& string_case_item, CaseItemIndex& index) const
{
    string_case_item.SetValue(index, case_item_node.Get<std::string>());
}
