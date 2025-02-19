#include "stdafx.h"
#include "CaseJsonSerializer.h"
#include "BinaryCaseItem.h"
#include "CaseItemJsonWriter.h"
#include "CaseItemReference.h"
#include <zToolsO/Encoders.h>
#include <zUtilO/BinaryContentReader.h>


DEFINE_ENUM_JSON_SERIALIZER_CLASS(PartialSaveMode,
    { PartialSaveMode::None,   "none" },
    { PartialSaveMode::Add,    "add" },
    { PartialSaveMode::Modify, "modify" },
    { PartialSaveMode::Verify, "verify" })


// --------------------------------------------------------------------------
// Case -> JSON framework
// --------------------------------------------------------------------------

namespace
{
    class CaseJsonWriter
    {
    public:
        CaseJsonWriter();
        CaseJsonWriter(JsonWriter& json_writer);

        void WriteCase(JsonWriter& json_writer, const Case& data_case) const;
        void WriteCaseRecordOccurrences(JsonWriter& json_writer, const CaseRecord& case_record) const;

    private:
        void WriteNamedReference(JsonWriter& json_writer, const NamedReference& named_reference, bool write_to_new_json_object = true) const;

        void WriteNote(JsonWriter& json_writer, const Note& note) const;

        void WriteCaseItem(JsonWriter& json_writer, const CaseItem& case_item, const CaseItemIndex& index) const;
        inline void WriteBinaryCaseItem(JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index) const;

        void WriteCaseItemsOnCaseRecord(JsonWriter& json_writer, const CaseRecord& case_record, CaseItemIndex& index) const;

        void WriteCaseLevel(JsonWriter& json_writer, const CaseLevel& case_level) const;

    private:
        bool m_verbose;
        bool m_writeBlankValues;
        const CaseItemPrinter* m_caseItemPrinterForLabels;
        const CaseJsonWriterSerializerHelper::BinaryDataWriter* m_binaryDataWriter;
        const FieldStatusRetriever* m_fieldStatusRetriever;
        bool m_writeCaseNote;
        bool m_writeVectorClock;
    };
}


void Case::WriteJson(JsonWriter& json_writer) const
{
    CaseJsonWriter case_json_writer(json_writer);

    case_json_writer.WriteCase(json_writer, *this);
}


void CaseRecord::WriteJson(JsonWriter& json_writer) const
{
    CaseJsonWriter case_json_writer(json_writer);

    json_writer.BeginArray();
    case_json_writer.WriteCaseRecordOccurrences(json_writer, *this);
    json_writer.EndArray();
}



// --------------------------------------------------------------------------
// CaseJsonWriter
// --------------------------------------------------------------------------

CaseJsonWriter::CaseJsonWriter()
    :   m_verbose(false),
        m_writeBlankValues(false),
        m_caseItemPrinterForLabels(nullptr),
        m_binaryDataWriter(nullptr),
        m_fieldStatusRetriever(nullptr),
        m_writeCaseNote(false),
        m_writeVectorClock(false)
{
}


CaseJsonWriter::CaseJsonWriter(JsonWriter& json_writer)
    :   CaseJsonWriter()
{
    m_verbose = json_writer.Verbose();

    const CaseJsonWriterSerializerHelper* case_json_writer_serializer_helper = json_writer.GetSerializerHelper().Get<CaseJsonWriterSerializerHelper>();

    if( case_json_writer_serializer_helper != nullptr )
    {
        if( case_json_writer_serializer_helper->GetVerbose() )
            m_verbose = true;

        m_writeBlankValues = case_json_writer_serializer_helper->GetWriteBlankValues();

        if( case_json_writer_serializer_helper->GetWriteLabels() )
        {
            m_caseItemPrinterForLabels = case_json_writer_serializer_helper->GetCaseItemPrinter();
            ASSERT(m_caseItemPrinterForLabels != nullptr);
        }

        m_binaryDataWriter = case_json_writer_serializer_helper->GetBinaryDataWriter();
        m_fieldStatusRetriever = case_json_writer_serializer_helper->GetFieldStatusRetriever();
        m_writeCaseNote = case_json_writer_serializer_helper->GetWriteCaseNote();
        m_writeVectorClock = case_json_writer_serializer_helper->GetWriteVectorClock();
    }

    if( m_verbose )
        m_writeBlankValues = true;
}


void CaseJsonWriter::WriteNamedReference(JsonWriter& json_writer, const NamedReference& named_reference, const bool write_to_new_json_object/* = true*/) const
{
    if( write_to_new_json_object )
        json_writer.BeginObject();

    json_writer.Write(JK::name, named_reference.GetName());

    if( m_verbose || !named_reference.GetLevelKey().empty() )
        json_writer.Write(JK::levelKey, named_reference.GetLevelKey());

    if( named_reference.HasOccurrences() || ( m_verbose && dynamic_cast<const CaseItemReference*>(&named_reference) != nullptr ) )
    {
        const CaseItemReference& case_item_reference = assert_cast<const CaseItemReference&>(named_reference);

        json_writer.Key(JK::occurrences);
        case_item_reference.GetItemIndexHelper().WriteJson(json_writer, case_item_reference,
                                                           m_verbose ? ItemIndexHelper::WriteJsonMode::FullOccurrences : ItemIndexHelper::WriteJsonMode::MinimalOccurrences);
    }

    if( write_to_new_json_object )
        json_writer.EndObject();
}


void CaseJsonWriter::WriteNote(JsonWriter& json_writer, const Note& note) const
{
    json_writer.BeginObject();

    json_writer.Write(JK::text, note.GetContent());

    if( note.GetModifiedDateTime() != 0 )
        json_writer.WriteDate(JK::modifiedTime, note.GetModifiedDateTime());

    if( m_verbose || !note.GetOperatorId().empty() )
        json_writer.Write(JK::operatorId, note.GetOperatorId());

    WriteNamedReference(json_writer, note.GetNamedReference(), false);

    json_writer.EndObject();
}


void CaseJsonWriter::WriteCaseItem(JsonWriter& json_writer, const CaseItem& case_item, const CaseItemIndex& index) const
{
    json_writer.BeginObject();

    if( IsBinary(case_item.GetDataType()) )
    {
        WriteBinaryCaseItem(json_writer, assert_cast<const BinaryCaseItem&>(case_item), index);
    }

    // numeric/string data will be written with the code and label
    else
    {
        if( !case_item.IsBlank(index) )
        {
            json_writer.Key(JK::code);

            switch( case_item.GetDataType() )
            {
                case DataType::Numeric:
                    CaseItemJsonWriter::WriteCaseItemCode(json_writer, assert_cast<const NumericCaseItem&>(case_item), index);
                    break;

                case DataType::String:
                    CaseItemJsonWriter::WriteCaseItemCode(json_writer, assert_cast<const StringCaseItem&>(case_item), index);
                    break;

                default:
                    ASSERT(false);
                    json_writer.WriteNull();
                    break;
            }

            // potentially write the label
            if( m_caseItemPrinterForLabels != nullptr )
                json_writer.Write(JK::label, m_caseItemPrinterForLabels->GetText(case_item, index));
        }
    }

    // potentially write the field status
    if( m_fieldStatusRetriever != nullptr )
        json_writer.Write(JK::entryStatus, (*m_fieldStatusRetriever)(case_item, index));

    json_writer.EndObject();
}


void CaseJsonWriter::WriteBinaryCaseItem(JsonWriter& json_writer, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index) const
{
    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);

    if( !binary_data_accessor.IsDefined() )
        return;

    bool exception_is_from_read_error = true;

    try
    {
        // write the binary content using a custom serializer...
        if( m_binaryDataWriter != nullptr )
        {
            json_writer.Write(JK::metadata, binary_data_accessor.GetBinaryDataMetadata());

            exception_is_from_read_error = false;
            (*m_binaryDataWriter)(json_writer, binary_case_item, index);
        }

        // ...or as a data URL
        else
        {
            const BinaryData& binary_data = binary_data_accessor.GetBinaryData();

            json_writer.Write(JK::metadata, binary_data.GetMetadata());

            json_writer.BeginObject(JK::content)
                       .Write(JK::url, Encoders::ToDataUrl(binary_data.GetContent(), ValueOrDefault(binary_data.GetMetadata().GetMimeType())))
                       .EndObject();
        }
    }

    catch( const CSProException& exception )
    {
        const Case& data_case = index.GetCase();

        if( data_case.GetCaseConstructionReporter() != nullptr )
            data_case.GetCaseConstructionReporter()->BinaryDataIOError(data_case, exception_is_from_read_error, exception.what());
    }
}


void CaseJsonWriter::WriteCaseItemsOnCaseRecord(JsonWriter& json_writer, const CaseRecord& case_record, CaseItemIndex& index) const
{
    for( const CaseItem* const case_item : case_record.GetCaseItems() )
    {
        const CDictItem& dict_item = case_item->GetDictItem();

        // write multiply-occurring items as arrays
        const bool write_as_array = ( case_item->GetTotalNumberItemSubitemOccurrences() > 1 );

        // ensure that there are either item or subitems occurrences, not both
        if( write_as_array )
        {
            ASSERT(( ( dict_item.GetOccurs() > 1 ) != ( dict_item.GetParentItem() != nullptr && dict_item.GetParentItem()->GetOccurs() > 1 ) ));
        }

        // determine how many occurrences are defined
        for( index.SetItemSubitemOccurrence(*case_item, case_item->GetTotalNumberItemSubitemOccurrences() - 1);
             index.GetItemSubitemOccurrence(*case_item) < case_item->GetTotalNumberItemSubitemOccurrences();
             index.DecrementItemSubitemOccurrence(*case_item) )
        {
            if( !case_item->IsBlank(index) )
                break;
        }

        size_t defined_occurrences = index.GetItemSubitemOccurrence(*case_item) + 1;

        if( defined_occurrences == 0 )
        {
            if( !m_writeBlankValues )
                continue;

            // when writing blank values, write at least one occurrence for singly-occurring items
            if( !write_as_array )
                defined_occurrences = 1;
        }

        if( write_as_array )
        {
            json_writer.BeginArray(dict_item.GetName());
        }

        else
        {
            json_writer.Key(dict_item.GetName());
        }

        for( index.SetItemSubitemOccurrence(*case_item, 0);
             index.GetItemSubitemOccurrence(*case_item) < defined_occurrences;
             index.IncrementItemSubitemOccurrence(*case_item) )
        {
            WriteCaseItem(json_writer, *case_item, index);
        }

        if( write_as_array )
            json_writer.EndArray();
    }
}


void CaseJsonWriter::WriteCaseRecordOccurrences(JsonWriter& json_writer, const CaseRecord& case_record) const
{
    for( CaseItemIndex record_index = case_record.GetCaseItemIndex();
         record_index.GetRecordOccurrence() < case_record.GetNumberOccurrences();
         record_index.IncrementRecordOccurrence() )
    {
        json_writer.BeginObject();
        WriteCaseItemsOnCaseRecord(json_writer, case_record, record_index);
        json_writer.EndObject();
    }
}


void CaseJsonWriter::WriteCaseLevel(JsonWriter& json_writer, const CaseLevel& case_level) const
{
    json_writer.BeginObject();

    // write the IDs
    const CaseRecord& id_case_record = case_level.GetIdCaseRecord();
    CaseItemIndex id_index = id_case_record.GetCaseItemIndex();
    WriteCaseItemsOnCaseRecord(json_writer, id_case_record, id_index);


    // write the records
    for( size_t record_number = 0; record_number < case_level.GetNumberCaseRecords(); ++record_number )
    {
        const CaseRecord& case_record = case_level.GetCaseRecord(record_number);

        // don't write undefined records by default
        if( !m_writeBlankValues && case_record.GetNumberOccurrences() == 0 )
            continue;

        json_writer.BeginArray(case_record.GetCaseRecordMetadata().GetDictRecord().GetName());
        WriteCaseRecordOccurrences(json_writer, case_record);
        json_writer.EndArray();
    }


    // write any child levels (not writing nonexistent levels by default)
    if( case_level.GetNumberChildCaseLevels() > 0 || m_writeBlankValues )
    {
        const CaseLevelMetadata* const child_case_level_metadata = case_level.GetCaseLevelMetadata().GetChildCaseLevelMetadata();
        ASSERT(child_case_level_metadata != nullptr || case_level.GetNumberChildCaseLevels() == 0);

        if( child_case_level_metadata != nullptr )
        {
            json_writer.BeginArray(child_case_level_metadata->GetDictLevel().GetName());

            for( size_t level_index = 0; level_index < case_level.GetNumberChildCaseLevels(); ++level_index )
                WriteCaseLevel(json_writer, case_level.GetChildCaseLevel(level_index));

            json_writer.EndArray();
        }
    }

    json_writer.EndObject();
}


void CaseJsonWriter::WriteCase(JsonWriter& json_writer, const Case& data_case) const
{
    json_writer.BeginObject();

    json_writer.Write(JK::key, data_case.GetKey())
               .Write(JK::uuid, data_case.GetUuid());

    if( m_verbose || !data_case.GetCaseLabel().empty() )
        json_writer.Write(JK::label, data_case.GetCaseLabel());

    if( m_verbose || data_case.GetDeleted() )
        json_writer.Write(JK::deleted, data_case.GetDeleted());

    if( m_verbose || data_case.GetVerified() )
        json_writer.Write(JK::verified, data_case.GetVerified());

    // partial save status
    if( data_case.IsPartial() )
    {
        json_writer.BeginObject(JK::partialSave)
                   .Write(JK::mode, data_case.GetPartialSaveMode());

        if( data_case.GetPartialSaveCaseItemReference() != nullptr )
        {
            WriteNamedReference(json_writer, *data_case.GetPartialSaveCaseItemReference(), false);
        }

        else
        {
            ASSERT(false);
        }

        json_writer.EndObject();
    }

    else if( m_verbose )
    {
        json_writer.WriteNull(JK::partialSave);
    }

    // case note (written only when syncing)
    if( m_writeCaseNote )
        json_writer.WriteIfNotBlank(JK::caseNote, data_case.GetCaseNote());

    // notes
    if( m_verbose || !data_case.GetNotes().empty() )
    {
        json_writer.BeginArray(JK::notes);

        for( const Note& note : data_case.GetNotes() )
            WriteNote(json_writer, note);

        json_writer.EndArray();
    }

    // levels
    json_writer.Key(data_case.GetRootCaseLevel().GetCaseLevelMetadata().GetDictLevel().GetName());
    WriteCaseLevel(json_writer, data_case.GetRootCaseLevel());

    // vector clock
    if( m_verbose || m_writeVectorClock )
        json_writer.Write(JK::clock, data_case.GetVectorClock());

    json_writer.EndObject();
}



// --------------------------------------------------------------------------
// JSON -> Case
// --------------------------------------------------------------------------

namespace
{
    class CaseJsonParser
    {
    public:
        CaseJsonParser(CaseJsonParserHelper& case_json_parser_helper, Case& data_case);

        void ParseCase(const JsonNode& json_node) const;

    private:
        template<typename T = NamedReference>
        std::unique_ptr<T> ParseNamedReference(const JsonNode& named_reference_node) const;

        Note ParseNote(const JsonNode& note_node) const;

        void ParseCaseItem(const JsonNode& case_item_node, const CaseItem& case_item, CaseItemIndex& index) const;

        void ParseCaseItemsOnCaseRecord(const JsonNode& case_record_node, CaseRecord& case_record, CaseItemIndex& index) const;

        void ParseCaseRecords(const JsonNode& case_records_node, CaseRecord& case_record) const;

        void ParseCaseLevel(const JsonNode& case_level_node, CaseLevel& case_level) const;

    private:
        CaseJsonParserHelper& m_caseJsonParserHelper;
        Case& m_case;
        bool m_usingCaseConstructionReporter;
    };
}


void Case::ParseJson(const JsonNode& json_node)
{
    CaseJsonParserHelper case_json_parser_helper(nullptr);
    case_json_parser_helper.ParseJson(*this, json_node);
}


std::unique_ptr<BinaryContentReader> CaseJsonParserHelper::CreateBinaryContentReader(std::optional<uint64_t> /*size*/)
{
    return nullptr;
}


void CaseJsonParserHelper::ParseJson(Case& data_case, const JsonNode& json_node)
{
    try
    {
        CaseJsonParser case_json_parser(*this, data_case);
        case_json_parser.ParseCase(json_node);
    }

    catch( const CSProException& )
    {
        // exceptions should not be thrown by the parser
        ASSERT(false);
        data_case.Reset();
    }
}


CaseJsonParser::CaseJsonParser(CaseJsonParserHelper& case_json_parser_helper, Case& data_case)
    :   m_caseJsonParserHelper(case_json_parser_helper),
        m_case(data_case),
        m_usingCaseConstructionReporter(m_case.GetCaseConstructionReporter() != nullptr)
{
}


template<typename T/* = NamedReference*/>
std::unique_ptr<T> CaseJsonParser::ParseNamedReference(const JsonNode& named_reference_node) const
{
    const std::string name = named_reference_node.GetOrConstruct<std::string>(JK::name);

    if( SO::IsWhitespace(name) )
        return nullptr;

    std::string level_key = named_reference_node.GetOrConstruct<std::string>(JK::levelKey);

    // see if this is an item
    const CaseItem* const case_item = ( m_caseJsonParserHelper.GetCaseAccess() != nullptr ) ? m_caseJsonParserHelper.GetCaseAccess()->LookupCaseItem(name) :
                                                                                              m_case.GetCaseMetadata().FindCaseItem(name);

    if( case_item != nullptr )
    {
        size_t record_occurrence = 0;
        size_t item_occurrence = 0;
        size_t subitem_occurrence = 0;

        if( named_reference_node.Contains(JK::occurrences) )
        {
            const JsonNode occurrence_node = named_reference_node.Get(JK::occurrences);

            // occurrences are written as one-based, though 0 will be written for item/subitem occurrences that do not apply
            auto get_occurrence = [&](const char* const key) -> size_t
            {
                const std::optional<size_t> occurrence = occurrence_node.GetOptional<size_t>(key);
                return ( occurrence.has_value() && *occurrence >= 1 ) ? ( *occurrence - 1 ) : 0;
            };

            record_occurrence = get_occurrence(JK::record);
            item_occurrence = get_occurrence(JK::item);
            subitem_occurrence = get_occurrence(JK::subitem);
        }

        return std::make_unique<CaseItemReference>(*case_item, std::move(level_key), record_occurrence, item_occurrence, subitem_occurrence);
    }

    if constexpr(std::is_same_v<T, CaseItemReference>)
    {
        return nullptr;
    }

    else
    {
        return std::make_unique<NamedReference>(std::move(name), std::move(level_key));
    }
}


Note CaseJsonParser::ParseNote(const JsonNode& note_node) const
{
    return Note(note_node.GetOrConstruct<std::string>(JK::text),
                ParseNamedReference(note_node),
                note_node.GetOrConstruct<std::string>(JK::operatorId),
                note_node.Contains(JK::modifiedTime) ? note_node.GetDate(JK::modifiedTime) : 0);
}


void CaseJsonParser::ParseCaseItem(const JsonNode& case_item_node, const CaseItem& case_item, CaseItemIndex& index) const
{
    // the case item should be blank (unless it is a subitem that already has a value from the parent being set)
    ASSERT(case_item.IsBlank(index) || case_item.GetDictItem().GetParentItem() != nullptr);

    if( IsNumeric(case_item.GetDataType()) )
    {
        CaseJsonParserHelper::ParseNumericCaseItem(assert_cast<const NumericCaseItem&>(case_item), index, case_item_node);
    }

    else if( IsString(case_item.GetDataType()) )
    {
        CaseJsonParserHelper::ParseStringCaseItem(assert_cast<const StringCaseItem&>(case_item), index, case_item_node);
    }

    else if( IsBinary(case_item.GetDataType()) )
    {
        m_caseJsonParserHelper.ParseBinaryCaseItem(assert_cast<const BinaryCaseItem&>(case_item), index, case_item_node);
    }

    else
    {
        ASSERT(false);
    }
}


void CaseJsonParserHelper::ParseNumericCaseItem(const NumericCaseItem& numeric_case_item, CaseItemIndex& index, const JsonNode& case_item_node)
{
    const JsonNode& code_node = case_item_node.GetOrEmpty(JK::code);

    std::optional<double> numeric_value = code_node.GetOptional<double>();

    if( numeric_value.has_value() )
    {
        numeric_case_item.SetValueFromInput(index, *numeric_value);
    }

    else if( !code_node.IsEmpty() && !code_node.IsNull() )
    {
        numeric_value = SpecialValues::StringIsSpecial<std::optional<double>>(case_item_node.GetOrConstruct<std::string>(JK::code));
        numeric_case_item.SetValue(index, numeric_value.value_or(DEFAULT));
    }
}


void CaseJsonParserHelper::ParseStringCaseItem(const StringCaseItem& string_case_item, CaseItemIndex& index, const JsonNode& case_item_node)
{
    string_case_item.SetValue(index, case_item_node.GetOrConstruct<std::string>(JK::code));
}


void CaseJsonParserHelper::ParseBinaryCaseItem(const BinaryCaseItem& binary_case_item, CaseItemIndex& index, const JsonNode& case_item_node)
{
    BinaryDataMetadata binary_data_metadata = case_item_node.GetOrEmpty(JK::metadata).Get<BinaryDataMetadata>();

    // read the binary content as a data URL...
    if( case_item_node.Contains(JK::content) )
    {
        const JsonNode content_node = case_item_node.Get(JK::content);

        if( content_node.Contains(JK::url) )
        {
            auto [content, mediatype] = Encoders::FromDataUrl(content_node.Get<std::string_view>(JK::url));

            if( content != nullptr )
            {
                binary_case_item.SetValue(index, BinaryData(std::move(content), std::move(binary_data_metadata)));
            }

            else
            {
                const Case& data_case = index.GetCase();

                if( data_case.GetCaseConstructionReporter() != nullptr )
                {
                    const std::string error = FormatText("The binary content for '%s' is not a valid data URL.",
                                                         binary_case_item.GetDictItem().GetName().c_str());
                    data_case.GetCaseConstructionReporter()->BinaryDataIOError(data_case, true, error);
                }
            }
        }
    }

    // ... or using a repository's binary content reader
    else
    {
        std::string signature = case_item_node.GetOrConstruct<std::string>(JK::signature);
        std::unique_ptr<BinaryContentReader> binary_content_reader;

        if( BinaryDataAccessor::IsValidSignature(signature) )
        {
            std::optional<uint64_t> size = case_item_node.GetOptional<uint64_t>(JK::size);

            // the size was serialized as "length" in V2 JSON
            if( !size.has_value() )
                size = case_item_node.GetOptional<uint64_t>(JK::length);

            binary_content_reader = CreateBinaryContentReader(std::move(size));
        }

        if( binary_content_reader != nullptr )
        {
            binary_case_item.SetValue(index, std::move(binary_data_metadata), std::move(signature), std::move(binary_content_reader));
        }

        else
        {
            ASSERT(false);
            binary_case_item.Clear(index);
        }
    }
}


void CaseJsonParser::ParseCaseItemsOnCaseRecord(const JsonNode& case_record_node, CaseRecord& case_record, CaseItemIndex& index) const
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


void CaseJsonParser::ParseCaseRecords(const JsonNode& case_records_node, CaseRecord& case_record) const
{
    ASSERT(case_record.GetNumberOccurrences() == 0);

    if( case_records_node.IsEmpty() )
        return;

    const size_t max_records = case_record.GetCaseRecordMetadata().GetDictRecord().GetMaxRecs();
    CaseItemIndex index = case_record.GetCaseItemIndex();
    size_t records_added = 0;

    for( const JsonNode& case_record_node_array_element : case_records_node.GetArrayOrEmpty() )
    {
        // issue a warning (and break out) when there are too many records
        if( records_added == max_records )
        {
            if( m_usingCaseConstructionReporter )
            {
                m_case.GetCaseConstructionReporter()->TooManyRecordOccurrences(m_case,
                                                                               case_record.GetCaseRecordMetadata().GetDictRecord().GetName(),
                                                                               max_records);
            }

            break;
        }

        // add the record occurrence
        case_record.SetNumberOccurrences(++records_added);

        if( records_added != 1 )
            index.IncrementRecordOccurrence();

        if( m_usingCaseConstructionReporter )
            m_case.GetCaseConstructionReporter()->IncrementRecordCount();

        // parse the record's items
        ParseCaseItemsOnCaseRecord(case_record_node_array_element, case_record, index);
    }
}


void CaseJsonParser::ParseCaseLevel(const JsonNode& case_level_node, CaseLevel& case_level) const
{
    ASSERT(case_level.GetNumberChildCaseLevels() == 0);

    if( case_level_node.IsEmpty() )
        return;

    if( m_usingCaseConstructionReporter )
        m_case.GetCaseConstructionReporter()->IncrementCaseLevelCount(case_level.GetCaseLevelMetadata().GetDictLevel().GetLevelNumber());

    // parse the IDs
    CaseRecord& id_case_record = case_level.GetIdCaseRecord();
    CaseItemIndex id_index = id_case_record.GetCaseItemIndex();
    ParseCaseItemsOnCaseRecord(case_level_node, id_case_record, id_index);

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
        for( const JsonNode& child_case_level_node_array_element : case_level_node.GetArrayOrEmpty(child_case_level_metadata->GetDictLevel().GetName()) )
        {
            CaseLevel& child_case_level = case_level.AddChildCaseLevel();
            ParseCaseLevel(child_case_level_node_array_element, child_case_level);
        }
    }
}


void CaseJsonParser::ParseCase(const JsonNode& json_node) const
{
    // reset objects that will not be directly overwritten
    m_case.SetPositionInRepository(-1);

    // set case values
    m_case.SetUuid(json_node.Contains(JK::uuid) ? json_node.Get<std::string>(JK::uuid) :
                                                  CreateUuid());

    m_case.SetCaseLabel(json_node.GetOrConstruct<std::string>(JK::label));
    m_case.SetDeleted(json_node.GetOrDefault(JK::deleted, false));
    m_case.SetVerified(json_node.GetOrDefault(JK::verified, false));

    // partial save status
    {
        PartialSaveMode partial_save_mode = PartialSaveMode::None;
        std::shared_ptr<CaseItemReference> partial_save_case_item_reference;

        if( json_node.Contains(JK::partialSave) )
        {
            const JsonNode partial_save_node = json_node.Get(JK::partialSave);
            partial_save_mode = partial_save_node.GetOrDefault(JK::mode, partial_save_mode);

            if( partial_save_mode != PartialSaveMode::None )
                partial_save_case_item_reference = ParseNamedReference<CaseItemReference>(partial_save_node);
        }

        m_case.SetPartialSaveStatus(partial_save_mode, std::move(partial_save_case_item_reference));
    }

    // notes
    {
        std::vector<Note>& notes = m_case.GetNotes();
        notes.clear();

        if( json_node.Contains(JK::notes) )
        {
            for( const JsonNode& note_node : json_node.GetArrayOrEmpty(JK::notes) )
                notes.emplace_back(ParseNote(note_node));
        }
    }

    // levels
    {
        CaseLevel& root_case_level = m_case.GetRootCaseLevel();
        root_case_level.Reset();

        const JsonNode case_level_node = json_node.GetOrEmpty(root_case_level.GetCaseLevelMetadata().GetDictLevel().GetName());
        ParseCaseLevel(case_level_node, root_case_level);
    }

    // vector clock
    {
        if( json_node.Contains(JK::clock) )
        {
            m_case.SetVectorClock(json_node.Get<VectorClock>(JK::clock));
        }

        else
        {
            m_case.GetVectorClock().clear();
        }
    }
}
