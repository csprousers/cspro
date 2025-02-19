#include "stdafx.h"
#include "CSWebRepository.h"
#include "CaseIterator.h"
#include "CSWebBinaryContentReader.h"
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

    throw DataRepositoryException::IOError("Error communicating with CSWeb data source '%s': %s",
                                           dictionary_name.c_str(),
                                           sync_error_formatter->GetFormattedError(exception).c_str());
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
        m_syncCaseSerializer = CreateSyncCaseSerializer(m_repositoryId, m_caseAccess, m_cswebConnection);
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
    ASSERT(m_cswebConnection == nullptr);

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

        std::unique_ptr<const ConnectResponse> csweb_connect_response = csweb_connection->Connect(CSWebVersion::V3);
        ASSERT(csweb_connection->GetUser().has_value());

        if( csweb_connection->GetUser()->IsAdmin() )
        {
            throw DataRepositoryException::IOError("You cannot use the CSWeb data source while connecting with role '%s'.",
                                                    csweb_connection->GetUser()->role_name.c_str());
        }

        const JsonNode dictionary_metadata_json_node = EnsureDictionaryExistsAndGetDictionaryMetadata(*csweb_connection, open_flag);
        const std::optional<int64_t> min_revision = dictionary_metadata_json_node.GetOptional<int64_t>(JK::minRevision);
        const std::optional<int64_t> max_revision = dictionary_metadata_json_node.GetOptional<int64_t>(JK::maxRevision);
        // CSWEB_TODO: check versus cache;
        // if min_revision is std::nullopt or is > the cache's revision, it means that the data was deleted via the CSWeb UI, so the entire cache should be cleared

        m_cswebConnection = std::move(csweb_connection);
        m_cswebConnectResponse = std::move(csweb_connect_response);

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


JsonNode CSWebRepository::EnsureDictionaryExistsAndGetDictionaryMetadata(CSWebConnection& csweb_connection, const DataRepositoryOpenFlag open_flag) const
{
    // CSWEB_TODO check that user's upload/download/truncate roles permit the options

    bool delete_existing_data = ( open_flag == DataRepositoryOpenFlag::CreateNew ||
                                  m_accessType == DataRepositoryAccess::BatchOutput );

    std::optional<JsonNode> dictionary_metadata_json_node;

    try
    {
        dictionary_metadata_json_node = csweb_connection.GetDictionaryMetadata(m_syncableDictionaryName);
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
        dictionary_metadata_json_node = csweb_connection.GetDictionaryMetadata(m_syncableDictionaryName);

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


void CSWebRepository::ToggleReadWriteMode()
{
    ASSERT(m_accessType == DataRepositoryAccess::ReadOnly || m_accessType == DataRepositoryAccess::ReadWrite);

    // CSWEB_TODO check that user's upload/download/truncate roles permit the options

    m_accessType = ( m_accessType == DataRepositoryAccess::ReadOnly ) ? DataRepositoryAccess::ReadWrite :
                                                                        DataRepositoryAccess::ReadOnly;
}


void CSWebRepository::Close()
{
    m_cswebConnection.reset();
}


void CSWebRepository::DeleteRepository()
{
    try
    {
        m_cswebConnection->DeleteDictionaryData(m_syncableDictionaryName);
    }

    catch( const std::exception& exception )
    {
        RethrowException(exception);
    }
}


std::unique_ptr<SyncCaseSerializer> CSWebRepository::CreateSyncCaseSerializer(UniqueId repository_id,
                                                                              std::shared_ptr<const CaseAccess> case_access,
                                                                              std::shared_ptr<CSWebConnection> csweb_connection)
{
    auto case_json_parser_helper = std::make_unique<CSWebCaseJsonParserHelper>(std::move(repository_id), case_access, std::move(csweb_connection));

    return std::make_unique<SyncCaseSerializer>(std::move(case_access), SyncCaseSerializer::Version::V3, std::move(case_json_parser_helper));
}


size_t CSWebRepository::ParseJsonCount(const JsonNode& json_node)
{
    return json_node.Get<size_t>(JK::count);
}


CaseKey CSWebRepository::ParseJsonIdentifier(const JsonNode& json_node)
{
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
    return CaseSummary(ParseJsonIdentifier(json_node),
                       json_node.GetOrConstruct<std::string>(JK::label),
                       json_node.GetOrDefault(JK::deleted, false),
                       json_node.GetOrDefault(JK::verified, false),
                       json_node.GetOrDefault(JK::partialSaveMode, PartialSaveMode::None),
                       json_node.GetOrConstruct<std::string>(JK::caseNote));
}


void CSWebRepository::ParseJsonCase(Case& data_case, const JsonNode& case_json_node, const JsonNode& metadata_json_node, SyncCaseSerializer& sync_case_serializer)
{
    sync_case_serializer.GetSyncCaseJsonSerializer().ParseCase(data_case, case_json_node);

    data_case.SetPositionInRepository(metadata_json_node.Get<double>(JK::position));
    ASSERT(data_case.GetPositionInRepository() >= 1);
}


size_t CSWebRepository::ExecuteCaseCountQuery(const std::string_view arguments_json_text_sv) const
{
    const JsonNode json_node = m_cswebConnection->QueryCasesRepository(m_syncableDictionaryName, arguments_json_text_sv);

    return ParseJsonCount(json_node);
}


template<bool requires_metadata/* = false*/, typename CF>
void CSWebRepository::ExecuteSingleCaseQuery(const char* const content, const char* const status,
                                             const char* const filter_type, const std::string_view filter_value_sv,
                                             const CF& callback_function) const
{
    const std::string arguments_json_text = SO::Concatenate(requires_metadata ? R"({"requestMetadata":true,"content":")" : R"({"content":")", content,
                                                            R"(","status":")", status,
                                                            R"(","filter":{"operator":"=","type":")", filter_type,
                                                            R"(","value":)", filter_value_sv,
                                                            R"(},"order":"position","limit":1})");

    const JsonNode json_node = m_cswebConnection->QueryCasesRepository(m_syncableDictionaryName, arguments_json_text);
    ASSERT(json_node.IsObject() && json_node.Contains(content) && json_node.Get(content).IsArray());

    const JsonNodeArray content_json_array_node = json_node.GetArray(content);
    ASSERT(content_json_array_node.size() <= 1);

    if( content_json_array_node.empty() )
        throw DataRepositoryException::CaseNotFound();

    if constexpr(requires_metadata)
    {
        const JsonNodeArray metadata_json_array_node = json_node.GetArray(JK::metadata);
        ASSERT(metadata_json_array_node.size() == content_json_array_node.size());

        callback_function(content_json_array_node[0], metadata_json_array_node[0]);
    }

    else
    {
        callback_function(content_json_array_node[0]);
    }
}


bool CSWebRepository::ContainsCase(const std::string& key)
{
    // CSWEB_TODO: access + update cache
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
    // CSWEB_TODO: access + update cache
    try
    {
        if( !key.empty() )
        {
            ExecuteSingleCaseQuery(JK::identifiers, JV::notDeletedOnly, JK::key, Encoders::ToJsonString(key),
                [&](const JsonNode& identifiers_json_node)
                {
                    ASSERT(key == identifiers_json_node.Get<std::string>(JK::key));
                    uuid = identifiers_json_node.Get<std::string>(JK::uuid);
                    position_in_repository = identifiers_json_node.Get<double>(JK::position);
                });
        }

        // or by UUID
        else if( !uuid.empty() )
        {
            ExecuteSingleCaseQuery(JK::identifiers, JV::all, JK::uuid, Encoders::ToJsonString(uuid),
                [&](const JsonNode& identifiers_json_node)
                {
                    key = identifiers_json_node.Get<std::string>(JK::key);
                    ASSERT(uuid == identifiers_json_node.Get<std::string>(JK::uuid));
                    position_in_repository = identifiers_json_node.Get<double>(JK::position);
                });
        }

        // or by position
        else
        {
            ASSERT(static_cast<int64_t>(position_in_repository) == position_in_repository);

            ExecuteSingleCaseQuery(JK::identifiers, JV::all, JK::position, IntToString(static_cast<int64_t>(position_in_repository)),
                [&](const JsonNode& identifiers_json_node)
                {
                    key = identifiers_json_node.Get<std::string>(JK::key);
                    uuid = identifiers_json_node.Get<std::string>(JK::uuid);
                    ASSERT(position_in_repository == identifiers_json_node.Get<double>(JK::position));
                });
        }
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
        throw;
    }

    catch( const std::exception& )
    {
        throw DataRepositoryException::GenericReadError(); // CSWEB_TODO revisit when this is a CSWeb communication error?
    }
}


DataRepositoryUniqueCaseIdentifer CSWebRepository::GetUniqueCaseIdentifer(const CaseKey& case_key)
{
    return case_key.GetPositionInRepository();
}


template<bool requires_metadata/* = false*/>
std::string CSWebRepository::CreateKeySearchQuery(const char* const content, const CaseIterationCaseStatus case_status,
                                                  const std::optional<CaseIterationMethod> iteration_method, const std::optional<CaseIterationOrder> iteration_order,
                                                  const CaseIteratorParameters* const start_parameters, const size_t offset, const size_t limit)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::content, content)
                .WriteIfNot(JK::requestMetadata, requires_metadata, false)
                .Write(JK::status, ( case_status == CaseIterationCaseStatus::All )            ? JV::all :
                                   ( case_status == CaseIterationCaseStatus::NotDeletedOnly ) ? JV::notDeletedOnly :
                                   ( case_status == CaseIterationCaseStatus::PartialsOnly )   ? JV::partialsOnly :
                                                                                                JV::duplicatesOnly);

    if( iteration_method.has_value() || iteration_order.has_value() )
    {
        json_writer->BeginObject(JK::sort);

        if( iteration_method.has_value() )
            json_writer->Write(JK::order, ( *iteration_method == CaseIterationMethod::KeyOrder ) ? JK::key : JK::position);

        if( iteration_order.has_value() )
            json_writer->Write(JK::ascending, ( *iteration_order == CaseIterationOrder::Ascending ));

        json_writer->EndObject();
    }

    if( start_parameters != nullptr )
    {
        json_writer->BeginObject(JK::filter);

        // use the key prefix if it is set and and is not empty
        if( start_parameters->key_prefix.has_value() && !start_parameters->key_prefix->empty() )
        {
            json_writer->Write(JK::operator_, "startswith")
                        .Write(JK::type, "key")
                        .Write(JK::value, *start_parameters->key_prefix);
        }

        else
        {
            json_writer->Write(JK::operator_, ToString(start_parameters->start_type))
                        .Write(JK::type, std::holds_alternative<std::string>(start_parameters->first_key_or_position) ? JK::key : JK::position)
                        .WriteVariant(JK::value, start_parameters->first_key_or_position);
        }

        json_writer->EndObject();
    }

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
    // CSWEB_TODO: access + update cache
    try
    {
        const std::string arguments_json_text = CreateKeySearchQuery(JK::identifiers,
                                                                     CaseIterationCaseStatus::NotDeletedOnly,
                                                                     iteration_method, iteration_order,
                                                                     start_parameters, 0, 1);

        const JsonNode json_node = m_cswebConnection->QueryCasesRepository(m_syncableDictionaryName, arguments_json_text);
        ASSERT(json_node.IsObject() && json_node.Contains(JK::identifiers) && json_node.Get(JK::identifiers).IsArray());

        const JsonNodeArray content_json_array_node = json_node.GetArray(JK::identifiers);
        ASSERT(content_json_array_node.size() <= 1);

        if( content_json_array_node.empty() )
            return std::nullopt;

        return ParseJsonIdentifier(content_json_array_node[0]);
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

    try
    {
        ExecuteSingleCaseQuery<true>(JK::cases, status, filter_type, filter_value_sv,
            [&](const JsonNode& case_json_node, const JsonNode& metadata_json_node)
            {
                ParseJsonCase(data_case, case_json_node, metadata_json_node, *m_syncCaseSerializer);
            });
    }

    catch( const DataRepositoryException::CaseNotFound& )
    {
        throw;
    }

    catch( const std::exception& )
    {
        throw DataRepositoryException::GenericReadError(); // CSWEB_TODO revisit when this is a CSWeb communication error?
    }

    // CSWEB_TODO: update cache
}


void CSWebRepository::ReadCase(Case& data_case, const std::string& key)
{
    // CSWEB_TODO: access cache
    ReadCase(data_case, JV::notDeletedOnly, JK::key, Encoders::ToJsonString(key));
}


void CSWebRepository::ReadCase(Case& data_case, const double position_in_repository)
{
    ASSERT(static_cast<int64_t>(position_in_repository) == position_in_repository);

    // CSWEB_TODO: access cache
    ReadCase(data_case, JV::all, JK::position, IntToString(static_cast<int64_t>(position_in_repository)));
}


void CSWebRepository::ReadCaseByUuid(Case& data_case, const std::string& uuid)
{
    // CSWEB_TODO: access cache
    ReadCase(data_case, JV::all, JK::uuid, Encoders::ToJsonString(uuid));
}


void CSWebRepository::WriteCase(Case& data_case, WriteCaseParameter* /*write_case_parameter = nullptr*/)
{
    ASSERT(m_syncCaseSerializer != nullptr);

    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    data_case.GetOrCreateUuid();
    data_case.GetVectorClock().increment(m_deviceId);

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

    catch( const std::exception& )
    {
        throw DataRepositoryException::GenericWriteError(); // CSWEB_TODO revisit when this is a CSWeb communication error?
    }

    // CSWEB_TODO: update case cache

    if( *m_syncBinaryDataUploadManager != nullptr )
    {
        (*m_syncBinaryDataUploadManager)->ForeachBinaryCaseItemInChunk(
            [&](const std::string& signature)
            {
                signature; // CSWEB_TODO: update binary data cache
            });
    }
}


void CSWebRepository::DeleteCase(const double position_in_repository, const bool deleted/* = true*/)
{
    if( IsReadOnly() )
        throw DataRepositoryException::WriteAccessRequired();

    // CSWEB_TODO: access + update cache
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
            WriteCase(*m_temporaryCase);
        }
    }

    catch( const std::exception& )
    {
        throw DataRepositoryException::GenericWriteError(); // CSWEB_TODO revisit when this is a CSWeb communication error?
    }
}


size_t CSWebRepository::GetNumberCases()
{
    // CSWEB_TODO: access + update cache
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


size_t CSWebRepository::GetNumberCases(const CaseIterationCaseStatus case_status, const CaseIteratorParameters* start_parameters/* = nullptr*/)
{
    if( case_status == CaseIterationCaseStatus::NotDeletedOnly && start_parameters == nullptr )
        return CSWebRepository::GetNumberCases();

    // CSWEB_TODO: access + update cache
    try
    {
        const std::string arguments_json_text = CreateKeySearchQuery(JK::count,
                                                                     case_status,
                                                                     std::nullopt, std::nullopt,
                                                                     start_parameters, 0, 1);

        return ExecuteCaseCountQuery(arguments_json_text);
    }

    catch( const std::exception& )
    {
        // GetNumberCases seems like it should not throw exceptions, though some of the other repositories do, so what to do? -- CSWEB_TODO revisit?
        return 0;
    }
}


std::unique_ptr<CaseIterator> CSWebRepository::CreateIterator(const CaseIterationContent iteration_content, const CaseIterationCaseStatus case_status,
                                                              const std::optional<CaseIterationMethod> iteration_method, const std::optional<CaseIterationOrder> iteration_order,
                                                              const CaseIteratorParameters* const start_parameters/* = nullptr*/, const size_t offset/* = 0*/, const size_t limit/* = SIZE_MAX*/)
{
    return std::make_unique<CSWebRepositoryIterator>(*this, iteration_content, case_status,
                                                            iteration_method, iteration_order,
                                                            start_parameters, offset, limit);
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

CSWebRepositoryIterator::CSWebRepositoryIterator(CSWebRepository& csweb_repository,
                                                 const CaseIterationContent iteration_content, const CaseIterationCaseStatus case_status,
                                                 const std::optional<CaseIterationMethod> iteration_method, const std::optional<CaseIterationOrder> iteration_order,
                                                 const CaseIteratorParameters* const start_parameters/* = nullptr*/, const size_t offset/* = 0*/, const size_t limit/* = SIZE_MAX*/)
    :   m_cswebRepository(csweb_repository),
        m_iterationContent(( iteration_content == CaseIterationContent::CaseKey )     ? JK::identifiers :
                           ( iteration_content == CaseIterationContent::CaseSummary ) ? JK::summaries :
                         /*( iteration_content == CaseIterationContent::Case )*/        JK::cases),
        m_caseStatus(case_status),
        m_iterationMethod(iteration_method),
        m_iterationOrder(iteration_order),
        m_startParameters(( start_parameters != nullptr ) ? std::make_unique<CaseIteratorParameters>(*start_parameters) : nullptr),
        m_offset(offset),
        m_limit(limit),
        m_casesRead(0)
{
}


template<bool requires_metadata>
typename std::conditional<requires_metadata, std::optional<std::tuple<JsonNode, JsonNode>>, std::optional<JsonNode>>::type CSWebRepositoryIterator::Step()
{
    // CSWEB_TODO: access + update cache

    bool query_next_set;

    // query content for the first time...
    if( !m_query.has_value() )
    {
        query_next_set = true;
    }

    // ...potentially query content again if all content was not previously received...
    else if( m_query->iterator_case_pos == m_query->case_count )
    {
        if( m_query->limit_satisfied )
            return std::nullopt;

        query_next_set = true;

        // adjust the offset for the next query
        m_offset += m_query->case_count;
    }

    // ... or process content from a previous query
    else
    {
        query_next_set = false;
    }

    if( query_next_set )
    {
        QueryNextSet(m_cswebRepository.CreateKeySearchQuery<requires_metadata>(m_iterationContent, m_caseStatus,
                                                                               m_iterationMethod, m_iterationOrder,
                                                                               m_startParameters.get(), m_offset, m_limit));
        ASSERT(m_query.has_value());
        ASSERT(requires_metadata == m_query->metadata_json_array_node.has_value());

        if( m_query->iterator_case_pos == m_query->case_count )
        {
            ASSERT(m_query->limit_satisfied);
            return std::nullopt;
        }
    }

    // process the content
    ASSERT(m_query->iterator_case_pos < m_query->case_count);

    ++m_casesRead;

    if constexpr(requires_metadata)
    {
        ASSERT(m_query->metadata_json_array_node.has_value());

        const size_t iterator_case_pos_copy = m_query->iterator_case_pos;

        return std::make_tuple(m_query->content_json_array_node[iterator_case_pos_copy],
                               (*m_query->metadata_json_array_node)[m_query->iterator_case_pos++]);
    }

    else
    {
        return m_query->content_json_array_node[m_query->iterator_case_pos++];
    }
}


void CSWebRepositoryIterator::QueryNextSet(const std::string& arguments_json_text)
{
    try
    {
        JsonNode json_node = m_cswebRepository.m_cswebConnection->QueryCasesRepository(m_cswebRepository.m_syncableDictionaryName, arguments_json_text);
        ASSERT(json_node.IsObject() && json_node.Contains(m_iterationContent) && json_node.Get(m_iterationContent).IsArray());

        JsonNodeArray content_json_array_node = json_node.GetArray(m_iterationContent);
        const size_t case_count = content_json_array_node.size();

        std::optional<JsonNodeArray> metadata_json_array_node;

        if( m_iterationContent == JK::cases )
        {
            metadata_json_array_node.emplace(json_node.GetArray(JK::metadata));
            ASSERT(metadata_json_array_node->size() == case_count);
        }

        m_query.emplace(
            Query
            {
                std::move(json_node),
                std::move(content_json_array_node),
                std::move(metadata_json_array_node),
                case_count,
                !json_node.Contains(JK::limit),
                0
            });
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
    if( m_iterationContent == JK::identifiers ||
        m_iterationContent == JK::summaries )
    {
        const std::optional<JsonNode> identifiers_json_node = Step<false>();

        if( !identifiers_json_node.has_value() )
            return false;

        case_key = CSWebRepository::ParseJsonIdentifier(*identifiers_json_node);
    }

    else
    {
        ASSERT(m_iterationContent == JK::cases);

        Case& data_case = GetTemporaryCaseObject<Case>();

        if( !NextCase(data_case) )
            return false;

        case_key = data_case;
    }

    return true;
}


bool CSWebRepositoryIterator::NextCaseSummary(CaseSummary& case_summary)
{
    if( m_iterationContent == JK::summaries )
    {
        const std::optional<JsonNode> summaries_json_node = Step<false>();

        if( !summaries_json_node.has_value() )
            return false;

        case_summary = CSWebRepository::ParseJsonSummary(*summaries_json_node);
    }

    else if( m_iterationContent == JK::cases )
    {
        Case& data_case = GetTemporaryCaseObject<Case>();

        if( !NextCase(data_case) )
            return false;

        case_summary = data_case;
    }

    else
    {
        ASSERT(m_iterationContent == JK::identifiers);

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
    if( m_iterationContent == JK::cases )
    {
        const std::optional<std::tuple<JsonNode, JsonNode>> case_and_metadata_json_nodes = Step<true>();

        if( !case_and_metadata_json_nodes.has_value() )
            return false;

        ASSERT(m_cswebRepository.m_syncCaseSerializer != nullptr);

        CSWebRepository::ParseJsonCase(data_case,
                                       std::get<0>(*case_and_metadata_json_nodes), std::get<1>(*case_and_metadata_json_nodes),
                                       *m_cswebRepository.m_syncCaseSerializer);
    }

    else
    {
        ASSERT(m_iterationContent == JK::identifiers ||
               m_iterationContent == JK::summaries);

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
            const size_t number_cases = m_cswebRepository.GetNumberCases(m_caseStatus, m_startParameters.get());
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

            // CSWEB_TODO check cache to see if the binary item has already been synced

            signatures_to_sync.emplace_back(signature);
        });
}


std::unique_ptr<SyncBinaryDataUploadManager> CSWebRepository::CreateSyncBinaryDataUploadManager()
{
    ASSERT(!m_syncBinaryDataUploadManager.has_value());

    return GetCaseAccess().GetCaseMetadata().UsesBinaryData() ? std::make_unique<CSWebRepositorySyncBinaryDataUploadManager>(*this) :
                                                                nullptr;
}
