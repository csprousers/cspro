#include "stdafx.h"
#include "SyncCaseSerializer.h"
#include "SyncCaseIncrementalParser.h"
#include "SyncBinaryDataUploadManager.h"
#include "SyncCaseJsonSerializer.h"
#include "SyncWithDataBinaryContentReader.h"
#include <zJson/IncrementalJsonObjectArrayParser.h>
#include <zUtilO/Versioning.h>
#include <zNetwork/CSWebConnection.h>
#include <zNetwork/HeaderList.h>
#include <zNetwork/SyncException.h>
#include <zSyncO/SyncLogCaseConstructionReporter.h>
#include <regex>


namespace
{
    constexpr std::string_view BinaryHeader_sv = "cssync";
    constexpr int32_t BinaryVersion            = 1;

    constexpr size_t BinaryFormatBytesBeforeCaseJson = BinaryHeader_sv.length() + // the header
                                                       sizeof(int32_t) +          // the version
                                                       sizeof(int32_t);           // the length of the case JSON
}


// --------------------------------------------------------------------------
// SyncCaseSerializer
// --------------------------------------------------------------------------

SyncCaseSerializer::SyncCaseSerializer(std::shared_ptr<const CaseAccess> case_access, const Version version,
                                       std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper/* = nullptr*/)
    :   m_caseAccess(std::move(case_access))
{
    ASSERT(m_caseAccess != nullptr);

    if( version == Version::V3 )
    {
        m_syncCaseJsonSerializer = std::make_unique<SyncCaseV3JsonSerializer>(m_caseAccess, std::move(case_json_parser_helper));
        m_caseUuidKey = JK::uuid;
    }

    else
    {
        ASSERT(version == Version::V1 || version == Version::V2);
        const bool write_case_as_text_blob = ( version == Version::V1 );

        m_syncCaseJsonSerializer = std::make_unique<SyncCaseV2JsonSerializer>(m_caseAccess, write_case_as_text_blob, std::move(case_json_parser_helper));
        m_caseUuidKey = "caseid";
    }
}


SyncCaseSerializer SyncCaseSerializer::CreateFromCSWebApiVersion(std::shared_ptr<const CaseAccess> case_access, const double csweb_api_version,
                                                                 std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper/* = nullptr*/)
{
    const Version version = ( csweb_api_version >= CSWebVersion::V3 ) ? Version::V3 :
                            ( csweb_api_version >= CSWebVersion::V2 ) ? Version::V2 :
                                                                        Version::V1;

    return SyncCaseSerializer(std::move(case_access), version, std::move(case_json_parser_helper));
}


SyncCaseSerializer SyncCaseSerializer::CreateFromCSProVersion(std::shared_ptr<const CaseAccess> case_access, const double cspro_version)
{
    const Version version = ( cspro_version >= 8.1 ) ? Version::V3 :
                            ( cspro_version >= 7.4 ) ? Version::V2 :
                                                       Version::V1;

    return SyncCaseSerializer(std::move(case_access), version);
}


SyncCaseSerializer SyncCaseSerializer::CreateFromCSProVersion(std::shared_ptr<const CaseAccess> case_access, const HeaderList& request_headers)
{
    const std::string user_agent = request_headers.GetValue("User-Agent");
    const size_t slash_pos = user_agent.find('/');

    if( slash_pos != std::string::npos )
    {
        // version string will be a detailed version string of CSPro without the prefix like 7.7.3
        const std::string version_string = user_agent.substr(slash_pos + 1);
        const std::regex pattern(R"(\d+\.?\d+)");
        std::smatch matches;

        // convert the major and minor version to decimal (do not include the patch release)
        if( std::regex_search(version_string, matches, pattern) )
            return CreateFromCSProVersion(std::move(case_access), atod(matches[0].str()));
    }

    // default to V2 JSON
    return SyncCaseSerializer(std::move(case_access), Version::V2);
}


std::string SyncCaseSerializer::GetSyncableJson(const Case& data_case, const bool wrap_single_case_in_array)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(JsonFormattingOptions::Compact);

    if( wrap_single_case_in_array )
        json_writer->BeginArray();

    m_syncCaseJsonSerializer->WriteCase(*json_writer, data_case);

    if( wrap_single_case_in_array )
        json_writer->EndArray();

    return json_writer->ReleaseString();
}


std::string SyncCaseSerializer::GetSyncableJson(const cs::span<const Case* const> cases)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(JsonFormattingOptions::Compact);

    json_writer->BeginArray();

    for( const Case* const data_case : cases )
        m_syncCaseJsonSerializer->WriteCase(*json_writer, *data_case);

    json_writer->EndArray();

    return json_writer->ReleaseString();
}


std::string SyncCaseSerializer::GetSyncableCaseData(const cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* const sync_binary_data_upload_manager)
{
    std::string cases_json = GetSyncableJson(cases);

    // write the new binary data format only if cases have binary items
    if( sync_binary_data_upload_manager == nullptr || !sync_binary_data_upload_manager->IsBinaryDataPartOfChunk() )
        return cases_json;

    std::ostringstream binary_case_data_output_stream;

    // write the header and version number
    binary_case_data_output_stream << BinaryHeader_sv;
    WriteInt32(binary_case_data_output_stream, BinaryVersion);

    // write the syncable case JSON
    WriteInt32(binary_case_data_output_stream, cases_json.length());
    binary_case_data_output_stream << cases_json;

    // write the binary items
    sync_binary_data_upload_manager->ForeachBinaryCaseItemInChunk(
        [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
        {
            WriteBinaryCaseItemWithData(binary_case_data_output_stream, binary_case_item, index);
        });

    return binary_case_data_output_stream.str();
}


void SyncCaseSerializer::WriteInt32(std::ostream& output_stream, const int32_t value)
{
    output_stream.write(reinterpret_cast<const char*>(&value), sizeof(value));
}


void SyncCaseSerializer::WriteBinaryCaseItemWithData(std::ostream& output_stream, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index) const
{
    ASSERT(!binary_case_item.IsBlank(index));

    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
    const BinaryData& binary_data = binary_data_accessor.GetBinaryData();

    // write some information about the binary case item in JSON
    {
        std::string binary_case_item_details_json;
        const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(binary_case_item_details_json, JsonFormattingOptions::Compact);

        json_writer->BeginObject()
                    .Write(m_caseUuidKey, index.GetCase().GetUuid())
                    .Write(JK::signature, binary_data_accessor.GetSignature())
                    .EndObject();

        WriteInt32(output_stream, binary_case_item_details_json.length());
        output_stream << binary_case_item_details_json;
    }

    // write the actual binary data
    {
        const std::vector<std::byte>& content = binary_data.GetContent();

        SYNCLOG_INFO << "Writing binary item to sync stream (MD5: " << binary_data_accessor.GetSignature() << ", size: " << content.size() << ")";

        WriteInt32(output_stream, content.size());
        output_stream.write(reinterpret_cast<const char*>(content.data()), content.size());
    }
}


std::unique_ptr<Case> SyncCaseSerializer::ParseCaseJsonFromSyncableCaseData(const JsonNode& json_node)
{
    if( m_syncLogCaseConstructionReporter == nullptr )
        m_syncLogCaseConstructionReporter = std::make_shared<SyncLogCaseConstructionReporter>();

    std::unique_ptr<Case> data_case = m_caseAccess->CreateCase();
    data_case->SetCaseConstructionReporter(m_syncLogCaseConstructionReporter);

    m_syncCaseJsonSerializer->ParseCase(*data_case, json_node);

    return data_case;
}


std::vector<std::shared_ptr<Case>> SyncCaseSerializer::ParseCasesJsonFromSyncableCaseData(const std::string_view case_json_sv)
{
    const JsonNode json_node = Json::Parse(case_json_sv);

    std::vector<std::shared_ptr<Case>> cases;

    for( const JsonNode& case_json_node : json_node.GetArray() )
        cases.emplace_back(ParseCaseJsonFromSyncableCaseData(case_json_node));

    return cases;
}


class SyncCaseSerializer::BinaryParser
{
public:
    BinaryParser(const std::string_view data_sv)
        :   m_dataItr(data_sv.data()),
            m_bytesRemaining(data_sv.length())
    {
    }

    bool AllDataRead() const
    {
        return ( m_bytesRemaining == 0 );
    }

    template<typename T>
    T Read()
    {
        return *reinterpret_cast<const T*>(GetCurrentPositionAndAdvance(sizeof(T)));
    }

    template<>
    std::string_view Read()
    {
        const int32_t length = Read<int32_t>();
        return std::string_view(GetCurrentPositionAndAdvance(length), length);
    }

    template<>
    std::vector<std::byte> Read()
    {
        const int32_t length = Read<int32_t>();
        const std::byte* const data_start = reinterpret_cast<const std::byte*>(GetCurrentPositionAndAdvance(length));
        return std::vector<std::byte>(data_start, data_start + length);
    }

    template<typename... Args>
    [[noreturn]] static void ThrowBinaryParseError(const char* const formatter, Args const&... args)
    {
        const std::string message = FormatText(formatter, args...);
        SYNCLOG_INFO << "Error parsing binary format: " << message;
        throw SyncError(100179, message);
    }

private:
    const char* GetCurrentPositionAndAdvance(const size_t bytes_required)
    {
        if( m_bytesRemaining < bytes_required )
        {
            ThrowBinaryParseError("Tried to read %d bytes but only %d remain in the buffer.",
                                  static_cast<int>(bytes_required), static_cast<int>(m_bytesRemaining));
        }

        const char* const current_position = m_dataItr;

        m_dataItr += bytes_required;
        m_bytesRemaining -= bytes_required;

        return current_position;
    }

private:
    const char* m_dataItr;
    size_t m_bytesRemaining;
};


std::vector<std::shared_ptr<Case>> SyncCaseSerializer::ParseSyncableCaseData(const std::string_view case_data_sv)
{
    // parse the header to determine if this is written in the binary data format
    std::optional<BinaryParser> binary_parser = ParseSyncableCaseDataHeader(case_data_sv);

    // if not using the binary data format, the case data will be only the syncable case JSON
    if( !binary_parser.has_value() )
    {
        return ParseCasesJsonFromSyncableCaseData(case_data_sv);
    }

    // otherwise we must parse the binary data format
    else
    {
        // read and parse the syncable case JSON
        const std::string_view case_json_sv = binary_parser->Read<std::string_view>();
        std::vector<std::shared_ptr<Case>> cases = ParseCasesJsonFromSyncableCaseData(case_json_sv);

        // read the binary data
        ParseSyncableCaseDataBinaryData(*binary_parser, cases);

        return cases;
    }
}


std::optional<SyncCaseSerializer::BinaryParser> SyncCaseSerializer::ParseSyncableCaseDataHeader(const std::string_view case_data_sv)
{
    std::optional<BinaryParser> binary_parser;

    if( SO::StartsWith(case_data_sv, BinaryHeader_sv) )
        binary_parser.emplace(case_data_sv.substr(BinaryHeader_sv.length()));

    SYNCLOG_INFO << "Parsing case data (length " << case_data_sv.length() << ") in " << ( binary_parser.has_value() ? "binary" : "JSON" ) << " format";

    // if applicable, check the version number
    if( binary_parser.has_value() )
    {
        const int32_t version = binary_parser->Read<int32_t>();

        if( version != BinaryVersion )
        {
            BinaryParser::ThrowBinaryParseError("Binary version %d not supported by CSPro %0.1f.",
                                                version, Versioning::Number);
        }
    }

    return binary_parser;
}


void SyncCaseSerializer::ParseSyncableCaseDataBinaryData(std::vector<std::shared_ptr<Case>>& cases,
                                                         const std::function<std::optional<JsonNode>()>& metadata_reader,
                                                         const std::function<std::vector<std::byte>(const std::string& signature)>& content_reader)
{
    ASSERT(m_caseAccess->GetCaseMetadata().UsesBinaryData());

    std::map<std::string, std::tuple<BinaryContentCacher::CacheableContent, bool>> binary_content_and_uses_map;
    std::optional<JsonNode> metadata_json_node;

    // read the metadata JSON
    while( ( metadata_json_node = metadata_reader() ).has_value() )
    {
        std::string signature = metadata_json_node->GetOrConstruct<std::string>(JK::signature);

        if( !BinaryDataAccessor::IsValidSignature(signature) )
        {
            BinaryParser::ThrowBinaryParseError("Binary signature '%s' is not valid.",
                                                signature.c_str());
        }

        // verify that this case was part of thes sync
        const std::string case_uuid = metadata_json_node->GetOrConstruct<std::string>(m_caseUuidKey);
        const auto& case_lookup = std::find_if(cases.cbegin(), cases.cend(),
                                               [&](const std::shared_ptr<Case>& data_case) { return ( case_uuid == data_case->GetUuid() ); });

        if( case_lookup == cases.cend() )
        {
            BinaryParser::ThrowBinaryParseError("Binary data with signature '%s' belongs to a case with UUID '%s' that was not synced.",
                                                signature.c_str(), case_uuid.c_str());
        }

        // read the content, check its validity, and add it to our map of binary content
        std::vector<std::byte> content = content_reader(signature);
        const std::string content_md5 = Hash::Md5::Create(content);

        SYNCLOG_INFO << "Read binary item from sync stream (MD5: " << content_md5 << ", size: " << content.size() << ")";

        if( signature != content_md5 )
        {
            BinaryParser::ThrowBinaryParseError("Binary data with signature '%s' was not sent correctly as data received has signature '%s'.",
                                                signature.c_str(), content_md5.c_str());
        }

        binary_content_and_uses_map.try_emplace(std::move(signature), std::move(content), false);
    }

    // match all of the binary case content with the binary items that use it
    for( const std::shared_ptr<Case>& data_case : cases )
    {
        data_case->ForeachDefinedBinaryCaseItem(
            [&](const BinaryCaseItem& binary_case_item, CaseItemIndex& index)
            {
                BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
                ASSERT(!binary_data_accessor.IsDefinedAndContentLoaded());

                const std::string& signature = binary_data_accessor.GetSignature();
                SyncWithDataBinaryContentReader* const sync_with_data_binary_content_reader = dynamic_cast<SyncWithDataBinaryContentReader*>(binary_data_accessor.GetBinaryContentReader());

                if( sync_with_data_binary_content_reader == nullptr )
                {
                    BinaryParser::ThrowBinaryParseError("Binary item with name '%s' and signature '%s' was not matched to data sent during the sync.",
                                                        binary_case_item.GetDictItem().GetName().c_str(), signature.c_str());
                }

                auto binary_content_and_use_lookup = binary_content_and_uses_map.find(signature);

                if( binary_content_and_use_lookup != binary_content_and_uses_map.cend() )
                {
                    sync_with_data_binary_content_reader->SetContent(std::get<0>(binary_content_and_use_lookup->second));
                    std::get<1>(binary_content_and_use_lookup->second) = true;
                }

                ASSERT(sync_with_data_binary_content_reader->ContentReceivedDuringSync() == ( binary_content_and_use_lookup != binary_content_and_uses_map.cend() ));
            });
    }

#ifdef _DEBUG
    // verify that all data was matched;
    // this asserts, and no longer throws an exception, because if a user removes a binary item from a
    // dictionary, but the synced case data still has (old) binary content, it should not be an error
    for( const auto& [signature, binary_content_and_use] : binary_content_and_uses_map )
    {
        if( !std::get<1>(binary_content_and_use) )
        {
            // BinaryParser::ThrowBinaryParseError("Binary data with signature '%s' was not matched to any binary items.",
            //                                     signature.c_str());
            ASSERT(false);
        }
    }
#endif
}


void SyncCaseSerializer::ParseSyncableCaseDataBinaryData(BinaryParser& binary_parser, std::vector<std::shared_ptr<Case>>& cases)
{
    ParseSyncableCaseDataBinaryData(cases,
        [&]() -> std::optional<JsonNode>
        {
            if( !binary_parser.AllDataRead() )
            {
                const std::string_view metadata_json_sv = binary_parser.Read<std::string_view>();

                return Json::Parse(metadata_json_sv);
            }

            return std::nullopt;
        },
        [&](const std::string& /*signature*/)
        {
            return binary_parser.Read<std::vector<std::byte>>();
        });
}



// --------------------------------------------------------------------------
// SyncCaseIncrementalJsonObjectArrayParser
// --------------------------------------------------------------------------

class SyncCaseIncrementalJsonObjectArrayParser : public IncrementalJsonObjectArrayParser
{
public:
    SyncCaseIncrementalJsonObjectArrayParser(SyncCaseIncrementalParser& sync_case_incremental_parser)
        :   m_syncCaseIncrementalParser(sync_case_incremental_parser)
    {
    }

protected:
    void OnObject(const JsonNode& json_node) override
    {
        m_syncCaseIncrementalParser.ParseCaseJsonFromSyncableCaseData(json_node);
    }

    void ProcessPostParseText(const std::string_view text_sv) override
    {
        ASSERT(m_syncCaseIncrementalParser.m_postJsonCaseData.empty());
        m_syncCaseIncrementalParser.m_postJsonCaseData = text_sv;
    }

private:
    SyncCaseIncrementalParser& m_syncCaseIncrementalParser;
};



// --------------------------------------------------------------------------
// SyncCaseIncrementalParser
// --------------------------------------------------------------------------

SyncCaseIncrementalParser::SyncCaseIncrementalParser(SyncCaseSerializer sync_case_serializer)
    :   m_syncCaseSerializer(std::move(sync_case_serializer)),
        m_incrementalJsonParser(std::make_unique<SyncCaseIncrementalJsonObjectArrayParser>(*this)),
        m_parsedCaseCount(0),
        m_parsedCaseJsonLength(0),
        m_currentCasesAreParseable(true),
        m_state(State::AnalyzingHeader)
{
}


SyncCaseIncrementalParser::~SyncCaseIncrementalParser()
{
}


void SyncCaseIncrementalParser::Update(const std::string_view case_data_sv)
{
    ASSERT(m_state != State::Finalized);

    if( case_data_sv.empty() )
        return;

    if( m_state == State::AnalyzingHeader )
    {
        Update_AnalyzingHeader(case_data_sv);
    }

    else if( m_state == State::InCaseJson )
    {
        Update_InCaseJson(case_data_sv);
    }

    else if( m_state == State::PostCaseJson )
    {
        m_postJsonCaseData.append(case_data_sv);
    }
}


void SyncCaseIncrementalParser::Update_AnalyzingHeader(std::string_view case_data_sv)
{
    ASSERT(!m_caseJsonLengthWhenProcessingBinaryData.has_value());

    // only process the header once we've read enough bytes
    if( !m_preJsonCaseData.empty() )
    {
        m_preJsonCaseData.append(case_data_sv);

        if( m_preJsonCaseData.length() < BinaryFormatBytesBeforeCaseJson )
            return;

        case_data_sv = m_preJsonCaseData;
    }

    else if( case_data_sv.length() < BinaryFormatBytesBeforeCaseJson )
    {
        m_preJsonCaseData.append(case_data_sv);
        return;
    }

    // parse the header
    ASSERT(case_data_sv.length() >= BinaryFormatBytesBeforeCaseJson);

    std::optional<SyncCaseSerializer::BinaryParser> binary_parser = SyncCaseSerializer::ParseSyncableCaseDataHeader(case_data_sv);
    m_state = State::InCaseJson;

    // if using the binary data format, read the length of the case JSON and then start processing the case JSON that follows the header
    if( binary_parser.has_value() )
    {
        m_caseJsonLengthWhenProcessingBinaryData = binary_parser->Read<int32_t>();
        Update_InCaseJson(case_data_sv.substr(BinaryFormatBytesBeforeCaseJson));
    }

    // otherwise process the case data, which is the case JSON
    else
    {
        Update_InCaseJson(case_data_sv);
    }
}


void SyncCaseIncrementalParser::Update_InCaseJson(const std::string_view case_data_sv)
{
    ASSERT(m_state == State::InCaseJson && !m_incrementalJsonParser->IsComplete());

    m_parsedCaseJsonLength += case_data_sv.length();

    m_incrementalJsonParser->Update(case_data_sv);

    if( m_incrementalJsonParser->IsComplete() )
    {
        m_parsedCaseJsonLength -= m_postJsonCaseData.length();
        m_state = State::PostCaseJson;
    }
}


std::vector<std::shared_ptr<Case>> SyncCaseIncrementalParser::Finish()
{
    // if the header was never analyzed, it means that the case data is short case JSON, e.g.: []
    if( m_state == State::AnalyzingHeader )
    {
        ASSERT(m_postJsonCaseData.empty());
        m_state = State::InCaseJson;
        Update_InCaseJson(m_preJsonCaseData);
    }

    if( m_state != State::PostCaseJson )
        throw JsonParseException("The case data JSON stream is incomplete");

    ASSERT(m_incrementalJsonParser->IsComplete());

    // if processing binary data, process the binary content following the case JSON
    if( m_caseJsonLengthWhenProcessingBinaryData.has_value() && m_currentCases != nullptr )
    {
        ASSERT(m_parsedCaseJsonLength == static_cast<size_t>(*m_caseJsonLengthWhenProcessingBinaryData));

        SyncCaseSerializer::BinaryParser binary_parser(m_postJsonCaseData);
        m_syncCaseSerializer.ParseSyncableCaseDataBinaryData(binary_parser, *m_currentCases);
    }

    // when only processing case JSON, there should be no post-case data
    else if( !SO::IsWhitespace(m_postJsonCaseData) )
    {
        throw JsonParseException("The case data JSON stream has invalid data following the array of objects");
    }

    m_state = State::Finalized;

    const std::unique_ptr<std::vector<std::shared_ptr<Case>>> parseable_cases = std::exchange(m_currentCases, nullptr);

    return ( parseable_cases != nullptr ) ? std::move(*parseable_cases) :
                                            std::vector<std::shared_ptr<Case>>();
}


void SyncCaseIncrementalParser::ParseCaseJsonFromSyncableCaseData(const JsonNode& json_node)
{
    if( m_currentCases == nullptr )
        m_currentCases = std::make_unique<std::vector<std::shared_ptr<Case>>>();

    m_currentCases->emplace_back(m_syncCaseSerializer.ParseCaseJsonFromSyncableCaseData(json_node));
    ++m_parsedCaseCount;
}


std::unique_ptr<std::vector<std::shared_ptr<Case>>> SyncCaseIncrementalParser::ReleaseParseableCases()
{
    ASSERT(m_state != State::Finalized);

    // we only need to determine what cases are parseable when receiving binary data
    if( !m_caseJsonLengthWhenProcessingBinaryData.has_value() )
        return std::exchange(m_currentCases, nullptr);

    // quit out if there are no cases or if we we have already determined that the current cases are not parseable
    if( m_currentCases == nullptr || !m_currentCasesAreParseable )
        return nullptr;

    size_t parseable_case_count = 0;

    for( ; parseable_case_count < m_currentCases->size(); ++parseable_case_count )
    {
        m_currentCases->at(parseable_case_count)->ForeachDefinedBinaryCaseItem(
            [&](const BinaryCaseItem& /*binary_case_item*/, const CaseItemIndex& /*index*/)
            {
                m_currentCasesAreParseable = false;
            });

        // once a case is found that is not parseable, return a sequential subset of cases that are parseable
        if( !m_currentCasesAreParseable )
        {
            if( parseable_case_count == 0 )
                return nullptr;

            const auto parseable_cases_begin = m_currentCases->cbegin();
            const auto parseable_cases_end = m_currentCases->cbegin() + parseable_case_count;

            auto parseable_cases = std::make_unique<std::vector<std::shared_ptr<Case>>>(parseable_cases_begin, parseable_cases_end);

            m_currentCases->erase(parseable_cases_begin, parseable_cases_end);

            return parseable_cases;
        }
    }

    ASSERT(m_currentCasesAreParseable);

    return std::exchange(m_currentCases, nullptr);
}
