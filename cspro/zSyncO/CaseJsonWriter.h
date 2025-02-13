#pragma once

#include <zToolsO/PortableFunctions.h>
#include <zToolsO/Utf8Convert.h>
#include <zAppO/SyncTypes.h>
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zCaseO/TextToCaseConverter.h>
#include <external/jsoncons/json.hpp>
#include <ostream>


class CaseJsonWriter 
{
public:

    std::ostream& ToJson(std::ostream& os, const Case& data_case)
    {
        jsoncons::compact_json_stream_encoder encoder(os, GetJsonOptions());
        WriteCase(encoder, data_case);
        return os;
    }

    std::string ToJson(const Case& data_case)
    {
        std::ostringstream os;
        ToJson(os, data_case);
        return os.str();
    }

    std::ostream& ToJson(std::ostream& os, const std::vector<std::shared_ptr<Case>>& cases)
    {
        jsoncons::compact_json_stream_encoder encoder(os, GetJsonOptions());
        encoder.begin_array();
        for (const auto& c : cases)
            WriteCase(encoder, *c);
        encoder.end_array();

        return os;
    }

    void WriteBinaryCaseItem(jsoncons::compact_json_stream_encoder& encoder, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
    {
        const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
        const BinaryDataMetadata* binary_data_metadata = nullptr;
        uint64_t length = 0;

        if( binary_data_accessor.IsDefined() )
        {
            try
            {
                binary_data_metadata = &binary_data_accessor.GetBinaryDataMetadata();
                ASSERT(!binary_data_metadata->GetBinaryDataKey().empty());

                length = binary_data_accessor.GetBinaryDataSize();
            }
            catch(...) { ASSERT(false); }
        }
        
        /* binary item json "MYBINARY_ITEM_NAME"
            {
                "length": 123,
                "signature" : "1136bedad7ce245c2330793d3c24f2ee"
                "caseid": "b9ce88e7-8df0-45ec-a523-4ab9f7e162d1",
                "metadata":{
                    "mime":"image/jpg"
                },
            }
        }*/

        encoder.key("length"); 
        encoder.uint64_value(length);

        encoder.key("signature");
        encoder.string_value(( binary_data_metadata != nullptr ) ? UTF8Convert::WideToUTF8(binary_data_metadata->GetBinaryDataKey()) :
                                                                   std::string_view());

        encoder.key("caseid");
        encoder.string_value(UTF8Convert::WideToUTF8(index.GetCase().GetUuid()));

        if( binary_data_metadata != nullptr )
        {
            encoder.key("metadata");
            encoder.begin_object();
            for( const auto& [attribute, value] : binary_data_metadata->GetProperties())
            {
                encoder.key(UTF8Convert::WideToUTF8(attribute));
                encoder.string_value(UTF8Convert::WideToUTF8(value));
            }
            encoder.end_object();
        }
    }

    std::ostream& ToJson(std::ostream& os, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
    {
        jsoncons::compact_json_stream_encoder encoder(os, GetJsonOptions());

        encoder.begin_object();
        //  Write binary case item.
        WriteBinaryCaseItem(encoder, binary_case_item, index);
        encoder.end_object();

        return os;
    }

    std::string ToJson(const std::vector<std::shared_ptr<Case>>& cases)
    {
        std::ostringstream os;
        ToJson(os, cases);
        return os.str();
    }

    std::string ToJson(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
    {
        std::ostringstream os;
        ToJson(os, binary_case_item, index);
        return os.str();
    }

    void SetCSPro74BackwardsCompatible(bool b)
    {
        m_backwards_compat = b;
    }

    void SetStringifyData(bool b)
    {
        m_stringify_data = b;
    }

private:
    bool m_backwards_compat = false;
    bool m_stringify_data = false;
    std::unique_ptr<TextToCaseConverter> m_text_case_converter;

    static jsoncons::json_options GetJsonOptions()
    {
        jsoncons::json_options options;
        options.spaces_around_colon(jsoncons::spaces_option::no_spaces);
        options.spaces_around_comma(jsoncons::spaces_option::no_spaces);
        return options;
    }

    template <typename EncoderType>
    void WriteCase(EncoderType& encoder, const Case& data_case)
    {
        encoder.begin_object();
        WriteField(encoder, "id", data_case.GetUuid());
        if (!data_case.GetCaseLabel().IsEmpty() || m_backwards_compat) {
            WriteField(encoder, "label", data_case.GetCaseLabel());
        }
        WriteField(encoder, "caseids", data_case.GetKey());
        if (data_case.GetDeleted() || m_backwards_compat) {
            WriteField(encoder, "deleted", data_case.GetDeleted());
        }
        if (data_case.GetVerified() || m_backwards_compat) {
            WriteField(encoder, "verified", data_case.GetVerified());
        }
        if (!data_case.GetNotes().empty() || m_backwards_compat) {
            WriteNotes(encoder, data_case.GetNotes());
        }
        WriteVectorClock(encoder, data_case.GetVectorClock());

        if (data_case.IsPartial()) {
            WritePartialSave(encoder, data_case.GetPartialSaveMode(), data_case.GetPartialSaveCaseItemReference());
        }

        if (m_backwards_compat)
            WriteDataText(encoder, data_case);
        else
            WriteData(encoder, data_case);

        encoder.end_object();
    }

    template <typename EncoderType>
    static void WriteValue(EncoderType& encoder, const CString& value)
    {
        encoder.string_value(UTF8Convert::WideToUTF8(value));
    }

    template <typename EncoderType>
    static void WriteKey(EncoderType& encoder, const CString& key)
    {
        encoder.key(UTF8Convert::WideToUTF8(key));
    }

    template <typename EncoderType>
    static void WriteField(EncoderType& encoder, const char* key, const CString& value)
    {
        encoder.key(key);
        WriteValue(encoder, value);
    }

    template <typename EncoderType>
    static void WriteField(EncoderType& encoder, const char* key, bool value)
    {
        encoder.key(key);
        encoder.bool_value(value);
    }

    template <typename EncoderType>
    static void WriteField(EncoderType& encoder, const char* key, int value)
    {
        WriteField(encoder, key, (int64_t)value);
    }

    template <typename EncoderType>
    static void WriteField(EncoderType& encoder, const char* key, uint32_t value)
    {
        WriteField(encoder, key, (uint64_t)value);
    }

#ifdef WASM
    template <typename EncoderType>
    static void WriteField(EncoderType& encoder, const char* key, unsigned long value)
    {
        WriteField(encoder, key, (unsigned long)value);
    }
#endif

    template <typename EncoderType>
    static void WriteField(EncoderType& encoder, const char* key, int64_t value)
    {
        encoder.key(key);
        encoder.int64_value(value);
    }

    template <typename EncoderType>
    static void WriteField(EncoderType& encoder, const char* key, uint64_t value)
    {
        encoder.key(key);
        encoder.uint64_value(value);
    }

    template <typename EncoderType>
    static void WriteNotes(EncoderType& encoder, const std::vector<Note>& notes)
    {
        encoder.key("notes");
        encoder.begin_array();
        for (auto& note : notes) {
            encoder.begin_object();
            WriteField(encoder, "content", note.GetContent());
            encoder.key("field");
            WriteNamedReference(encoder, note.GetNamedReference());
            WriteField(encoder, "operatorId", note.GetOperatorId());
            WriteField(encoder, "modifiedTime", PortableFunctions::TimeToRFC3339String(note.GetModifiedDateTime()));
            encoder.end_object();
        }
        encoder.end_array();
    }

    template <typename EncoderType>
    static void WriteNamedReference(EncoderType& encoder, const NamedReference& named_reference)
    {
        encoder.begin_object();
        WriteField(encoder, "name", named_reference.GetName());
        WriteField(encoder, "levelKey", named_reference.GetLevelKey());
        // named references don't always have occurrences but at the moment they must be serialized for CSWeb
        const auto& one_based_occurrences = named_reference.GetOneBasedOccurrences();
        ASSERT(one_based_occurrences.empty() || one_based_occurrences.size() == 3);
        WriteField(encoder, "recordOccurrence", one_based_occurrences.empty() ? 0 : one_based_occurrences[0]);
        WriteField(encoder, "itemOccurrence", one_based_occurrences.empty() ? 0 : one_based_occurrences[1]);
        WriteField(encoder, "subitemOccurrence", one_based_occurrences.empty() ? 0 : one_based_occurrences[2]);
        encoder.end_object();
    }

    template <typename EncoderType>
    static void WriteVectorClock(EncoderType& encoder, const VectorClock& clock)
    {
        encoder.key("clock");
        encoder.begin_array();
        std::vector<DeviceId> devices = clock.getAllDevices();
        for (std::vector<DeviceId>::const_iterator i = devices.begin(); i != devices.end(); ++i) {
            encoder.begin_object();
            WriteField(encoder, "deviceId", *i);
            WriteField(encoder, "revision", clock.getVersion(*i));
            encoder.end_object();
        }
        encoder.end_array();
    }

    template <typename EncoderType>
    void WritePartialSave(EncoderType& encoder, PartialSaveMode mode, const CaseItemReference* item)
    {
        encoder.key("partialSave");
        encoder.begin_object();
        encoder.key("mode");
        switch (mode) {
        case PartialSaveMode::Add:
            encoder.string_value("add");
            break;
        case PartialSaveMode::Modify:
            encoder.string_value("modify");
            break;
        case PartialSaveMode::Verify:
            encoder.string_value("verify");
            break;
        default:
            assert(false);
            // This should never happen, but if it does just pick
            // add mode to keep the JSON valid otherwise it won't parse
            encoder.string_value("add");
        }
        
        if (item != nullptr || m_backwards_compat) {
            encoder.key("field");

            if (item != nullptr) 
                WriteNamedReference(encoder, *item);
            else // CSWeb requires a field, so give it a dummy field
                WriteNamedReference(encoder, NamedReference(CString(), CString()));
        }

        encoder.end_object();
    }

    template <typename EncoderType>
    void WriteData(EncoderType& encoder, const Case& data_case)
    {
        encoder.key("level-1");
        if (m_stringify_data) {
            std::ostringstream data_string;
            jsoncons::compact_json_stream_encoder data_encoder(data_string, GetJsonOptions());
            WriteLevel(data_encoder, data_case.GetRootCaseLevel());
            data_encoder.flush();
            auto s = data_string.str();
            encoder.string_value(s);
        }
        else {
            WriteLevel(encoder, data_case.GetRootCaseLevel());
        }
    }

    template <typename EncoderType>
    void WriteLevel(EncoderType& encoder, const CaseLevel& level)
    {
        encoder.begin_object();
        encoder.key("id");
        WriteRecord(encoder, level.GetIdCaseRecord());

        for (size_t record_number = 0; record_number < level.GetNumberCaseRecords(); ++record_number) {
            const CaseRecord& record = level.GetCaseRecord(record_number);
            if (record.GetNumberOccurrences() > 0) {
                WriteKey(encoder, record.GetCaseRecordMetadata().GetDictionaryRecord().GetName());
                WriteRecord(encoder, level.GetCaseRecord(record_number));
            }
        }

        if (level.GetNumberChildCaseLevels() > 0) {
            encoder.key(std::string("level-") + std::to_string(level.GetCaseLevelMetadata().GetDictLevel().GetLevelNumber() + 2));
            encoder.begin_array();
            for (size_t level_index = 0; level_index < level.GetNumberChildCaseLevels(); ++level_index)
            {
                const auto& child_level = level.GetChildCaseLevel(level_index);
                WriteLevel(encoder, child_level);
            }
            encoder.end_array();
        }
        encoder.end_object();
    }

    template <typename EncoderType>
    void WriteRecord(EncoderType& encoder, const CaseRecord& record)
    {
        if (record.GetCaseRecordMetadata().GetDictionaryRecord().GetMaxRecs() > 1)
            encoder.begin_array();
        for (CaseItemIndex index = record.GetCaseItemIndex(); index.GetRecordOccurrence() < record.GetNumberOccurrences(); index.IncrementRecordOccurrence())
        {
            encoder.begin_object();
            for (auto& item : record.GetCaseItems()) {
                WriteItem(encoder, *item, index);
            }
            encoder.end_object();
        }
        if (record.GetCaseRecordMetadata().GetDictionaryRecord().GetMaxRecs() > 1)
            encoder.end_array();
    }

    template <typename EncoderType>
    void WriteItem(EncoderType& encoder, const CaseItem& item, CaseItemIndex index)
    {
        if (item.GetDictionaryItem().GetItemSubitemOccurs() > 1)
            WriteMultipleItem(encoder, item, index);
        else
            WriteSingleItem(encoder, item, index);
    }

    template <typename EncoderType>
    void WriteSingleItem(EncoderType& encoder, const CaseItem& item, CaseItemIndex index)
    {
        if (!item.IsBlank(index)) {
            WriteKey(encoder, item.GetDictionaryItem().GetName());
            WriteItemValue(encoder, item, index);
        }
    }

    template <typename EncoderType>
    void WriteMultipleItem(EncoderType& encoder, const CaseItem& item, CaseItemIndex index)
    {
        int max_not_blank = -1;
        for (size_t i = 0; i < item.GetTotalNumberItemSubitemOccurrences(); ++i) {
            index.SetItemSubitemOccurrence(item, i);
            if (!item.IsBlank(index)) {
                max_not_blank = i;
            }
        }

        if (max_not_blank == -1)
            return;

        WriteKey(encoder, item.GetDictionaryItem().GetName());
        encoder.begin_array();

        for (int i = 0; i <= max_not_blank; ++i) {
            index.SetItemSubitemOccurrence(item, i);
            WriteItemValue(encoder, item, index);
        }
        encoder.end_array();
    }

    template <typename EncoderType>
    void WriteItemValue(EncoderType& encoder, const CaseItem& item, const CaseItemIndex& index)
    {
        if (item.IsTypeString()) {
            const auto& string_item = static_cast<const StringCaseItem&>(item);
            CString string_value = string_item.GetValue(index);
            SO::MakeTrimRightSpace(string_value);
            WriteValue(encoder, string_value);
        }
        else if (item.IsTypeNumeric()) {
            if (item.IsBlank(index)) {
                // write notappl as empty string
                WriteValue(encoder, "");
            }
            else {
                const auto& numeric_item = static_cast<const NumericCaseItem&>(item);
                double value = numeric_item.GetValueForOutput(index);
                if (IsSpecial(value)) {
                    WriteValue(encoder, SpecialValues::ValueToString(value));
                }
                else {
                    if (item.GetDictionaryItem().GetDecimal() > 0)
                        encoder.double_value(value);
                    else
                        encoder.int64_value((int64_t)value);
                }
            }
        }
        else if (item.IsTypeBinary())
        {
            //  write binary case item
              /* {
                    "MYBINARY_ITEM_NAME":{
                        "length": 123,
                        "caseid": "b9ce88e7-8df0-45ec-a523-4ab9f7e162d1",
                        "signature" : "1136bedad7ce245c2330793d3c24f2ee",
                        "metadata":{
                            "mime":"image/jpg"
                        },
                    }
                }*/

            encoder.begin_object();
            const BinaryCaseItem& binary_case_item = assert_cast<const BinaryCaseItem&>(item);
            WriteBinaryCaseItem(encoder, binary_case_item, index);
            encoder.end_object();
        }
    }

    template <typename EncoderType>
    void WriteDataText(EncoderType& encoder, const Case& data_case)
    {
        encoder.key("data");
        if (!m_text_case_converter) {
            m_text_case_converter = std::make_unique<TextToCaseConverter>(data_case.GetCaseMetadata());
        }

        size_t len;
        const char* case_contents = m_text_case_converter->CaseToTextUtf8(data_case, &len);
        WriteTextDataLines(encoder, case_contents, len);
    }

    template <typename EncoderType>
    void WriteTextDataLines(EncoderType& encoder, const char* case_text, size_t case_text_len)
    {
        // Data is written out as an array of lines since the newlines in
        // the blob would mess up a json parser.
        encoder.begin_array();
        std::string_view case_string(case_text, case_text_len);
        while (!case_string.empty()) {
            size_t pos = case_string.find("\r\n");
            if (pos != std::string_view::npos) {
                encoder.string_value(case_string.substr(0, pos));
                case_string.remove_prefix(pos + 2);
            }
            else {
                encoder.string_value(case_string);
                break;
            }
        }
        encoder.end_array();
    }
};
