#include "stdafx.h"
#include "CaseTestHelpers.h"
#include <zToolsO/ApiKeys.h>
#include <zAppO/SyncTypes.h>
#include <zDictO/DDClass.h>
#include <zCaseO/CaseItemReference.h>
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zDataO/SyncCaseJsonSerializer.h>
#include <zNetwork/FileInfo.h>
#include <zNetwork/OAuth2Token.h>
#include <zSyncO/JsonConverter.h>


namespace SyncUnitTest
{
    TEST_CLASS(JsonConverterTest)
    {
    public:
        TEST_METHOD(TestCaseListFromJson)
        {
            const std::string json = "[{\"id\":\"guid1\", \"caseids\":\"  1\", \"data\":[\"1  1data1line1\",\"1  1data1line2\"],\"notes\":[{\"content\":\"This is note one\",\"field\":{\"name\":\"TEST_ITEM\",\"levelKey\":\"\",\"recordOccurrence\":2,\"itemOccurrence\":1,\"subitemOccurrence\":0},\"operatorId\":\"op1\",\"modifiedTime\":\"2016-10-19T22:07:23Z\"},{\"content\":\"This is note two\",\"field\":{\"name\":\"TESTDICT_ID\"},\"operatorId\":\"op1\",\"modifiedTime\":\"2000-05-05T10:15:00Z\"}],\"clock\":[{\"deviceId\":\"dev1\",\"revision\":1},{\"deviceId\":\"dev2\",\"revision\":2}]},{\"id\":\"guid2\",\"caseids\":\"  2\",\"data\":[\"1  2data2\"],\"deleted\":true,\"verified\":true,\"partialSave\":{\"mode\":\"add\",\"field\":{\"name\":\"TEST_ITEM\",\"levelKey\":\"\",\"recordOccurrence\":1,\"itemOccurrence\":1,\"subitemOccurrence\":0}},\"clock\":[{\"deviceId\":\"dev1\",\"revision\":1},{\"deviceId\":\"dev2\",\"revision\":2}]}]";

            std::unique_ptr<const CDataDict> dictionary = CreateTestDictionary();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

            SyncCaseSerializer sync_case_serializer(case_access, SyncCaseSerializer::Version::V1);

            const std::vector<std::shared_ptr<Case>> case_vector = sync_case_serializer.ParseSyncableCaseData(json);
            Assert::AreEqual(size_t(2), case_vector.size());

            Assert::AreEqual("guid1", case_vector[0]->GetUuid().c_str());
            auto& idRecord0 = case_vector[0]->GetRootCaseLevel().GetIdCaseRecord();
            Assert::AreEqual(1.0, static_cast<const NumericCaseItem*>(idRecord0.GetCaseItems().front())->GetValue(idRecord0.GetCaseItemIndex()));
            auto& dataRecord0 = case_vector[0]->GetRootCaseLevel().GetCaseRecord(0);
            Assert::AreEqual(size_t(2), dataRecord0.GetNumberOccurrences());
            Assert::AreEqual("data1line1                    ", static_cast<const StringCaseItem*>(dataRecord0.GetCaseItems().front())->GetValue(dataRecord0.GetCaseItemIndex(0)).c_str());
            Assert::AreEqual("data1line2                    ", static_cast<const StringCaseItem*>(dataRecord0.GetCaseItems().front())->GetValue(dataRecord0.GetCaseItemIndex(1)).c_str());
            Assert::AreEqual(1, case_vector[0]->GetVectorClock().getVersion("dev1"));
            Assert::AreEqual(2, case_vector[0]->GetVectorClock().getVersion("dev2"));
            Assert::AreEqual(size_t(2), case_vector[0]->GetNotes().size());
            Assert::AreEqual("This is note one", case_vector[0]->GetNotes()[0].GetContent().c_str());
            Assert::AreEqual("TEST_ITEM", case_vector[0]->GetNotes()[0].GetNamedReference().GetName().c_str());
            Assert::AreEqual("", case_vector[0]->GetNotes()[0].GetNamedReference().GetLevelKey().c_str());
            Assert::AreEqual(size_t(2), case_vector[0]->GetNotes()[0].GetNamedReference().GetOneBasedOccurrences()[0]);
            Assert::AreEqual(size_t(1), case_vector[0]->GetNotes()[0].GetNamedReference().GetOneBasedOccurrences()[1]);
            Assert::AreEqual(size_t(0), case_vector[0]->GetNotes()[0].GetNamedReference().GetOneBasedOccurrences()[2]);
            Assert::AreEqual("op1", case_vector[0]->GetNotes()[0].GetOperatorId().c_str());
            Assert::AreEqual((int64_t) 1476914843, (int64_t)case_vector[0]->GetNotes()[0].GetModifiedDateTime());
            Assert::AreEqual("This is note two", case_vector[0]->GetNotes()[1].GetContent().c_str());
            Assert::AreEqual("TESTDICT_ID", case_vector[0]->GetNotes()[1].GetNamedReference().GetName().c_str());
            Assert::AreEqual("", case_vector[0]->GetNotes()[1].GetNamedReference().GetLevelKey().c_str());
            Assert::AreEqual(size_t(1), case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[0]);
            Assert::AreEqual(size_t(1), case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[1]);
            Assert::AreEqual((size_t)0, case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[2]);
            Assert::AreEqual(int64_t(957521700), case_vector[0]->GetNotes()[1].GetModifiedDateTime());
            Assert::AreEqual("op1", case_vector[0]->GetNotes()[1].GetOperatorId().c_str());
            Assert::AreEqual(false, case_vector[0]->GetVerified());
            Assert::IsTrue(case_vector[0]->GetPartialSaveCaseItemReference() == nullptr);

            Assert::AreEqual("guid2", case_vector[1]->GetUuid().c_str());
            CaseRecord& idRecord1 = case_vector[1]->GetRootCaseLevel().GetIdCaseRecord();
            Assert::AreEqual(2.0, static_cast<const NumericCaseItem*>(idRecord1.GetCaseItems().front())->GetValue(idRecord1.GetCaseItemIndex()));
            CaseRecord& dataRecord1 = case_vector[1]->GetRootCaseLevel().GetCaseRecord(0);
            Assert::AreEqual(size_t(1), dataRecord1.GetNumberOccurrences());
            Assert::AreEqual("data2                         ", static_cast<const StringCaseItem*>(dataRecord1.GetCaseItems().front())->GetValue(dataRecord1.GetCaseItemIndex(0)).c_str());
            Assert::AreEqual(1, case_vector[1]->GetVectorClock().getVersion("dev1"));
            Assert::AreEqual(2, case_vector[1]->GetVectorClock().getVersion("dev2"));
            Assert::IsTrue(case_vector[1]->GetNotes().empty());
            Assert::AreEqual(true, case_vector[1]->GetDeleted());
            Assert::AreEqual(true, case_vector[1]->GetVerified());
            Assert::IsTrue(case_vector[1]->GetPartialSaveCaseItemReference() != nullptr);
            Assert::AreEqual("TEST_ITEM", case_vector[1]->GetPartialSaveCaseItemReference()->GetName().c_str());
            Assert::AreEqual(size_t(1), case_vector[1]->GetPartialSaveCaseItemReference()->GetOneBasedOccurrences()[0]);
            Assert::AreEqual(size_t(1), case_vector[1]->GetPartialSaveCaseItemReference()->GetOneBasedOccurrences()[1]);
            Assert::AreEqual(size_t(0), case_vector[1]->GetPartialSaveCaseItemReference()->GetOneBasedOccurrences()[2]);
            Assert::IsTrue(PartialSaveMode::Add == case_vector[1]->GetPartialSaveMode());
        }


        TEST_METHOD(TestCaseListToJson)
        {
            std::unique_ptr<const CDataDict> dictionary = CreateTestDictionary();
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

            VectorClock clock;
            clock.increment("devid");

            std::vector<std::shared_ptr<Case>> cases;
            cases.emplace_back(CreateCase(*case_access, "guid1", 1, { "somedata" }, false));
            cases[0]->SetVectorClock(clock);
            cases[0]->SetCaseLabel("label1");
            addItemNote(*case_access, *cases[0], "Note 1", "TEST_ITEM", 0, 0, 0, "opid", 123456789L);
            addItemNote(*case_access, *cases[0], "Note 2", "TESTDICT_ID", 0, 0, 0, "opid", 123456789L);

            cases.emplace_back(CreateCase(*case_access, "guid2", 1, { "moredata" }, false));
            cases[1]->SetVectorClock(clock);
            cases[1]->SetCaseLabel("label2");
            SetPartialSave(*case_access, *cases[1], PartialSaveMode::Add, "TEST_ITEM", 0, 0, 0);

            SyncCaseSerializer sync_case_serializer_v2(case_access, SyncCaseSerializer::Version::V2);
            assert_cast<SyncCaseV2JsonSerializer&>(sync_case_serializer_v2.GetSyncCaseJsonSerializer()).GetSyncCaseV2JsonWriter().SetStringifyLevelData(false);
            const std::string result74 = sync_case_serializer_v2.GetSyncableJson(SpanHelpers::CreatePointersSpan(cases));
            std::string expectedJson74(R"([{"id":"guid1","label":"label1","caseids":"  1","notes":[{"content":"Note 1","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"opid","modifiedTime":"1973-11-29T21:33:09Z"},{"content":"Note 2","field":{"name":"TESTDICT_ID","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"opid","modifiedTime":"1973-11-29T21:33:09Z"}],"clock":[{"deviceId":"devid","revision":1}],"level-1":{"id":{"TESTDICT_ID":1},"TESTDICT_REC":[{"TEST_ITEM":"somedata"}]}},{"id":"guid2","label":"label2","caseids":"  1","clock":[{"deviceId":"devid","revision":1}],"partialSave":{"mode":"add","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0}},"level-1":{"id":{"TESTDICT_ID":1},"TESTDICT_REC":[{"TEST_ITEM":"moredata"}]}}])");
            Assert::AreEqual(expectedJson74, result74);

            SyncCaseSerializer sync_case_serializer_v1(case_access, SyncCaseSerializer::Version::V1);
            const std::string result = sync_case_serializer_v1.GetSyncableJson(SpanHelpers::CreatePointersSpan(cases));
            std::string expectedJson(R"([{"id":"guid1","label":"label1","caseids":"  1","deleted":false,"verified":false,"notes":[{"content":"Note 1","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"opid","modifiedTime":"1973-11-29T21:33:09Z"},{"content":"Note 2","field":{"name":"TESTDICT_ID","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"opid","modifiedTime":"1973-11-29T21:33:09Z"}],"clock":[{"deviceId":"devid","revision":1}],"data":["1  1somedata"]},{"id":"guid2","label":"label2","caseids":"  1","deleted":false,"verified":false,"notes":[],"clock":[{"deviceId":"devid","revision":1}],"partialSave":{"mode":"add","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0}},"data":["1  1moredata"]}])");
            Assert::AreEqual(expectedJson, result);
        }

        TEST_METHOD(TestConnectResponseFromJson)
        {
            std::string json("{\"deviceId\":\"myServer\"}");
            ConnectResponse response = Json::FromJson<ConnectResponse>(json);
            Assert::AreEqual("myServer", response.GetServerDeviceId().c_str());
        }


        TEST_METHOD(TestTokenRequestToJson)
        {
            OAuth2TokenRequest request = OAuth2TokenRequest::CreatePasswordRequest(CSWebKeys::client_id, CSWebKeys::client_secret, "savy", "savypwd");
            std::string json = Json::ToJson(request, JsonFormattingOptions::Compact);
            std::string expectedJson = FormatText("{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"password\",\"username\":\"savy\",\"password\":\"savypwd\"}",
                                                  CSWebKeys::client_id, CSWebKeys::client_secret);
            Assert::AreEqual(expectedJson, json);
            request = OAuth2TokenRequest::CreateRefreshRequest(CSWebKeys::client_id, CSWebKeys::client_secret, "myrefreshtoken");
            json = Json::ToJson(request, JsonFormattingOptions::Compact);
            expectedJson = FormatText("{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"refresh_token\",\"refresh_token\":\"myrefreshtoken\"}",
                                      CSWebKeys::client_id, CSWebKeys::client_secret);
            Assert::AreEqual(expectedJson, json);
        }


        TEST_METHOD(TestTokenResponseFromJson)
        {
            std::string json("{\"access_token\":\"45111c188ca0b183d8af772e1f1834add10cfcfb\",\"expires_in\":3600,\"token_type\":\"Bearer\",\"scope\":null,\"refresh_token\":\"5d66c0c358bd5f5c75ffb82b4c2d2767c7feaad1\"}");
            OAuth2Token response = Json::FromJson<OAuth2Token>(json);
            Assert::AreEqual("45111c188ca0b183d8af772e1f1834add10cfcfb", response.GetAccessToken().c_str());
            Assert::AreEqual(3600, response.GetExpiresIn());
            Assert::AreEqual("", response.GetScope().c_str());
            Assert::AreEqual("5d66c0c358bd5f5c75ffb82b4c2d2767c7feaad1", response.GetRefreshToken().c_str());
        }


        TEST_METHOD(TestFileInfoFromJson)
        {
            const std::string json("[{\"type\":\"directory\",\"name\":\"baz\",\"directory\":\"\\/foo\\/bar\\/\"},{\"type\":\"file\",\"name\":\"test.jpg\",\"directory\":\"\\/foo\\/bar\\/\",\"md5\":\"bbc362fa59b408847b2a96d3daaff041\",\"size\":3773888},{\"type\":\"file\",\"name\":\"test.txt\",\"directory\":\"\\/foo\\/bar\\/\",\"md5\":\"372c8f51bfb74e940268972168bf1490\",\"size\":36}]");
            const std::vector<FileInfo> response = Json::Parse(json).GetArray().GetVector<FileInfo>();
            Assert::AreEqual((size_t) 3, response.size());
            Assert::AreEqual((int) FileInfo::FileType::Directory, (int) response.at(0).GetType());
            Assert::AreEqual("baz", response.at(0).GetName().c_str());
            Assert::AreEqual("/foo/bar/", response.at(0).GetDirectory().c_str());
            Assert::AreEqual((int) FileInfo::FileType::File, (int) response.at(1).GetType());
            Assert::AreEqual("test.jpg", response.at(1).GetName().c_str());
            Assert::AreEqual("/foo/bar/", response.at(1).GetDirectory().c_str());
            Assert::AreEqual("bbc362fa59b408847b2a96d3daaff041", response.at(1).GetMd5().c_str());
            Assert::AreEqual((int64_t) 3773888, response.at(1).GetSize());
            Assert::AreEqual((int) FileInfo::FileType::File, (int) response.at(2).GetType());
            Assert::AreEqual("test.txt", response.at(2).GetName().c_str());
            Assert::AreEqual("/foo/bar/", response.at(2).GetDirectory().c_str());
            Assert::AreEqual("372c8f51bfb74e940268972168bf1490", response.at(2).GetMd5().c_str());
            Assert::AreEqual((int64_t) 36, response.at(2).GetSize());
        }


        TEST_METHOD(TestApplicationPackageFromJson)
        {
            const std::string json("{\"name\":\"package name\",\"description\":\"package description\",\"buildTime\":\"2017-12-19T23:58:37Z\"}");
            const ApplicationPackage package = JsonConverter::CreateApplicationPackageFromJson(Json::Parse(json));
            Assert::AreEqual("package name", package.GetName().c_str());
            Assert::AreEqual("package description", package.GetDescription().c_str());
            Assert::AreEqual(int64_t(1513727917), package.GetBuildTime());
        }


        TEST_METHOD(TestApplicationPackageListFromJson)
        {
            const std::string json("[{\"name\":\"package name\",\"description\":\"package description\",\"buildTime\":\"2017-12-19T23:58:37Z\"},{\"name\":\"package name2\",\"description\":\"package description2\",\"buildTime\":\"2018-01-01T00:00:00Z\"}]");
            const std::vector<ApplicationPackage> packages = JsonConverter::CreateApplicationPackageListFromJson(Json::Parse(json));
            Assert::AreEqual(size_t(2), packages.size());
            Assert::AreEqual("package name", packages[0].GetName().c_str());
            Assert::AreEqual("package description", packages[0].GetDescription().c_str());
            Assert::AreEqual(int64_t(1513727917), packages[0].GetBuildTime());
            Assert::AreEqual("package name2", packages[1].GetName().c_str());
            Assert::AreEqual("package description2", packages[1].GetDescription().c_str());
            Assert::AreEqual(int64_t(1514764800), packages[1].GetBuildTime());
        }
    };
}
