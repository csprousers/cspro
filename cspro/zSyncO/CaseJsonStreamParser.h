#pragma once

#include <zSyncO/zSyncO.h>
#include <zSyncO/JsonStreamParser.h>
#include <zToolsO/NumberConverter.h>
#include <zUtilO/BinaryDataAccessor.h>
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseConstructionHelpers.h>
#include <zCaseO/CaseConstructionReporter.h>
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zCaseO/TextToCaseConverter.h>
#include <zDataO/SyncJsonBinaryDataReader.h>


class CaseItemsType : public JsonStreamParser::DefaultType
{
public:
    CaseItemsType(const CaseItem& case_item):
        m_case_item(case_item),
        m_is_multiple_occurrence(false),
        m_occurrence(0),
        m_binary_item_is_metadata(false),
        m_binary_item_object_counter(0)
    {
    }

    void SetIndex(const CaseItemIndex& index)
    {
        m_index.emplace(index);
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StringEvent& e) override
    {
        m_index->SetItemSubitemOccurrence(m_case_item, m_occurrence++);

        if (m_case_item.IsTypeString()) {
            const auto& string_item = static_cast<const StringCaseItem&>(m_case_item);
            string_item.SetValue(*m_index, UTF8Convert::UTF8ToWide<CString>(e.value));
        }
        else if (m_case_item.IsTypeNumeric()) {
            // Try to convert to number
            const auto& numeric_item = static_cast<const NumericCaseItem&>(m_case_item);
            CString string_value = UTF8Convert::UTF8ToWide<CString>(e.value);
            if (string_value.Trim().IsEmpty()) {
                numeric_item.SetNotappl(*m_index);
            }
            else {
                numeric_item.SetValueFromInput(*m_index, StringToNumber(string_value));
            }
        }

        if (m_is_multiple_occurrence) {
            return JsonStreamParser::KeepCurrentType{};
        }
        else {
            return JsonStreamParser::PopType{ std::any() };
        }
    }

    JsonStreamParser::Action HandleNumeric(double value)
    {
        m_index->SetItemSubitemOccurrence(m_case_item, m_occurrence++);
        if (m_case_item.IsTypeNumeric()) {
            const auto& numeric_item = static_cast<const NumericCaseItem&>(m_case_item);
            numeric_item.SetValue(*m_index, value);
        }
        else if (m_case_item.IsTypeString()) {
            const auto& string_item = static_cast<const StringCaseItem&>(m_case_item);
            string_item.SetValue(*m_index, NumberToString(value));
        }

        if (m_is_multiple_occurrence) {
            return JsonStreamParser::KeepCurrentType{};
        }
        else {
            return JsonStreamParser::PopType{ std::any() };
        }
    }

    JsonStreamParser::Action Handle(JsonStreamParser::DoubleEvent& e) override
    {
        return HandleNumeric(e.value);
    }

    JsonStreamParser::Action Handle(JsonStreamParser::IntEvent& e) override
    {
        return HandleNumeric((double)e.value);
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartArrayEvent&) override
    {
        m_is_multiple_occurrence = true;
        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::EndArrayEvent&) override
    {
        return JsonStreamParser::PopType{ std::any() };
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartObjectEvent&) override
    {
        if (m_case_item.IsTypeBinary()) {
            m_binary_item_object_counter++;
        }
        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::EndObjectEvent&) override
    {
        //This method assumes only binary items are written out as json objects
        //when endobject event is triggered, if binary item has metadata tag we need to finish reading the binary item informationn
        //so return the same type and set the metadata flag as false
        if (m_case_item.IsTypeBinary()) {
            //decrement the counter on ending an object
            m_binary_item_object_counter--;

            //set the flag to false
            m_binary_item_is_metadata = false;

            //if binary item has other json objects keep current type
            if (m_binary_item_object_counter > 0) {
                return JsonStreamParser::KeepCurrentType{};
            }

            //we are done reading the current binary object. set the binary data of the binary case item if there was actually binary data
            std::unique_ptr<BinaryDataMetadata> binary_data_metadata = std::move(m_binaryDataMetadata);

            if (binary_data_metadata != nullptr && !binary_data_metadata->GetBinaryDataKey().empty()) {
                const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(m_case_item);
                BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(*m_index);           
                ASSERT(binary_data_accessor.GetBinaryDataReader() == nullptr);

                binary_data_accessor.SetBinaryDataReader(std::make_unique<SyncJsonBinaryDataReader>(std::move(*binary_data_metadata)));
            }
        }

        //if the item has multiple occurrences continue reading the other binary item objects if not pop any type
        if (m_is_multiple_occurrence) {
            return JsonStreamParser::KeepCurrentType{};
        }
        else {
            return JsonStreamParser::PopType{ std::any() };
        }
    }

    JsonStreamParser::Action Handle(JsonStreamParser::KeyEvent& e) override
    {
        m_current_key = e.key;
        if (m_case_item.IsTypeBinary()) {
            if (m_current_key == "metadata") {
                m_binary_item_is_metadata = true;
                return JsonStreamParser::KeepCurrentType{};
            }
            else if (!m_binary_item_is_metadata && m_current_key == "length") {
                return JsonStreamParser::PushType{ typeid(int64_t) };
            }
        }
        return JsonStreamParser::PushType{ typeid(CString) };
    }

    JsonStreamParser::Action Handle(JsonStreamParser::ChildCompletedEvent& e) override
    {
        if (m_case_item.IsTypeBinary()) {
            //currently ignoring length, caseid. Signature is written as metadata property 
            if (!m_binary_item_is_metadata && (m_current_key == "caseid" || m_current_key == "length")) {
                return JsonStreamParser::KeepCurrentType{};
            }

            if( m_binaryDataMetadata == nullptr ) {
                m_binaryDataMetadata = std::make_unique<BinaryDataMetadata>();
            }

            if (!m_binary_item_is_metadata && m_current_key == "signature") {
                m_index->SetItemSubitemOccurrence(m_case_item, m_occurrence++);
                m_binaryDataMetadata->SetBinaryDataKey(CS2WS(std::any_cast<CString>(e.child_value)));
            }
            else {
                m_binaryDataMetadata->SetProperty(CS2WS(m_current_key.c_str()), CS2WS(std::any_cast<CString>(e.child_value)));
            }
        }
        return JsonStreamParser::KeepCurrentType{};
    }

    void Reset() override
    {
        m_binaryDataMetadata.reset();
        m_occurrence = 0;
        m_is_multiple_occurrence = false;
        m_binary_item_is_metadata = false;
        m_binary_item_object_counter = 0;
    }

private:
    const CaseItem& m_case_item;
    std::string m_current_key;
    std::optional<CaseItemIndex> m_index;
    std::unique_ptr<BinaryDataMetadata> m_binaryDataMetadata;
    size_t m_occurrence;
    bool m_is_multiple_occurrence;
    bool m_binary_item_is_metadata;
    size_t m_binary_item_object_counter;
};


class RecordItemsType : public JsonStreamParser::DefaultType
{
public:
    RecordItemsType(const CaseRecordMetadata& metadata) :
        m_metadata(metadata)
    {
        for (const CaseItem* case_item : metadata.GetCaseItems()) {
            m_item_map[UTF8Convert::WideToUTF8(case_item->GetDictionaryItem().GetName())] = std::make_shared<CaseItemsType>(*case_item);
        }
    }

    void SetCaseRecord(CaseRecord* record, int occurrence)
    {
        m_index.emplace(record->GetCaseItemIndex(occurrence));
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartObjectEvent&) override
    {
        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::KeyEvent& e) override
    {
        const auto& type = m_item_map.find(e.key);
        if (type != m_item_map.end()) {
            type->second->SetIndex(*m_index);
            return JsonStreamParser::PushTypeHandler{ type->second };
        }
        else {
            return JsonStreamParser::IgnoreValue{};
        }
    }

    JsonStreamParser::Action Handle(JsonStreamParser::ChildCompletedEvent&) override
    {
        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::EndObjectEvent&) override
    {
        return JsonStreamParser::PopType{ std::any() };
    }

private:
    const CaseRecordMetadata& m_metadata;
    std::optional<CaseItemIndex> m_index;
    std::map<std::string, std::shared_ptr<CaseItemsType>, std::less<>> m_item_map;
};


class RecordType : public JsonStreamParser::DefaultType
{
public:
    RecordType(const CaseRecordMetadata& metadata) :
        m_metadata(metadata),
        m_record(nullptr),
        m_items_type(std::make_shared<RecordItemsType>(metadata)),
        m_occurences(Occurrences::DONTKNOWYET),
        m_occurrence(0)
    {}

    void SetCaseRecord(CaseRecord* record)
    {
        m_record = record;
    }

    const CaseRecordMetadata& Metadata()
    {
        return m_metadata;
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartArrayEvent&) override
    {
        if (m_occurences == Occurrences::DONTKNOWYET)
            m_occurences = Occurrences::MULTIPLE;
        else
            return JsonStreamParser::ParseError{ L"Unexpected '[' in data record`" };

        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::ChildCompletedEvent&) override
    {
        if (m_occurences == Occurrences::SINGLE)
            return JsonStreamParser::Action(JsonStreamParser::PopType{ std::any() });
        else
            return JsonStreamParser::Action(JsonStreamParser::KeepCurrentType{});
    }

    JsonStreamParser::Action Handle(JsonStreamParser::EndArrayEvent&) override
    {
        return JsonStreamParser::Action(JsonStreamParser::PopType{ std::any() });
    }

    bool IsIdRecord(const CaseRecord& record)
    {
        return &record.GetCaseRecordMetadata() == record.GetCaseRecordMetadata().GetCaseLevelMetadata().GetIdCaseRecordMetadata();
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartObjectEvent&) override
    {
        if (m_occurences == Occurrences::DONTKNOWYET)
            m_occurences = Occurrences::SINGLE;

        if (IsIdRecord(*m_record) || IncrementRecordOccurrences(*m_record)) {
            m_items_type->SetCaseRecord(m_record, m_occurrence++);
            return JsonStreamParser::Action(JsonStreamParser::PushTypeHandlerAndResendEvent{ m_items_type });
        }
        else {
            return JsonStreamParser::Action(JsonStreamParser::IgnoreValue{});
        }
    }

    bool IncrementRecordOccurrences(CaseRecord& record)
    {
        auto& data_case = record.GetCaseLevel().GetCase();
        auto error_reporter = data_case.GetCaseConstructionReporter();
        if (error_reporter)
            error_reporter->IncrementRecordCount();

        size_t num_occs = record.GetNumberOccurrences() + 1;

        if (num_occs <= record.GetCaseRecordMetadata().GetDictionaryRecord().GetMaxRecs())
        {
            record.SetNumberOccurrences(num_occs);
            return true;
        }
        else if (error_reporter)
        {
            error_reporter->TooManyRecordOccurrences(data_case, record.GetCaseRecordMetadata().GetDictionaryRecord().GetName(),
                record.GetCaseRecordMetadata().GetDictionaryRecord().GetMaxRecs());
        }

        return false;
    }

    JsonStreamParser::Action Handle(JsonStreamParser::EndObjectEvent&) override
    {
        return JsonStreamParser::Action(JsonStreamParser::PopType{ std::any() });
    }

    void Reset() override
    {
        m_occurrence = 0;
        m_occurences = Occurrences::DONTKNOWYET;
    }

private:
    const CaseRecordMetadata& m_metadata;
    CaseRecord* m_record;
    int m_occurrence;
    std::shared_ptr<RecordItemsType> m_items_type;
    enum class Occurrences {
        DONTKNOWYET, SINGLE, MULTIPLE
    };
    Occurrences m_occurences;
};


// Dummy type needed since parser wants a top level type
// but we are using it in a way that it doesn't actually
// return anything when it is used to parse stringified level.
struct CaseLevelPlaceholder {
};


class LevelNodeListType;

class LevelType : public JsonStreamParser::DefaultType
{
public:
    using ValueType = CaseLevelPlaceholder;

    LevelType(const CaseLevelMetadata& level_meta);

    void SetLevel(CaseLevel* level)
    {
        m_level = level;
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartObjectEvent&) override
    {
        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::KeyEvent& e) override;

    JsonStreamParser::Action Handle(JsonStreamParser::ChildCompletedEvent&) override
    {
        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::EndObjectEvent&) override
    {
        if (m_level->GetCase().GetCaseConstructionReporter())
            m_level->GetCase().GetCaseConstructionReporter()->IncrementCaseLevelCount(m_level->GetCaseLevelMetadata().GetDictLevel().GetLevelNumber());

        return JsonStreamParser::PopType{ CaseLevelPlaceholder{} };
    }

private:
    std::map<std::string, std::shared_ptr<RecordType>, std::less<>> m_record_map;
    std::shared_ptr<LevelNodeListType> m_child_level_nodes_type;
    CaseLevel* m_level;
};


class Level1Type : public JsonStreamParser::DefaultType
{
public:
    Level1Type(const CaseLevelMetadata& level1_metadata) :
        m_level_type(std::make_shared<LevelType>(level1_metadata))
    {
        m_level_parser.GetTypeRegistry().Put(m_level_type);
    }

    void SetLevel(CaseLevel* level)
    {
        m_level_type->SetLevel(level);
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartObjectEvent&) override
    {
        // level is encoded as regular JSON object
        return JsonStreamParser::PushTypeHandlerAndResendEvent{ m_level_type };
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StringEvent& e) override
    {
        // level is JSON but encoded as JSON string value
        m_level_parser.Reset();
        m_level_parser.Parse(e.value);
        m_level_parser.EndParse();

        return JsonStreamParser::PopType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::ChildCompletedEvent&) override
    {
        return JsonStreamParser::PopType{ std::any() };
    }

private:
    std::shared_ptr<LevelType> m_level_type;
    JsonStreamParser::Parser<LevelType> m_level_parser;
};


class LevelNodeListType : public JsonStreamParser::DefaultType
{
public:
    LevelNodeListType(const CaseLevelMetadata& level_metadata) :
        m_child_level_type(std::make_shared<LevelType>(level_metadata)),
        m_parent_level(nullptr)
    {}

    void SetParentLevel(CaseLevel* level)
    {
        m_parent_level = level;
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartArrayEvent&) override
    {
        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::ChildCompletedEvent&) override
    {
        return JsonStreamParser::Action(JsonStreamParser::KeepCurrentType{});
    }

    JsonStreamParser::Action Handle(JsonStreamParser::EndArrayEvent&) override
    {
        return JsonStreamParser::Action(JsonStreamParser::PopType{ std::any() });
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartObjectEvent&) override
    {
        CaseLevel& child_level = m_parent_level->AddChildCaseLevel();
        m_child_level_type->SetLevel(&child_level);
        return JsonStreamParser::Action(JsonStreamParser::PushTypeHandlerAndResendEvent{ m_child_level_type });
    }

    JsonStreamParser::Action Handle(JsonStreamParser::EndObjectEvent&) override
    {
        return JsonStreamParser::Action(JsonStreamParser::PopType{ std::any() });
    }

private:
    CaseLevel* m_parent_level;
    std::shared_ptr<LevelType> m_child_level_type;
};


inline LevelType::LevelType(const CaseLevelMetadata& level_meta):
        m_level(nullptr)
{
    m_record_map["id"] = std::make_shared<RecordType>(*level_meta.GetIdCaseRecordMetadata());
    for (const auto& record_meta : level_meta.GetCaseRecordsMetadata())
    {
        m_record_map[UTF8Convert::WideToUTF8(record_meta->GetDictionaryRecord().GetName())] = std::make_shared<RecordType>(*record_meta);
    }

    if (level_meta.GetChildCaseLevelMetadata()) {
        m_child_level_nodes_type = std::make_shared<LevelNodeListType>(*level_meta.GetChildCaseLevelMetadata());
    }
}


inline JsonStreamParser::Action LevelType::Handle(JsonStreamParser::KeyEvent& e)
{
    const auto& type = m_record_map.find(e.key);
    if (type != m_record_map.end()) {
        CaseRecord& case_record = m_level->GetCaseRecord(type->second->Metadata());
        type->second->SetCaseRecord(&case_record);
        return JsonStreamParser::PushTypeHandler{ type->second };
    } else if (e.key.substr(0, 6) == "level-" && m_child_level_nodes_type) {
        m_child_level_nodes_type->SetParentLevel(m_level);
        return JsonStreamParser::PushTypeHandler{ m_child_level_nodes_type };
    }
    else {
        return JsonStreamParser::IgnoreValue{};
    }
}


class CaseType : public JsonStreamParser::DefaultType
{
public:
    using ValueType = std::shared_ptr<Case>;

    struct VectorClockEntry
    {
        CString deviceId;
        int revision;
    };

    struct PartialSave {
        PartialSaveMode mode;
        std::shared_ptr<CaseItemReference> field;
    };

    CaseType(const CaseAccess& case_access, std::shared_ptr<CaseConstructionReporter> case_construction_reporter) :
        m_case_access(case_access),
        m_text_to_case_converter(case_access.GetCaseMetadata()),
        m_started(false),
        m_level1_type(std::make_shared<Level1Type>(*case_access.GetCaseMetadata().GetCaseLevelsMetadata().front())),
        m_case_construction_reporter(case_construction_reporter)
    {
    }

    JsonStreamParser::Action Handle(JsonStreamParser::StartObjectEvent&) override
    {
        if (m_started)
            return JsonStreamParser::ParseError{ "Expecting key or value but got '{'" };
        m_started = true;
        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::KeyEvent& e) override
    {
        if (!m_started)
            return JsonStreamParser::ParseError{ "Expecting '{'" };
        m_current_field = e.key;
        if (m_current_field == "id") {
            return JsonStreamParser::PushType{ typeid(CString) };
        } else if (m_current_field == "label") {
            return JsonStreamParser::PushType{ typeid(CString) };
        } else if (m_current_field == "caseids") {
            return JsonStreamParser::PushType{ typeid(CString) };
        } else if (m_current_field == "data") {
            return JsonStreamParser::PushType{ typeid(std::vector<CString>) };
        } else if (m_current_field == "verified") {
            return JsonStreamParser::PushType{ typeid(bool) };
        } else if (m_current_field == "deleted") {
            return JsonStreamParser::PushType{ typeid(bool) };
        } else if (m_current_field == "notes") {
            return JsonStreamParser::PushType{ typeid(std::vector<Note>) };
        } else if (m_current_field == "partialSave") {
            return JsonStreamParser::PushType{ typeid(PartialSave) };
        } else if (m_current_field == "clock") {
            return JsonStreamParser::PushType{ typeid(std::vector<VectorClockEntry>) };
        } else if (m_current_field == "level-1") {
            m_level1_type->SetLevel(&m_current_case->GetRootCaseLevel());
            return JsonStreamParser::PushTypeHandler{ m_level1_type };
        } else {
            return JsonStreamParser::IgnoreValue{};
        }
    }

    JsonStreamParser::Action Handle(JsonStreamParser::ChildCompletedEvent& e) override
    {
        if (!m_started)
            return JsonStreamParser::ParseError{ "Expecting '{'" };

        if (m_current_field == "id") {
            m_current_case->SetUuid(CS2WS(std::any_cast<CString>(e.child_value)));
        }
        else if (m_current_field == "label") {
            m_current_case->SetCaseLabel(std::any_cast<CString>(e.child_value));
        }
        else if (m_current_field == "caseids") {
            // ignore - caseids extracted from data
        }
        else if (m_current_field == "data") {
            const auto& data_lines = *std::any_cast<std::vector<CString>>(&e.child_value);
            CString data;
            for (const CString& line : data_lines) {
                if (!data.IsEmpty())
                    data += L"\r\n";
                data += line;
            }
            m_text_to_case_converter.TextWideToCase(*m_current_case, data, data.GetLength());
        }
        else if (m_current_field == "verified") {
            m_current_case->SetVerified(std::any_cast<bool>(e.child_value));
        }
        else if (m_current_field == "deleted") {
            m_current_case->SetDeleted(std::any_cast<bool>(e.child_value));
        }
        else if (m_current_field == "notes") {
            m_current_case->SetNotes(std::move(*std::any_cast<std::vector<Note>>(&e.child_value)));
        }
        else if (m_current_field == "partialSave") {
            auto partial_save = std::any_cast<PartialSave>(&e.child_value);
            m_current_case->SetPartialSaveStatus(partial_save->mode, std::move(partial_save->field));
        }
        else if (m_current_field == "clock") {
            auto clock_entries = std::any_cast<std::vector<VectorClockEntry>>(&e.child_value);
            VectorClock clock;
            for (auto& entry : *clock_entries)
                clock.setVersion(entry.deviceId, entry.revision);
            m_current_case->SetVectorClock(clock);
        }

        return JsonStreamParser::KeepCurrentType{};
    }

    JsonStreamParser::Action Handle(JsonStreamParser::EndObjectEvent&) override
    {
        if (!m_started)
            return JsonStreamParser::ParseError{ "Got '}' before '{'" };

        if (m_current_case->GetUuid().IsEmpty()) {
            return JsonStreamParser::ParseError{ "Case missing id" };
        }

        return JsonStreamParser::PopType{ std::make_any<std::shared_ptr<Case>>(m_current_case) };
    }

    void Reset() override
    {
        m_started = false;
        m_current_case = m_case_access.CreateCase();
        if (m_case_construction_reporter)
            m_current_case->SetCaseConstructionReporter(m_case_construction_reporter);
    }

private:
    bool m_started;
    std::string m_current_field;
    const CaseAccess& m_case_access;
    std::shared_ptr<Case> m_current_case;
    TextToCaseConverter m_text_to_case_converter;
    std::shared_ptr<Level1Type> m_level1_type;
    std::shared_ptr<CaseConstructionReporter> m_case_construction_reporter;
};


///<summary>
///Incrementally parses text stream containing list of cases in JSON format.
///</summary>
class CaseJsonStreamParser {
public:

    CaseJsonStreamParser(const CaseAccess& case_access, std::shared_ptr<CaseConstructionReporter> case_construction_reporter = nullptr);

    ///<summary>
    ///Parse json text and send events to subscribers.
    ///Call this multiple times until data is finished then call EndParse().
    ///</summary>
    std::vector<std::shared_ptr<Case>> Parse(const std::string_view& json_string)
    {
        return m_parser.Parse(json_string);
    }

    ///<summary>
    ///Call this when all text to parse has been sent to Parse() method and there is no more input.
    ///May call subscribers to notify completion or error.
    ///</summary>
    std::vector<std::shared_ptr<Case>> EndParse()
    {
        return m_parser.EndParse();
    }

private:
    const CaseAccess& m_case_access;
    JsonStreamParser::Parser<CaseType> m_parser;
};


inline CaseJsonStreamParser::CaseJsonStreamParser(const CaseAccess& case_access, std::shared_ptr<CaseConstructionReporter> case_construction_reporter) :
    m_case_access(case_access)
{
    auto case_type = std::make_shared<CaseType>(case_access, case_construction_reporter);
    m_parser.GetTypeRegistry().Put(case_type);

    auto partial_save_type = std::make_shared<JsonStreamParser::ObjectType<CaseType::PartialSave>>([](JsonStreamParser::ObjectFieldStorage& values) {
        auto field = values.Get<std::shared_ptr<CaseItemReference>>("field");
        return CaseType::PartialSave{ *values.Get<PartialSaveMode>("mode"), field ? std::move(*field) : nullptr };
        });
    partial_save_type->AddField<PartialSaveMode>("mode", true);
    partial_save_type->AddField<std::shared_ptr<CaseItemReference>>("field");
    m_parser.GetTypeRegistry().Put(partial_save_type);

    auto partial_save_mode_type = std::make_shared<JsonStreamParser::EnumType<PartialSaveMode>>([](std::string_view mode_string) -> std::optional<PartialSaveMode> {
        if (mode_string == "add")
            return PartialSaveMode::Add;
        else if (mode_string == "modify")
            return PartialSaveMode::Modify;
        else if (mode_string == "verify")
            return PartialSaveMode::Verify;
        else
            return std::nullopt;
        });
    m_parser.GetTypeRegistry().Put(partial_save_mode_type);

    auto json_data_to_case_item_ref = [&case_access](JsonStreamParser::ObjectFieldStorage& values) {

        CString& name = *values.Get<CString>("name");
        CString&& level_key = values.Get<CString>("levelKey", ""); // default to first level

        size_t occurrences[3] = {
            (size_t)std::max((int64_t) 0, values.Get<int64_t>("recordOccurrence", 1) - 1),
            (size_t)std::max((int64_t) 0, values.Get<int64_t>("itemOccurrence", 1) - 1),
            (size_t)std::max((int64_t) 0, values.Get<int64_t>("subitemOccurrence", 1) - 1) };

        return CaseConstructionHelpers::CreateCaseItemReference(case_access, level_key, name, occurrences);
    };

    auto case_item_reference_type = std::make_shared<JsonStreamParser::ObjectType<std::shared_ptr<CaseItemReference>>>(json_data_to_case_item_ref);
    case_item_reference_type->AddField<CString>("name", true);
    case_item_reference_type->AddField<CString>("levelKey");
    case_item_reference_type->AddField<int64_t>("recordOccurrence");
    case_item_reference_type->AddField<int64_t>("itemOccurrence");
    case_item_reference_type->AddField<int64_t>("subitemOccurrence");
    m_parser.GetTypeRegistry().Put(case_item_reference_type);

    auto json_data_to_named_item_ref = [&case_access](JsonStreamParser::ObjectFieldStorage& values) {

        CString& name = *values.Get<CString>("name");
        CString&& level_key = values.Get<CString>("levelKey", ""); // default to first level

        size_t occurrences[3] = {
            (size_t)std::max((int64_t) 0, values.Get<int64_t>("recordOccurrence", 1) - 1),
            (size_t)std::max((int64_t) 0, values.Get<int64_t>("itemOccurrence", 1) - 1),
            (size_t)std::max((int64_t) 0, values.Get<int64_t>("subitemOccurrence", 1) - 1) };

        return CaseConstructionHelpers::CreateNamedReference(case_access, level_key, name, occurrences);
    };
    auto named_reference_type = std::make_shared<JsonStreamParser::ObjectType<std::shared_ptr<NamedReference>>>(json_data_to_named_item_ref);
    named_reference_type->AddField<CString>("name", true);
    named_reference_type->AddField<CString>("levelKey");
    named_reference_type->AddField<int64_t>("recordOccurrence");
    named_reference_type->AddField<int64_t>("itemOccurrence");
    named_reference_type->AddField<int64_t>("subitemOccurrence");
    m_parser.GetTypeRegistry().Put(named_reference_type);

    auto note_type = std::make_shared<JsonStreamParser::ObjectType<Note>>([](JsonStreamParser::ObjectFieldStorage& values) {
        return Note(*values.Get<CString>("content"),
            *values.Get<std::shared_ptr<NamedReference>>("field"),
            *values.Get<CString>("operatorId"),
            values.Get<JsonStreamParser::Timestamp>("modifiedTime")->time);
        });
    note_type->AddField<CString>("content", true);
    note_type->AddField<std::shared_ptr<NamedReference>>("field", true);
    note_type->AddField<CString>("operatorId", true);
    note_type->AddField<JsonStreamParser::Timestamp>("modifiedTime", true);
    m_parser.GetTypeRegistry().Put(note_type);

    auto note_array_type = std::make_shared<JsonStreamParser::ArrayType<JsonStreamParser::ObjectType<Note>>>();
    m_parser.GetTypeRegistry().Put(note_array_type);

    auto vector_clock_entry_type = std::make_shared<JsonStreamParser::ObjectType<CaseType::VectorClockEntry>>([](JsonStreamParser::ObjectFieldStorage& values) {
        return CaseType::VectorClockEntry{ *values.Get<CString>("deviceId"), (int) *values.Get<int64_t>("revision") };
        });
    vector_clock_entry_type->AddField<CString>("deviceId", true);
    vector_clock_entry_type->AddField<int64_t>("revision", true);
    m_parser.GetTypeRegistry().Put(vector_clock_entry_type);

    auto vector_clock_entry_array_type = std::make_shared<JsonStreamParser::ArrayType<JsonStreamParser::ObjectType<CaseType::VectorClockEntry>>>();
    m_parser.GetTypeRegistry().Put(vector_clock_entry_array_type);
}
