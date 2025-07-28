#include "stdafx.h"
#include "CaseTestHelpers.h"
#include "SyncTestCredentials.h"
#include <zUtilO/BinaryContentCacher.h>
#include <zUtilO/TemporaryFile.h>
#include <zCaseO/CaseItemReference.h>
#include <zDataO/CSWebRepository.h>
#include <zDataO/CSWebRepositoryJsonKeys.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zDataO/SyncBinaryDataUploadManager.h>
#include <zNetwork/CSWebConnection.h>
#include <zNetwork/CurlHttpConnection.h>
#include <zSyncO/CaseObservable.h>
#include <zSyncO/CSWebSyncService.h>

namespace SyncUnitTest { class CSWebApiTest; }


TEST_CLASS(SyncUnitTest::CSWebApiTest)
{
public:
    CSWebApiTest();

    TEST_METHOD(TestSyncDataV2)       { TestSyncData(true, false);  }
    TEST_METHOD(TestSyncBinaryDataV2) { TestSyncData(true, true);   }
    TEST_METHOD(TestSyncDataV3)       { TestSyncData(false, false); }
    TEST_METHOD(TestSyncBinaryDataV3) { TestSyncData(false, true);  }

private:
    std::unique_ptr<CSWebSyncService> Connect(bool v2);

    void TestSyncData(bool v2, bool binary_test);

    void WriteCasesToFakeRepository(std::vector<std::shared_ptr<Case>>& cases);

    JsonNode QueryCasesEndpoint(CSWebConnection& csweb_connection, const std::string_view content_sv, const std::string_view status_sv) const;

    struct CaseEndpointTestData;
    void TestCasesEndpoints(std::shared_ptr<CSWebConnection> csweb_connection, const std::vector<std::shared_ptr<Case>>& cases) const;
    void TestCasesEndpoint_Count(CSWebConnection& csweb_connection, const CaseEndpointTestData& data) const;

    template<typename T, typename CF>
    void TestCasesEndpoint_Object(CSWebConnection& csweb_connection, const CaseEndpointTestData& data,
                                  const std::string_view content_sv, const CF& callback_function) const;

    void TestCasesEndpoint_Identifiers(CSWebConnection& csweb_connection, const CaseEndpointTestData& data) const;
    void TestCasesEndpoint_Summaries(CSWebConnection& csweb_connection, const CaseEndpointTestData& data) const;
    void TestCasesEndpoint_Cases(std::shared_ptr<CSWebConnection> csweb_connection, const CaseEndpointTestData& data) const;

private:
    std::unique_ptr<CDataDict> m_dictionary;
    std::shared_ptr<CaseAccess> m_caseAccess;
    DeviceId m_putDeviceId;
    DeviceId m_getDeviceId;
};



SyncUnitTest::CSWebApiTest::CSWebApiTest()
    :   m_dictionary(CreateTestDictionaryWithUniqueName()),
        m_caseAccess(CaseAccess::CreateAndInitializeFullCaseAccess(*m_dictionary)),
        m_putDeviceId(MakeUniqueDeviceId("CSWebApiTest-put-")),
        m_getDeviceId(MakeUniqueDeviceId("CSWebApiTest-get-"))
{
}


std::unique_ptr<CSWebSyncService> SyncUnitTest::CSWebApiTest::Connect(const bool v2)
{
    const SyncTestCredentials::Credentials credentials = SyncTestCredentials().GetCredentialsCSWeb(v2);

    auto sync_service = std::make_unique<CSWebSyncService>(std::make_unique<CurlHttpConnection>(),
                                                           credentials.sync_connection_string, credentials.username_password);

    sync_service->Connect();

    return sync_service;
}


void SyncUnitTest::CSWebApiTest::TestSyncData(const bool v2, const bool binary_test)
{
    const std::unique_ptr<CSWebSyncService> sync_service = Connect(v2);

    // upload the dictionary
    sync_service->PutDictionary(*m_dictionary);

    // create some cases
    std::vector<std::shared_ptr<Case>> put_cases;

    for( int i = 1; i <= 20; ++i )
    {
        std::vector<std::string> string_values;

        for( int j = 1; j <= i; ++j )
            string_values.emplace_back(SO::WideSubstring(u8"☀☁☂☃☄★☆☇☈☉☊☋☌☍☎☏☐☑☒☓☚☛☜☝", i - 1));

        Case& data_case = *put_cases.emplace_back(CreateCase(*m_caseAccess, CreateUuid(), i, string_values, binary_test));

        // modify some case-level attributes
        if( i % 3 == 1 )
            data_case.SetCaseLabel(FormatText("Case label for '%s' with some data: %s", data_case.GetKey().c_str(), string_values.back().c_str()));

        if( i % 5 == 4 )
            data_case.SetDeleted(true);

        if( i % 3 == 2 )
            data_case.SetVerified(true);

        if( i % 4 == 1 )
        {
            ( i % 8 == 1 ) ? SetPartialSave(*m_caseAccess, data_case, PartialSaveMode::Add, "TESTDICT_ID") :
                             SetPartialSave(*m_caseAccess, data_case, PartialSaveMode::Verify, "TEST_ITEM");
        }

        if( i % 6 == 5 )
        {
            data_case.GetNotes().emplace_back(FormatText("Case note for '%s' with some data: %s", data_case.GetKey().c_str(), string_values.front().c_str()),
                                              std::make_unique<NamedReference>(m_dictionary->GetName(), std::string()),
                                              "Operator " + CreateUuid());
        }

        // add some duplicate cases
        if( i % 10 == 3 )
        {
            data_case.SetDeleted(false);

            Case& duplicate_case = *put_cases.emplace_back(m_caseAccess->CreateCase());
            duplicate_case = data_case;
            duplicate_case.SetUuid(CreateUuid());
        }
    }

    // upload the cases
    const std::vector<Case*> put_cases_in_chunk = SpanHelpers::CreatePointersSpan(put_cases);
    const std::unique_ptr<SyncBinaryDataUploadManager> sync_binary_data_upload_manager = binary_test ? CreateSyncBinaryDataUploadManager(m_caseAccess, put_cases_in_chunk) :
                                                                                                       nullptr;

    const SyncPutResponse put_response = sync_service->PutCases(m_caseAccess, put_cases_in_chunk,
                                                                sync_binary_data_upload_manager.get(), m_putDeviceId,
                                                                std::string(), std::string());
    Assert::AreEqual(SyncPutResponse::SyncPutResult::Complete, put_response.GetResult());

    // download the cases
    SyncGetResponse get_response = sync_service->GetCases(m_caseAccess, m_getDeviceId,
                                                          std::string(), std::string(), std::string(), std::vector<std::string>());
    Assert::AreEqual(SyncGetResponse::SyncGetResult::Complete, get_response.GetResult());

    std::vector<std::shared_ptr<Case>> get_cases = ObservableToVector(*get_response.GetCases());
    Assert::AreEqual(put_cases.size(), get_cases.size());

    // because syncable cases with binary content are only sent the binary content once, we write
    // them to a fake .csdb file so all the binary items are correctly associated with each case
    if( binary_test )
        WriteCasesToFakeRepository(get_cases);

    // compare the cases
    CompareCases(put_cases_in_chunk,
                 SpanHelpers::CreatePointersSpan(get_cases),
                 CompareCasesType::CountsAndFullCase);

    // test the V3 endpoints
    if( !v2 )
        TestCasesEndpoints(sync_service->GetSharedCSWebConnection(), put_cases);

    // delete the dictionary
    sync_service->DeleteDictionary(m_dictionary->GetSyncableName());
}


void SyncUnitTest::CSWebApiTest::WriteCasesToFakeRepository(std::vector<std::shared_ptr<Case>>& cases)
{
    const TemporaryFile temporary_file = TemporaryFile::FromPath(PortableFunctions::GetUniqueFilePathInDirectory(GetTempDirectory(), FileExtensions::Data::CSProDB));

    const std::unique_ptr<DataRepository> repository = DataRepository::CreateAndOpen(m_caseAccess, ConnectionString(temporary_file.GetPath()),
                                                                                     DataRepositoryAccess::ReadWrite, DataRepositoryOpenFlag::CreateNew);

    ISyncableDataRepository* const syncable_data_repository = repository->GetSyncableDataRepository();
    Assert::IsNotNull(syncable_data_repository);

    syncable_data_repository->StartSync(m_getDeviceId, "csweb_device_name", "user", SyncDirection::Get, "", false);
    syncable_data_repository->SyncCasesFromRemote(cases, "server_revision");
    syncable_data_repository->EndSync();

    // update the cases from the repository
    for( const std::shared_ptr<Case>& data_case : cases )
    {
        repository->ReadCaseByUuid(*data_case, data_case->GetUuid());

        // load the binary data (because so that a reader doesn't try to access the binary data in a closed repository)
        data_case->LoadAllBinaryData();
    }

    repository->Close();
}


JsonNode SyncUnitTest::CSWebApiTest::QueryCasesEndpoint(CSWebConnection& csweb_connection, const std::string_view content_sv, const std::string_view status_sv) const
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::content, content_sv)
                .WriteIfNotBlank(JK::status, status_sv)
                .WriteIfNot(JK::requestMetadata, true, ( content_sv != JK::cases ))
                .EndObject();

    JsonNode json_node = csweb_connection.QueryCasesRepository(m_dictionary->GetSyncableName(), json_writer->GetString());
    Assert::IsTrue(json_node.IsObject());

    return json_node;
}


struct SyncUnitTest::CSWebApiTest::CaseEndpointTestData
{
    std::vector<std::shared_ptr<Case>> cases_all;
    std::vector<std::shared_ptr<Case>> cases_not_deleted_only;
    std::vector<std::shared_ptr<Case>> cases_partials_only;
    std::vector<std::shared_ptr<Case>> cases_duplicates_only;
};


void SyncUnitTest::CSWebApiTest::TestCasesEndpoints(const std::shared_ptr<CSWebConnection> csweb_connection, const std::vector<std::shared_ptr<Case>>& cases) const
{
    CaseEndpointTestData data { cases };

    for( const std::shared_ptr<Case>& data_case : cases )
    {
        if( data_case->GetDeleted() )
            continue;

        data.cases_not_deleted_only.emplace_back(data_case);

        if( data_case->IsPartial() )
            data.cases_partials_only.emplace_back(data_case);

        const auto& duplicate_lookup = std::find_if(cases.cbegin(), cases.cend(),
            [&](const std::shared_ptr<Case>& other_data_case)
            {
                return ( data_case.get() != other_data_case.get() &&
                         data_case->GetKey() == other_data_case->GetKey() );
            });

        if( duplicate_lookup != cases.cend() )
            data.cases_duplicates_only.emplace_back(data_case);
    }

    Assert::IsFalse(data.cases_all.empty());
    Assert::IsFalse(data.cases_not_deleted_only.empty());
    Assert::AreNotEqual(data.cases_all.size(), data.cases_not_deleted_only.size());
    Assert::IsFalse(data.cases_partials_only.empty());
    Assert::IsFalse(data.cases_duplicates_only.empty());

    TestCasesEndpoint_Count(*csweb_connection, data);
    TestCasesEndpoint_Identifiers(*csweb_connection, data);
    TestCasesEndpoint_Summaries(*csweb_connection, data);
    TestCasesEndpoint_Cases(csweb_connection, data);
}


void SyncUnitTest::CSWebApiTest::TestCasesEndpoint_Count(CSWebConnection& csweb_connection, const CaseEndpointTestData& data) const
{
    auto test = [&](const std::string_view status_sv, const std::vector<std::shared_ptr<Case>>& cases)
    {
        const JsonNode json_node = QueryCasesEndpoint(csweb_connection, JK::count, status_sv);
        Assert::AreEqual(cases.size(), CSWebRepository::ParseJsonCount(json_node));
    };

    test(std::string_view(), data.cases_all);
    test(JV::all, data.cases_all);
    test(JV::notDeletedOnly, data.cases_not_deleted_only);
    test(JV::partialsOnly, data.cases_partials_only);
    test(JV::duplicatesOnly, data.cases_duplicates_only);
}


template<typename T, typename CF>
void SyncUnitTest::CSWebApiTest::TestCasesEndpoint_Object(CSWebConnection& csweb_connection, const CaseEndpointTestData& data,
                                                          const std::string_view content_sv, const CF& callback_function) const
{
    auto test = [&](const std::string_view status_sv, std::vector<std::shared_ptr<Case>> cases)
    {
        const JsonNode json_node = QueryCasesEndpoint(csweb_connection, content_sv, status_sv);
        std::set<double> positions_in_repository;

        const JsonNodeArray content_json_array_node = json_node.GetArray(content_sv);

        const std::optional<JsonNodeArray> metadata_json_array_node = json_node.Contains(JK::metadata) ? std::make_optional(json_node.GetArray(JK::metadata)) :
                                                                                                         std::nullopt;
        size_t metadata_json_node_counter = 0;
        Assert::IsTrue(!metadata_json_array_node.has_value() || metadata_json_array_node->size() == content_json_array_node.size());

        for( const JsonNode& content_json_node : content_json_array_node )
        {
            const std::optional<JsonNode> metadata_json_node =
                metadata_json_array_node.has_value() ? std::make_optional((*metadata_json_array_node)[metadata_json_node_counter++]) :
                                                       std::nullopt;

            std::shared_ptr<T> object;
            std::vector<std::shared_ptr<Case>>::const_iterator cases_lookup;
            std::tie(object, cases_lookup) = callback_function(cases, content_json_node, metadata_json_node);

            Assert::IsFalse(cases_lookup == cases.cend());
            cases.erase(cases_lookup);

            // make sure that the position in the repository is unique
            Assert::IsTrue(object->GetPositionInRepository() >= 1);
            Assert::AreEqual(size_t(0), positions_in_repository.count(object->GetPositionInRepository()));
            positions_in_repository.insert(object->GetPositionInRepository());
        }

        Assert::IsTrue(cases.empty());
    };

    test(std::string_view(), data.cases_all);
    test(JV::all, data.cases_all);
    test(JV::notDeletedOnly, data.cases_not_deleted_only);
    test(JV::partialsOnly, data.cases_partials_only);
    test(JV::duplicatesOnly, data.cases_duplicates_only);
}


void SyncUnitTest::CSWebApiTest::TestCasesEndpoint_Identifiers(CSWebConnection& csweb_connection, const CaseEndpointTestData& data) const
{
    TestCasesEndpoint_Object<CaseKey>(csweb_connection, data, JK::identifiers,
        [&](const std::vector<std::shared_ptr<Case>>& cases, const JsonNode& content_json_node, const std::optional<JsonNode>& metadata_json_node)
        {
            Assert::IsFalse(metadata_json_node.has_value());

            auto case_key = std::make_shared<CaseKey>(CSWebRepository::ParseJsonIdentifier(content_json_node));

            return std::make_tuple(case_key, std::find_if(cases.cbegin(), cases.cend(),
                [&](const std::shared_ptr<Case>& data_case)
                {
                    return ( case_key->GetKey() == data_case->GetKey() );
                }));
        });
}


void SyncUnitTest::CSWebApiTest::TestCasesEndpoint_Summaries(CSWebConnection& csweb_connection, const CaseEndpointTestData& data) const
{
    TestCasesEndpoint_Object<CaseSummary>(csweb_connection, data, JK::summaries,
        [&](const std::vector<std::shared_ptr<Case>>& cases, const JsonNode& content_json_node, const std::optional<JsonNode>& metadata_json_node)
        {
            Assert::IsFalse(metadata_json_node.has_value());

            auto case_summary = std::make_shared<CaseSummary>(CSWebRepository::ParseJsonSummary(content_json_node));

            return std::make_tuple(case_summary, std::find_if(cases.cbegin(), cases.cend(),
                [&](const std::shared_ptr<Case>& data_case)
                {
                    return ( case_summary->GetKey() == data_case->GetKey() &&
                             case_summary->GetCaseLabel() == data_case->GetCaseLabel() &&
                             case_summary->GetDeleted() == data_case->GetDeleted() &&
                             case_summary->GetVerified() == data_case->GetVerified() &&
                             case_summary->GetPartialSaveMode() == data_case->GetPartialSaveMode() &&
                             case_summary->GetCaseNote() == data_case->GetCaseNote() );
                }));
        });
}


void SyncUnitTest::CSWebApiTest::TestCasesEndpoint_Cases(std::shared_ptr<CSWebConnection> csweb_connection, const CaseEndpointTestData& data) const
{
    const UniqueId repository_id;
    const std::unique_ptr<SyncCaseSerializer> sync_case_serializer = CSWebRepository::CreateSyncCaseSerializer(repository_id, m_caseAccess, csweb_connection);

    // clear the cache to force the test to retrieve the binary data from CSWeb
    BinaryContentCacher::ClearCache();

    TestCasesEndpoint_Object<Case>(*csweb_connection, data, JK::cases,
        [&](const std::vector<std::shared_ptr<Case>>& cases, const JsonNode& content_json_node, const std::optional<JsonNode>& metadata_json_node)
        {
            Assert::IsTrue(metadata_json_node.has_value());

            const std::shared_ptr<Case> this_data_case = m_caseAccess->CreateCase();
            CSWebRepository::ParseJsonCase(*this_data_case, content_json_node, *metadata_json_node,
                                           sync_case_serializer->GetSyncCaseJsonSerializer());

            return std::make_tuple(this_data_case, std::find_if(cases.cbegin(), cases.cend(),
                [&](const std::shared_ptr<Case>& data_case)
                {
                    return this_data_case->Equals(*data_case, false);
                }));
        });
}
