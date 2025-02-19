#pragma once

#include <zDataO/zDataO.h>
#include <zToolsO/span.h>

class BinaryCaseItem;
class Case;
class CaseAccess;
class CaseConstructionReporter;
class CaseItemIndex;
class CaseJsonParserHelper;
class HeaderList;
class SyncBinaryDataUploadManager;
class SyncBinaryDataUploadManager;
class SyncCaseIncrementalParser;
class SyncCaseJsonSerializer;


// --------------------------------------------------------------------------
// SyncCaseSerializer
// --------------------------------------------------------------------------

class ZDATAO_API SyncCaseSerializer
{
    friend class SyncCaseIncrementalParser;

public:
    enum class Version
    {
        V1, // CSPro 7.0 - 7.3 | case data written as a text blob
        V2, // CSPro 7.4 - 8.0 | case data written using JSON objects
        V3  // CSPro 8.1+      | case data written in the JSON format for cases introduced in CSPro 8.0
    };

    // Creates a serializer based on the version number.
    // If case_json_parser_helper is null, it will be set to an object that creates SyncWithDataBinaryContentReader objects when parsing binary data.
    SyncCaseSerializer(std::shared_ptr<const CaseAccess> case_access, Version version,
                       std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper = nullptr);

    // Creates a serializer based on the CSWeb API version.
    static SyncCaseSerializer CreateFromCSWebApiVersion(std::shared_ptr<const CaseAccess> case_access, double csweb_api_version,
                                                        std::shared_ptr<CaseJsonParserHelper> case_json_parser_helper = nullptr);

    // Creates a serializer based on the CSPro version (for Bluetooth syncs), with the second version reading the CSPro version from the headers.
    static SyncCaseSerializer CreateFromCSProVersion(std::shared_ptr<const CaseAccess> case_access, double cspro_version);
    static SyncCaseSerializer CreateFromCSProVersion(std::shared_ptr<const CaseAccess> case_access, const HeaderList& request_headers);

    // Returns the object that creates syncable case JSON for the case.
    SyncCaseJsonSerializer& GetSyncCaseJsonSerializer() { return *m_syncCaseJsonSerializer; }

    // Returns the syncable case JSON for the case, potentially wrapping the single case JSON in an array.
    std::string GetSyncableJson(const Case& data_case, bool wrap_single_case_in_array);

    // Returns the syncable case JSON array for the cases.
    std::string GetSyncableJson(cs::span<const Case* const> cases);

    // Returns the syncable case data for the cases.
    // If sync_binary_data_upload_manager is non-null and contains binary data, the response will instead be written in the binary data format.
    std::string GetSyncableCaseData(cs::span<const Case* const> cases, const SyncBinaryDataUploadManager* sync_binary_data_upload_manager);

    // Parses the syncable case data, whether a JSON array of cases, or cases written in the binary data format.
    // Errors in construction are reported using a SyncLogCaseConstructionReporter.
    // JSON errors are thrown using JsonParseException and binary data errors are thrown using SyncError.
    std::vector<std::shared_ptr<Case>> ParseSyncableCaseData(std::string_view case_data_sv);

    // Parses a single case from syncable case JSON. Errors are handled as specified above.
    std::unique_ptr<Case> ParseCaseJsonFromSyncableCaseData(const JsonNode& json_node);

    // Calls the metadata reader to get a JSON node with metadata about binary data, and when the metadata has a value,
    // calls the content reader to get the content, which is then associated with relevant cases.
    void ParseSyncableCaseDataBinaryData(std::vector<std::shared_ptr<Case>>& cases,
                                         const std::function<std::optional<JsonNode>()>& metadata_reader,
                                         const std::function<std::vector<std::byte>(const std::string& signature)>& content_reader);

private:
    class BinaryParser;

    static void WriteInt32(std::ostream& output_stream, int32_t value);
    void WriteBinaryCaseItemWithData(std::ostream& output_stream, const BinaryCaseItem& binary_case_item, const CaseItemIndex& index) const;

    std::vector<std::shared_ptr<Case>> ParseCasesJsonFromSyncableCaseData(std::string_view case_json_sv);

    // Parses the header, returning a BinaryParser if the case data is written in the binary data format.
    static std::optional<BinaryParser> ParseSyncableCaseDataHeader(std::string_view case_data_sv);

    // Parses the binary data written after the case JSON when case data is written in the binary data format.
    void ParseSyncableCaseDataBinaryData(BinaryParser& binary_parser, std::vector<std::shared_ptr<Case>>& cases);

private:
    std::shared_ptr<const CaseAccess> m_caseAccess;
    std::shared_ptr<SyncCaseJsonSerializer> m_syncCaseJsonSerializer;
    const char* m_caseUuidKey;
    std::shared_ptr<CaseConstructionReporter> m_syncLogCaseConstructionReporter;
};
