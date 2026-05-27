#include "stdafx.h"
#include "CSWebRepository.h"
#include "CaseIterator.h"
#include "CSWebBinaryContentReader.h"
#include "CSWebRepositoryCache.h"
#include "CSWebRepositoryIterators.h"
#include "CSWebRepositoryJsonKeys.h"
#include "SyncBinaryDataUploadManager.h"
#include "SyncCaseJsonSerializer.h"
#include "SyncCaseSerializer.h"
#include <zUtilO/CSProExecutables.h>
#include <zUtilO/Versioning.h>
#include <zToolsO/Encoders.h>
#include <zNetwork/ConnectResponse.h>
#include <zNetwork/CSWebConnection.h>
#include <zNetwork/HttpConnection.h>
#include <zNetwork/SyncException.h>
#include <zSyncO/SyncDictionaryInfo.h>


// --------------------------------------------------------------------------
// CSWebRepository
// --------------------------------------------------------------------------

CSWebRepository::CSWebRepository(std::shared_ptr<const CaseAccess> case_access, const DataRepositoryAccess access_type)
    :   DataRepository(DataRepositoryType::CSWeb, std::move(case_access), access_type),
        m_deviceId(CreateDeviceId()),
        m_syncableDictionaryName(m_caseAccess->GetDataDict().GetSyncableName())
{
}


CSWebRepository::~CSWebRepository()
{
}


std::string CSWebRepository::CreateDeviceId()
{
    return SO::Concatenate(
#ifdef WIN_DESKTOP
        Path::GetFilenameWithoutExtension(CSProExecutables::GetModuleFilePath()),
#else
        GetOperatingSystemName(),
#endif
        " ", Versioning::NumberDetailedText,
        ": ", GetDeviceId());
}


void CSWebRepository::RethrowException(const std::exception& exception, const std::string& dictionary_name, std::unique_ptr<SyncErrorFormatter>& sync_error_formatter)
{
    if( sync_error_formatter == nullptr )
        sync_error_formatter = std::make_unique<SyncErrorFormatter>();

    const std::string message = sync_error_formatter->GetFormattedError(exception);

    const SyncError* const sync_error = dynamic_cast<const SyncError*>(&exception);

    // because the generic error message starts with "Error interacting with CSWeb",
    // the message will not be prefaced with "Error communicating..."
    if( sync_error != nullptr &&
        sync_error->GetErrorMessageNumber() == CSWebConnection::GetGenericErrorMessageNumber() )
    {
        throw DataRepositoryException::IOError(message);
    }

    else
    {
        throw DataRepositoryException::IOError("Error communicating with CSWeb data source '%s': %s",
                                               dictionary_name.c_str(), message.c_str());
    }
}


void CSWebRepository::RethrowException(const std::exception& exception)
{
    RethrowException(exception, GetDictionaryNameForErrors(), m_syncErrorFormatter);
}


std::string CSWebRepository::GetDictionaryNameForErrors() const
{
    ASSERT(m_caseAccess != nullptr);

    return SyncDictionaryInfo::GetDisplayName(m_syncableDictionaryName, m_caseAccess->GetDataDict().GetName());
}


void CSWebRepository::ResetCaseObjects()
{
    if( m_cswebConnection == nullptr )
    {
        m_syncCaseSerializer.reset();
    }

    else
    {
        m_syncCaseSerializer = CreateSyncCaseSerializer(this, m_caseAccess, m_cswebConnection);
    }

    m_syncBinaryDataUploadManager.reset();
    m_temporaryCase.reset();
}


void CSWebRepository::ModifyCaseAccess(std::shared_ptr<const CaseAccess> case_access)
{
    m_caseAccess = std::move(case_access);
    ASSERT(m_caseAccess != nullptr);

    ResetCaseObjects();
}


void CSWebRepository::Open(const DataRepositoryOpenFlag open_flag)
{
    ASSERT(m_cswebConnection == nullptr && m_permissions.empty() && m_cache == nullptr);

    // allow the syncable dictionary name to be overridden in the connection string
    const std::string* const dictionary_name_override = m_connectionString.GetProperty(CSProperty::dictionaryName);

    if( dictionary_name_override != nullptr )
    {
        if( !CIMSAString::IsName(*dictionary_name_override) )
        {
            throw DataRepositoryException::IOError("No dictionary named '%s' can exist on the CSWeb server because it is not a valid name.",
                                                   dictionary_name_override->c_str());
        }

        m_syncableDictionaryName = *dictionary_name_override;
    }

    std::unique_ptr<HttpConnection> http_connection = HttpConnection::Create();

    if( http_connection == nullptr)
        throw DataRepositoryException::IOError("HTTP access is not supported on this platform.");

    try
    {
        std::unique_ptr<CSWebConnection> csweb_connection = std::make_unique<CSWebConnection>(std::move(http_connection),
                                                                                              m_connectionString.ToString(),
                                                                                              CreateLoginCredentials(m_connectionString));

        const std::unique_ptr<const ConnectResponse> csweb_connect_response = csweb_connection->Connect(CSWebVersion::V3);
        ASSERT(csweb_connection->GetUser().has_value());

        if( csweb_connection->GetUser()->IsAdmin() )
        {
            throw DataRepositoryException::IOError("You cannot use the CSWeb data source while connecting with role '%s'.",
                                                    csweb_connection->GetUser()->role_name.c_str());
        }

        const JsonNode dictionary_metadata_json_node = EnsureDictionaryExistsAndUserHasPermissions(*csweb_connection, open_flag);

        ASSERT(m_permissions.find(CSWebDictionaryPermission::Read) != m_permissions.cend());
        ASSERT(( m_permissions.find(CSWebDictionaryPermission::Write) != m_permissions.cend() ) || IsReadOnly());

        // potentially open the cache (enabled by default)
        if( m_connectionString.HasPropertyOrDefault(CSProperty::cacheLocally, CSValue::true_, true) )
        {
            try
            {
                m_cache = CSWebRepositoryCache::Create(*this, csweb_connect_response->GetServerDeviceId(),
                                                       *csweb_connection->GetUser(), dictionary_metadata_json_node);
            }
            catch(...) { ASSERT(false); }
        }

        m_cswebConnection = std::move(csweb_connection);

        ResetCaseObjects();
    }

    catch( const DataRepositoryException::Error& )
    {
        throw;
    }

    catch( const std::exception& exception )
    {
        const SyncError* const sync_error = dynamic_cast<const SyncError*>(&exception);

        if( sync_error != nullptr )
        {
            if( sync_error->GetErrorMessageNumber() == 100131 )
                throw DataRepositoryException::IOError("You can only connect to CSWeb servers that are running version 8.1 or greater.");

            if( sync_error->GetErrorMessageNumber() == 100144 )
            {
                throw DataRepositoryException::IOError("No dictionary named '%s' exists on the CSWeb server.",
                                                       GetDictionaryNameForErrors().c_str());
            }
        }

        RethrowException(exception);
    }
}


LoginCredentials CSWebRepository::CreateLoginCredentials(const ConnectionString& connection_string)
{
    const std::string* const username = connection_string.GetProperty(CSProperty::username);

    if( username != nullptr )
    {
        const std::string* const password =  connection_string.GetProperty(CSProperty::password);

        if( password != nullptr )
            return LoginCredentials(*username, *password);
    }

    return LoginCredentials(std::make_unique<LoginAccessorWithoutBluetoothSupport>());
}


JsonNode CSWebRepository::EnsureDictionaryExistsAndUserHasPermissions(CSWebConnection& csweb_connection, const DataRepositoryOpenFlag open_flag)
{
    bool delete_existing_data = ( open_flag == DataRepositoryOpenFlag::CreateNew ||
                                  m_accessType == DataRepositoryAccess::BatchOutput );

    std::optional<JsonNode> dictionary_metadata_json_node;

    auto get_dictionary_metadata_and_check_permissions = [&]()
    {
        dictionary_metadata_json_node = csweb_connection.GetDictionaryMetadata(m_syncableDictionaryName);

        // ensure that the user (via their role) has permission to access this data
        m_permissions = CSWebConnection::ParseDictionaryPermissions(*dictionary_metadata_json_node);

        EnsureUserHasPermission(csweb_connection, CSWebDictionaryPermission::Read);

        if( !IsReadOnly() )
            EnsureUserHasPermission(csweb_connection, CSWebDictionaryPermission::Write);

        if( delete_existing_data )
            EnsureUserHasPermission(csweb_connection, CSWebDictionaryPermission::Clear);
    };

    try
    {
        get_dictionary_metadata_and_check_permissions();
    }

    catch( const SyncError& exception )
    {
        if( exception.GetHttpResponseCode() != HttpResponse::Status_404_NotFound )
            throw;

        // if the dictionary does not exist, we can upload it when the data source does not need to exist
        if( open_flag == DataRepositoryOpenFlag::OpenMustExist )
        {
            throw DataRepositoryException::IOError("The data source '%s' does not exist on CSWeb.",
                                                    GetDictionaryNameForErrors().c_str());
        }

        ASSERT(open_flag == DataRepositoryOpenFlag::CreateNew || open_flag == DataRepositoryOpenFlag::OpenOrCreate);

        PutDictionaryThatDoesNotExist(csweb_connection, m_caseAccess->GetDataDict(), m_syncableDictionaryName);
        delete_existing_data = false;
    }

    if( delete_existing_data )
    {
        csweb_connection.DeleteDictionaryData(m_syncableDictionaryName);
        dictionary_metadata_json_node.reset();
    }

    if( !dictionary_metadata_json_node.has_value() )
        get_dictionary_metadata_and_check_permissions();

    const std::string key_structure = dictionary_metadata_json_node->GetOrConstruct<std::string>(JK::dictionaryKeyStructure);

    if( key_structure != CalculateDictionaryKeyStructure(m_caseAccess->GetDataDict()) )
    {
        throw DataRepositoryException::IOError("The dictionary '%s' on CSWeb is incompatible with this version.",
                                                GetDictionaryNameForErrors().c_str());
    }

    return std::move(*dictionary_metadata_json_node);
}


void CSWebRepository::PutDictionaryThatDoesNotExist(CSWebConnection& csweb_connection, const CDataDict& dictionary, const std::string& syncable_name)
{
    cs::non_null_shared_or_raw_ptr<const CDataDict> dictionary_to_put = &dictionary;

    // in the rare instance when the syncable name is overridden in the connection string, create a new dictionary
    // with that syncable name prior to syncing; eventually this could be optimized by setting the syncable name
    // in the header when hitting the dictionaries endpoint
    if( syncable_name != dictionary.GetSyncableName() )
    {
        auto dictionary_copy = std::make_unique<CDataDict>(dictionary);
        dictionary_copy->SetSyncableName(syncable_name);
        dictionary_to_put = std::move(dictionary_copy);
    }

    csweb_connection.PutDictionarySpec(dictionary_to_put->GetJson(true));
}


std::string CSWebRepository::CalculateDictionaryKeyStructure(const CDataDict& dictionary)
{
    // the dictionary key structure is, for each first level ID:
    // - the length
    // - the content type (as serialized by JSON)
    // - for numeric items, the zero fill setting, serialized as 1/0
    const CDictRecord& dict_id_record = *dictionary.GetLevel(0).GetIdItemsRec();
    std::string key_structure;

    for( int i = 0; i < dict_id_record.GetNumItems(); ++i )
    {
        const CDictItem& dict_id_item = *dict_id_record.GetItem(i);

        key_structure.append(IntToString(dict_id_item.GetLen()))
                     .append(ToString(dict_id_item.GetContentType(), true));

        if( dict_id_item.GetContentType() == ContentType::Numeric )
            key_structure.push_back(dict_id_item.GetZeroFill() ? '1' : '0');
    }

    return key_structure;
}


void CSWebRepository::EnsureUserHasPermission(const CSWebConnection& csweb_connection, const CSWebDictionaryPermission permission)
{
    if( m_permissions.find(permission) != m_permissions.cend() )
        return;

    const char* const permission_text =
        ( permission == CSWebDictionaryPermission::Read )  ? "read" :
        ( permission == CSWebDictionaryPermission::Write ) ? "write" :
        ( permission == CSWebDictionaryPermission::Clear ) ? "delete" :
                                                             ReturnProgrammingError("");
    ASSERT(csweb_connection.GetUser().has_value());

    throw DataRepositoryException::IOError("The data source '%s' cannot be opened with %s access due to insufficient privileges using the role '%s'.",
                                            m_syncableDictionaryName.c_str(),
                                            permission_text,
                                            csweb_connection.GetUser()->role_name.c_str());
}


void CSWebRepository::ToggleReadWriteMode()
{
    ASSERT(m_accessType == DataRepositoryAccess::ReadOnly || m_accessType == DataRepositoryAccess::ReadWrite);

    if( m_accessType == DataRepositoryAccess::ReadOnly )
    {
        EnsureUserHasPermission(*m_cswebConnection, CSWebDictionaryPermission::Write);
        m_accessType = DataRepositoryAccess::ReadWrite;
    }

    else
    {
        m_accessType = DataRepositoryAccess::ReadOnly;
    }
}


void CSWebRepository::Close()
{
    m_cswebConnection.reset();
    m_permissions.clear();
    m_cache.reset();
}


void CSWebRepository::DeleteRepository()
{
    try
    {
        m_cswebConnection->DeleteDictionaryData(m_syncableDictionaryName);

        CSWebRepositoryCache::DeleteCache(m_cache);
    }

    catch( const std::exception& exception )
    {
        RethrowException(exception);
    }
}


std::unique_ptr<SyncCaseSerializer> CSWebRepository::CreateSyncCaseSerializer(std::variant<CSWebRepository*, UniqueId> repository_or_repository_id,
                                                                              std::shared_ptr<const CaseAccess> case_access,
                                                                              std::shared_ptr<CSWebConnection> csweb_connection)
{
    auto case_json_parser_helper = std::make_unique<CSWebCaseJsonParserHelper>(std::move(repository_or_repository_id), case_access, std::move(csweb_connection));

    return std::make_unique<SyncCaseSerializer>(std::move(case_access), SyncCaseSerializer::Version::V3, std::move(case_json_parser_helper));
}


size_t CSWebRepository::ParseJsonCount(const JsonNode& json_node)
{
    ASSERT(!json_node.IsEmpty());

    return json_node.Get<size_t>(JK::count);
}


CaseKey CSWebRepository::ParseJsonIdentifier(const JsonNode& json_node)
{
    ASSERT(!json_node.IsEmpty());

#ifdef _DEBUG
    CaseKey case_key =
#else
    return
#endif
        CaseKey(json_node.Get<std::string>(JK::key),
                json_node.Get<double>(JK::position));

#ifdef _DEBUG
    ASSERT(case_key.GetPositionInRepository() >= 1);
    return case_key;
#endif
}


CaseSummary CSWebRepository::ParseJsonSummary(const JsonNode& json_node)
{
    ASSERT(!json_node.IsEmpty());

    return CaseSummary(ParseJsonIdentifier(json_node),
                       json_node.GetOrConstruct<std::string>(JK::label),
                       json_node.GetOrDefault(JK::deleted, false),
                       json_node.GetOrDefault(JK::verified, false),
                       json_node.GetOrDefault(JK::partialSaveMode, PartialSaveMode::None),
                       json_node.GetOrConstruct<std::string>(JK::caseNote));
}


void CSWebRepository::ParseJsonCase(Case& data_case, const JsonNode& case_json_node, const JsonNode& metadata_json_node, SyncCaseSerializer& sync_case_serializer)
{
    ASSERT(!case_json_node.IsEmpty() && !metadata_json_node.IsEmpty());

    sync_case_serializer.GetSyncCaseJsonSerializer().ParseCase(data_case, case_json_node);

    data_case.SetPositionInRepository(metadata_json_node.Get<double>(JK::position));
    ASSERT(data_case.GetPositionInRepository() >= 1);
}


void CSWebRepository::ParseJsonCase(Case& data_case, const CSWebCaseResponse& case_response) const
{
    ASSERT(case_response.IsCaseResponse());

    // CSWeb returns empty objects if the case has not been modified from what we have in the cache,
    // so we can return what we have in the cache
    if( case_response.IsContentEmpty() )
    {
        ParseJsonCaseFromCache(data_case, case_response);
    }

    else
    {
        ParseJsonCase(data_case, case_response.GetContentJsonNode(), case_response.GetMetadataJsonNode(), *m_syncCaseSerializer);
    }

}

void CSWebRepository::ParseJsonCaseFromCache(Case& data_case, const CSWebCaseResponse& case_response) const
{
    ASSERT(case_response.IsCaseResponse() && case_response.IsContentEmpty());

    try
    {
        if( m_cache == nullptr )
            throw ProgrammingErrorException();

        return ParseJsonCase(data_case, m_cache->RetrieveCase(CSWebCaseQuery::cases, case_response));
    }

    catch(...)
    {
        ASSERT(false);

        std::string position_text;
        try { position_text = IntToString(case_response.GetPosition()); } catch(...) { ASSERT(false); }

        throw DataRepositoryException::IOError("The CSWeb cache does not contain the case with position: %s",
                                                !position_text.empty() ? position_text.c_str() : "<unknown>");
    }
}


size_t CSWebRepository::ExecuteCaseCountQuery(const std::string_view arguments_json_text_sv) const
{
    std::optional<JsonNode> json_node;

    // check the cache
    if( m_cache != nullptr )
        json_node = m_cache->RetrieveQuery(arguments_json_text_sv);

    if( !json_node.has_value() )
    {
        json_node = m_cswebConnection->QueryCasesRepository(m_syncableDictionaryName, arguments_json_text_sv);

        // update the cache
        if( m_cache != nullptr )
            m_cache->CacheQuery(arguments_json_text_sv, *json_node);
    }

    return ParseJsonCount(*json_node);
}


CSWebCaseQueryResponse CSWebRepository::ExecuteCaseQuery(const CSWebCaseQuery query, const std::string& arguments_json_text) const
{
    std::unique_ptr<std::string> cache_header_json_text;

    // check the cache
    if( m_cache != nullptr )
    {
        CSWebCacheCaseResponse cache_case_response = m_cache->RetrieveCaseQuery(query, arguments_json_text);

        // use an up-to-date case query...
        if( std::holds_alternative<CSWebCaseQueryResponse>(cache_case_response) )
        {
            return std::move(std::get<CSWebCaseQueryResponse>(cache_case_response));
        }

        // ...or potentially query with information about previously-retrieved cases
        else if( std::holds_alternative<CSWebCacheStaleCaseData>(cache_case_response) )
        {
            cache_header_json_text = std::get<CSWebCacheStaleCaseData>(cache_case_response).CreateCacheHeaderJsonText();
        }
    }

    // if not in the cache, or not up-to-date, query CSWeb
    CSWebCaseQueryResponse case_query_response(query, m_cswebConnection->QueryCasesRepository(
        m_syncableDictionaryName,
        arguments_json_text,
        std::move(cache_header_json_text)
    ));

    // update the cache
    if( m_cache != nullptr )
        m_cache->CacheCaseQuery(arguments_json_text, case_query_response);

    return case_query_response;
}


CSWebCaseResponse CSWebRepository::ExecuteSingleCaseQuery(const CSWebCaseQuery query, const char* const status,
                                                          const char* const filter_type, const std::string_view filter_value_sv) const
{
    ASSERT(query == CSWebCaseQuery::cases || query == CSWebCaseQuery::identifiers);
    const char* const content_key = GetCSWebContentKey(query);

    const std::string arguments_json_text = SO::Concatenate(
        R"({"content":")", content_key,
        R"(","status":")", status,
        R"(","filter":{"operator":"=","type":")", filter_type,
        R"(","value":)", filter_value_sv,
        R"(},"order":"position","limit":1)",
        ( query == CSWebCaseQuery::cases || m_cache != nullptr ) ? R"(,"requestMetadata":true})" : R"(})"
    );

    CSWebCaseQueryResponse case_query_response = ExecuteCaseQuery(query, arguments_json_text);
    ASSERT(case_query_response.GetCaseCount() <= 1);

    if( case_query_response.GetCaseCount() == 0 )
        throw DataRepositoryException::CaseNotFound();

    return case_query_response.GetCase(0);
}


bool CSWebRepository::ContainsCase(const std::string& key)
{
    // if not found in the cache here, the cache will be further checked/updated in ExecuteCaseCountQuery
    if( m_cache != nullptr && m_cache->HasNonDeletedCaseByKey(key) )
        return true;

    try
    {
        const std::string arguments_json_text = SO::Concatenate(R"({"content":"count","status":"notDeletedOnly","filter":{"operator":"=","type":"key","value":)",
                                                                Encoders::ToJsonString(key),
                                                                "}}");

        return ( ExecuteCaseCountQuery(arguments_json_text) > 0 );
    }

    catch( const std::exception& )
    {
        // ContainsCase cannot throw exceptions so assume that the case does not exist -- CSWEB_TODO revisit?
        return false;
    }
}


void CSWebRepository::PopulateCaseIdentifiers(std::string& key, std::string& uuid, double& position_in_repository)
{
    // the cache will be checked/updated in ExecuteSingleCaseQuery
    try
    {
        // search by key
        if( !key.empty() )
        {
            const CSWebCaseResponse identifier_response = ExecuteSingleCaseQuery(
                CSWebCaseQuery::identifiers, JV::notDeletedOnly, JK::key,
                Encoders::ToJsonString(key)
            );

            ASSERT(key == identifier_response.GetKey());
            uuid = identifier_response.GetUuid();
            position_in_repository = identifier_response.GetPosition<double>();
        }

        // or by UUID
        else if( !uuid.empty() )
        {
            const CSWebCaseResponse identifier_response = ExecuteSingleCaseQuery(
                CSWebCaseQuery::identifiers, JV::all, JK::uuid,
                Encoders::ToJsonString(uuid)
            );

            key = identifier_response.GetKey();
            ASSERT(uuid == identifier_response.GetUuid());
            position_in_repository = identifier_response.GetPosition<double>();
        }

        // or by position
        else
        {
            ASSERT(static_cast<int64_t>(position_in_repository) == position_in_repository);

            const CSWebCaseResponse identifier_response = ExecuteSingleCaseQuery(
                CSWebCaseQuery::identifiers, JV::all, JK::position,
                IntToString(static_cast<int64_t>(position_in_repository))
            );

            key = identifier_response.GetKey();
            uuid = identifier_response.GetUuid();
            ASSERT(position_in_repository == identifier_response.GetPosition<double>());
        }
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
        throw;
    }

    catch( const std::exception& exception )
    {
        RethrowException(exception); // CSWEB_TODO revisit when this is a CSWeb communication error?
    }
}


DataRepositoryUniqueCaseIdentifer CSWebRepository::GetUniqueCaseIdentifer(const CaseKey& case_key)
{
    return case_key.GetPositionInRepository();
}


template<bool requires_metadata/* = false*/>
std::string CSWebRepository::CreateKeySearchQuery(const char* const content, const CaseIteratorSettings& iterator_settings,
                                                  const size_t offset, const size_t limit)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::content, content);

    iterator_settings.WriteJson(*json_writer, false);

    if( requires_metadata || m_cache != nullptr )
        json_writer->Write(JK::requestMetadata, true);

    if( offset != 0 )
        json_writer->Write(JK::offset, offset);

    if( limit != SIZE_MAX )
        json_writer->Write(JK::limit, limit);

    json_writer->EndObject();

    return json_writer->ReleaseString();
}


std::optional<CaseKey> CSWebRepository::FindCaseKey(const CaseIterationMethod iteration_method, const CaseIterationOrder iteration_order,
                                                    const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    // the cache will be checked/updated in ExecuteCaseQuery
    try
    {
        const CaseIteratorSettings iterator_settings(CaseIterationCaseStatus::NotDeletedOnly, iteration_method, iteration_order, start_parameters);

        const std::string arguments_json_text = CreateKeySearchQuery(JK::identifiers, iterator_settings, 0, 1);

        const CSWebCaseQueryResponse case_query_response = ExecuteCaseQuery(CSWebCaseQuery::identifiers, arguments_json_text);
        ASSERT(case_query_response.GetCaseCount() <= 1);

        if( case_query_response.GetCaseCount() == 0 )
            return std::nullopt;

        const CSWebCaseResponse identifier_response = case_query_response.GetCase(0);

        return ParseJsonIdentifier(identifier_response.GetContentJsonNode());
    }

    catch( const std::exception& )
    {
        // FindCaseKey cannot throw exceptions so assume that the case does not exist -- CSWEB_TODO revisit?
        return std::nullopt;
    }
}


void CSWebRepository::ReadCase(Case& data_case, const char* const status, const char* const filter_type, const std::string_view filter_value_sv)
{
    ASSERT(m_syncCaseSerializer != nullptr);

    // the cache will be checked/updated in ExecuteSingleCaseQuery
    try
    {
        const CSWebCaseResponse case_response = ExecuteSingleCaseQuery(
            CSWebCaseQuery::cases, status, filter_type, filter_value_sv
        );

        ParseJsonCase(data_case, case_response);
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
        throw;
    }

    catch( const std::exception& exception )
    {
        RethrowException(exception); // CSWEB_TODO revisit when this is a CSWeb communication error?
    }
}


void CSWebRepository::ReadCase(Case& data_case, const std::string& key)
{
    ReadCase(data_case, JV::notDeletedOnly, JK::key, Encoders::ToJsonString(key));
}


void CSWebRepository::ReadCase(Case& data_case, const double position_in_repository)
{
    ASSERT(static_cast<int64_t>(position_in_repository) == position_in_repository);
    const int64_t position = static_cast<int64_t>(position_in_repository);

    ReadCase(data_case, JV::all, JK::position, IntToString(position));
}


void CSWebRepository::ReadCaseByUuid(Case& data_case, const std::string& uuid)
{
    ReadCase(data_case, JV::all, JK::uuid, Encoders::ToJsonString(uuid));
}


void CSWebRepository::WriteCase(Case& data_case, const WriteCaseParameter* const write_case_parameter/* = nullptr*/)
{
    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    // if this case should replace an existing case, we need to make sure that we reuse its UUID
    std::optional<CSWebCaseResponse> identifier_response;

    if( write_case_parameter != nullptr )
    {
        if( write_case_parameter->IsModifyParameter() )
        {
             identifier_response = ExecuteSingleCaseQuery(
                CSWebCaseQuery::identifiers, JV::all, JK::position,
                IntToString(static_cast<int64_t>(write_case_parameter->GetPositionInRepository()))
            );
        }
    }

    else if( m_accessType == DataRepositoryAccess::ReadWrite )
    {
        try
        {
            identifier_response = ExecuteSingleCaseQuery(
                CSWebCaseQuery::identifiers, JV::notDeletedOnly, JK::key,
                Encoders::ToJsonString(data_case.GetKey())
            );
        }

        catch( const DataRepositoryException::CaseNotFound& )
        {
            // the case not existing is fine as this means that this is a brand new case
        }
    }

    data_case.SetUuid(identifier_response.has_value() ? identifier_response->GetUuid() : CreateUuid());

    WriteCaseWorker(data_case);
}


void CSWebRepository::WriteCaseWorker(Case& data_case)
{
    ASSERT(!IsReadOnly());
    ASSERT(m_syncCaseSerializer != nullptr);
    ASSERT(!data_case.GetUuid().empty());

    // update the vector clock
    data_case.GetVectorClock().increment(m_deviceId);

    // update the cache because the server will now have a new revision number
    if( m_cache != nullptr )
        m_cache->MarkCacheDirty();

    if( !m_syncBinaryDataUploadManager.has_value() )
        m_syncBinaryDataUploadManager = CreateSyncBinaryDataUploadManager();

    // when using binary data, determine what needs to be uploaded
    if( *m_syncBinaryDataUploadManager != nullptr )
    {
        (*m_syncBinaryDataUploadManager)->ResetForNextChunk();
        (*m_syncBinaryDataUploadManager)->AnalyzeCaseBinaryData(data_case);
    }

    try
    {
        std::string case_data = m_syncCaseSerializer->GetSyncableCaseData({ &data_case }, m_syncBinaryDataUploadManager->get());
        m_cswebConnection->UploadCase(m_syncableDictionaryName, m_deviceId, std::move(case_data));
    }

    catch( const std::exception& exception )
    {
        RethrowException(exception); // CSWEB_TODO revisit when this is a CSWeb communication error?
    }

    // reset the position in the repository as we do not know the value CSWeb will use for this
    data_case.SetPositionInRepository(-1);

    // cache any binary data
    if( m_cache != nullptr && *m_syncBinaryDataUploadManager != nullptr )
        m_cache->CacheBinaryData(*(*m_syncBinaryDataUploadManager));
}


void CSWebRepository::DeleteCase(const double position_in_repository, const bool deleted/* = true*/)
{
    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    /* unneeded as this is done in WriteCaseWorker
    // update the cache because the server will now have a new revision number
    if( m_cache != nullptr )
        m_cache->MarkCacheDirty();
    */

    // CSWEB_TODO: for deleting cases, revisit if we should hit the delete endpoint, sending the device ID in the header so CSWeb can update the vector clock
    try
    {
        // read the case so that we can flip the deleted flag
        if( m_temporaryCase == nullptr )
            m_temporaryCase = m_caseAccess->CreateCase();

        ReadCase(*m_temporaryCase, position_in_repository);

        // only modify the case when the deleted status has changed
        if( m_temporaryCase->GetDeleted() != deleted )
        {
            m_temporaryCase->SetDeleted(deleted);
            WriteCaseWorker(*m_temporaryCase);
        }
    }

    catch( const std::exception& exception )
    {
        RethrowException(exception); // CSWEB_TODO revisit when this is a CSWeb communication error?
    }
}


size_t CSWebRepository::GetNumberCases()
{
    // the cache will be checked/updated in ExecuteCaseCountQuery
    try
    {
        constexpr std::string_view arguments_json_text_sv = R"({"content":"count","status":"notDeletedOnly"})";

        return ExecuteCaseCountQuery(arguments_json_text_sv);
    }

    catch( const std::exception& )
    {
        // GetNumberCases seems like it should not throw exceptions, though some of the other repositories do, so what to do? -- CSWEB_TODO revisit?
        return 0;
    }
}


size_t CSWebRepository::GetNumberCases(const CaseIterationCaseStatus case_status, const CaseIteratorParameters* const start_parameters/* = nullptr*/)
{
    if( case_status == CaseIterationCaseStatus::NotDeletedOnly && start_parameters == nullptr )
        return CSWebRepository::GetNumberCases();

    // the cache will be checked/updated in ExecuteCaseCountQuery
    try
    {
        const CaseIteratorSettings iterator_settings(case_status, std::nullopt, std::nullopt, start_parameters);

        const std::string arguments_json_text = CreateKeySearchQuery(JK::count, iterator_settings, 0, 1);

        return ExecuteCaseCountQuery(arguments_json_text);
    }

    catch( const std::exception& )
    {
        // GetNumberCases seems like it should not throw exceptions, though some of the other repositories do, so what to do? -- CSWEB_TODO revisit?
        return 0;
    }
}


std::unique_ptr<CaseIterator> CSWebRepository::CreateIterator(const CaseIterationContent iteration_content,
                                                              const CaseIteratorSettings& iterator_settings,
                                                              const size_t offset/* = 0*/, const size_t limit/* = SIZE_MAX*/)
{
    return std::make_unique<CSWebRepositoryIterator>(*this, iteration_content, iterator_settings, offset, limit);
}


std::unique_ptr<CDataDict> CSWebRepository::GetDictionary(const std::string& dictionary_name, const ConnectionString& connection_string)
{
    const std::unique_ptr<CSWebConnection> csweb_connection = std::make_unique<CSWebConnection>(HttpConnection::Create(),
                                                                                                connection_string.GetUrl(),
                                                                                                CreateLoginCredentials(connection_string));
    csweb_connection->Connect(CSWebVersion::V3);

    const std::string dictionary_json = csweb_connection->GetDictionarySpec(dictionary_name);

    return CDataDict::CreateFromJson<std::unique_ptr<CDataDict>>(Json::Parse(dictionary_json));
}



// --------------------------------------------------------------------------
// CSWebRepositoryIterator
// --------------------------------------------------------------------------

CSWebRepositoryIterator::CSWebRepositoryIterator(CSWebRepository& csweb_repository, const CaseIterationContent iteration_content,
                                                 CaseIteratorSettings iterator_settings, const size_t offset, const size_t limit)
    :   m_cswebRepository(csweb_repository),
        m_query(( iteration_content == CaseIterationContent::CaseKey )     ? CSWebCaseQuery::identifiers :
                ( iteration_content == CaseIterationContent::CaseSummary ) ? CSWebCaseQuery::summaries :
              /*( iteration_content == CaseIterationContent::Case )*/        CSWebCaseQuery::cases),
        m_iterationContent(GetCSWebContentKey(m_query)),
        m_caseIteratorSettings(std::move(iterator_settings)),
        m_offset(offset),
        m_limit(limit),
        m_limitRequestIndex(0),
        m_fullLimitRequested(false),
        m_casesRead(0)
{
}


size_t CSWebRepositoryIterator::GetQueryLimit()
{
    // uses CSWeb's actual limit for keys and summaries
    constexpr static size_t LimitForKeysSummaries = 1000;

    // because case iterators are used even when not iterating over cases (e.g., locate followed by a loadcase),
    // initally use a lower number for cases, increasing it at each subsequent request
    constexpr static size_t InitialLimitForCases = 50;

    const size_t limit = ( m_query == CSWebCaseQuery::cases ) ? std::min(++m_limitRequestIndex * InitialLimitForCases, LimitForKeysSummaries) :
                                                                LimitForKeysSummaries;

    // no need to request more cases than necessary
    const size_t potential_cases_remaining = m_limit - m_casesRead;

    if( potential_cases_remaining <= limit )
    {
        m_fullLimitRequested = true;
        return potential_cases_remaining;
    }

    return limit;
}


template<CSWebCaseQuery query>
std::optional<CSWebCaseResponse> CSWebRepositoryIterator::Step()
{
    constexpr bool requires_metadata = ( query == CSWebCaseQuery::cases );

    bool query_next_set;

    // query content for the first time...
    if( !m_queryResult.has_value() )
    {
        query_next_set = true;
    }

    // ...potentially query content again if all content was not previously received...
    else if( m_queryResult->iterator_case_pos == m_queryResult->case_count )
    {
        if( ( m_casesRead == m_limit ) ||
            ( m_fullLimitRequested && !m_queryResult->results_potentially_limited_by_csweb ) )
        {
            return std::nullopt;
        }

        query_next_set = true;

        // adjust the offset for the next query
        m_offset += m_queryResult->case_count;
    }

    // ... or process content from a previous query
    else
    {
        query_next_set = false;
    }

    if( query_next_set )
    {
        QueryNextSet(m_cswebRepository.CreateKeySearchQuery<requires_metadata>(m_iterationContent, m_caseIteratorSettings,
                                                                               m_offset, GetQueryLimit()));
        ASSERT(m_queryResult.has_value());
        ASSERT(requires_metadata == m_queryResult->case_query_response.IsCaseResponse());

        if( m_queryResult->case_count == 0 )
        {
            ASSERT(!m_queryResult->results_potentially_limited_by_csweb);
            return std::nullopt;
        }
    }

    // process the content
    ASSERT(m_queryResult->iterator_case_pos < m_queryResult->case_count);

    ++m_casesRead;

    return m_queryResult->case_query_response.GetCase(m_queryResult->iterator_case_pos++);
}


CSWebRepositoryIterator::QueryResult::QueryResult(CSWebCaseQueryResponse case_query_response_)
    :   case_query_response(std::move(case_query_response_)),
        case_count(case_query_response.GetCaseCount()),
        iterator_case_pos(0),
        results_potentially_limited_by_csweb(case_query_response.GetJsonNode().Contains(JK::resultLimit))
{
}


void CSWebRepositoryIterator::QueryNextSet(const std::string& arguments_json_text)
{
    try
    {
        // the cache will be checked/updated in ExecuteCaseQuery
        m_queryResult.emplace(m_cswebRepository.ExecuteCaseQuery(m_query, arguments_json_text));

        ASSERT(( m_casesRead + m_queryResult->case_count ) <= m_limit);
    }

    catch( const std::exception& exception )
    {
        m_cswebRepository.RethrowException(exception);
    }
}


template<typename T>
T& CSWebRepositoryIterator::GetTemporaryCaseObject()
{
    // in the rare event that the iteration type does not match the object being queried,
    // case objects will be created that can be used to access the object used for iteration
    if( !m_temporaryCaseObjects.has_value() )
        m_temporaryCaseObjects.emplace(std::make_unique<CaseKey>(), m_cswebRepository.GetCaseAccess().CreateCase());

    return *std::get<std::unique_ptr<T>>(*m_temporaryCaseObjects);
}


bool CSWebRepositoryIterator::NextCaseKey(CaseKey& case_key)
{
    if( m_query == CSWebCaseQuery::identifiers ||
        m_query == CSWebCaseQuery::summaries )
    {
        const std::optional<CSWebCaseResponse> identifier_response = Step<CSWebCaseQuery::identifiers>();

        if( !identifier_response.has_value() )
            return false;

        case_key = CSWebRepository::ParseJsonIdentifier(identifier_response->GetContentJsonNode());
    }

    else
    {
        ASSERT(m_query == CSWebCaseQuery::cases);

        Case& data_case = GetTemporaryCaseObject<Case>();

        if( !NextCase(data_case) )
            return false;

        case_key = data_case;
    }

    return true;
}


bool CSWebRepositoryIterator::NextCaseSummary(CaseSummary& case_summary)
{
    if( m_query == CSWebCaseQuery::summaries )
    {
        const std::optional<CSWebCaseResponse> summary_response = Step<CSWebCaseQuery::summaries>();

        if( !summary_response.has_value() )
            return false;

        case_summary = CSWebRepository::ParseJsonSummary(summary_response->GetContentJsonNode());
    }

    else if( m_query == CSWebCaseQuery::cases )
    {
        Case& data_case = GetTemporaryCaseObject<Case>();

        if( !NextCase(data_case) )
            return false;

        case_summary = data_case;
    }

    else
    {
        ASSERT(m_query == CSWebCaseQuery::identifiers);

        if( !NextCaseKey(case_summary) )
            return false;

        Case& data_case = GetTemporaryCaseObject<Case>();

        m_cswebRepository.ReadCase(data_case, case_summary.GetPositionInRepository());
        case_summary = data_case;
    }

    return true;
}


bool CSWebRepositoryIterator::NextCase(Case& data_case)
{
    if( m_query == CSWebCaseQuery::cases )
    {
        const std::optional<CSWebCaseResponse> case_response = Step<CSWebCaseQuery::cases>();

        if( !case_response.has_value() )
            return false;

        m_cswebRepository.ParseJsonCase(data_case, *case_response);
    }

    else
    {
        ASSERT(m_query == CSWebCaseQuery::identifiers ||
               m_query == CSWebCaseQuery::summaries);

        CaseKey& case_key = GetTemporaryCaseObject<CaseKey>();

        if( !NextCaseKey(case_key) )
            return false;

        m_cswebRepository.ReadCase(data_case, case_key.GetPositionInRepository());
    }

    return true;
}


int CSWebRepositoryIterator::GetPercentRead() const
{
    // get the number of cases if necessary
    if( !m_percentMultiplier.has_value() )
    {
        try
        {
            const size_t number_cases = m_cswebRepository.GetNumberCases(m_caseIteratorSettings.GetStatus(),
                                                                         m_caseIteratorSettings.GetParameters());
            m_percentMultiplier = CreatePercentMultiplier(number_cases);
        }

        catch(...)
        {
            // don't throw an exception if the number of cases cannot be retrieved (probably due to a network error),
            // letting the next call to Next... throw the exception
            return 0;
        }
    }

    return static_cast<int>(m_casesRead * *m_percentMultiplier);
}



// --------------------------------------------------------------------------
// CSWebRepositorySyncBinaryDataUploadManager
// --------------------------------------------------------------------------

class CSWebRepositorySyncBinaryDataUploadManager : public SyncBinaryDataUploadManager
{
public:
    CSWebRepositorySyncBinaryDataUploadManager(CSWebRepository& repository);

protected:
    void AddBinarySignaturesNotSyncedWithRemote(const Case& data_case, std::vector<std::string>& signatures_to_sync) override;

private:
    CSWebRepository& m_repository;
};


CSWebRepositorySyncBinaryDataUploadManager::CSWebRepositorySyncBinaryDataUploadManager(CSWebRepository& repository)
    :   m_repository(repository)
{
}


void CSWebRepositorySyncBinaryDataUploadManager::AddBinarySignaturesNotSyncedWithRemote(const Case& data_case, std::vector<std::string>& signatures_to_sync)
{
    data_case.ForeachDefinedBinaryCaseItem(
        [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
        {
            const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
            const std::string& signature = binary_data_accessor.GetSignature();
            ASSERT(BinaryDataAccessor::IsValidSignature(signature));

            // if this binary content originated from this repository, we do not need to sync it
            const CSWebBinaryContentReader* const csweb_binary_content_reader = dynamic_cast<const CSWebBinaryContentReader*>(binary_data_accessor.GetBinaryContentReader());

            if( csweb_binary_content_reader != nullptr &&
                csweb_binary_content_reader->GetUniqueId() == &m_repository.m_repositoryId )
            {
                return;
            }

            // check the cache to see if the binary item has already been synced;
            // because the above check ensures that binary content read from this repository is not synced,
            // this would only be true if data was loaded from the disk, or from another repository, that
            // happened to be the same as previously synced (and eventually cached) binary content
            if( m_repository.m_cache != nullptr &&
                m_repository.m_cache->HasBinaryData(signature) )
            {
                return;
            }

            signatures_to_sync.emplace_back(signature);
        });
}


std::unique_ptr<SyncBinaryDataUploadManager> CSWebRepository::CreateSyncBinaryDataUploadManager()
{
    ASSERT(!m_syncBinaryDataUploadManager.has_value());

    return GetCaseAccess().GetCaseMetadata().UsesBinaryData() ? std::make_unique<CSWebRepositorySyncBinaryDataUploadManager>(*this) :
                                                                nullptr;
}
