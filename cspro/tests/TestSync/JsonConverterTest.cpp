#include "stdafx.h"
#include "CaseTestHelpers.h"
#include "CppUnitTest.h"
#include <zToolsO/ApiKeys.h>
#include <zToolsO/Utf8Convert.h>
#include <zAppO/SyncTypes.h>
#include <zDictO/DDClass.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseItemReference.h>
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zNetwork/FileInfo.h>
#include <zSyncO/ConnectResponse.h>
#include <zSyncO/JsonConverter.h>
#include <zSyncO/OauthTokenRequest.h>
#include <zSyncO/OauthTokenResponse.h>
#include <zSyncO/SyncRequest.h>
#include <external/jsoncons/json.hpp>
#include <string>


namespace Microsoft {
    namespace VisualStudio {
        namespace CppUnitTestFramework
        {
            // ToString specialization is required for all types used in the AssertTrue macro
            template<> inline std::wstring ToString<CString>(const CString& t) { return std::wstring(t); }

        }
    }
}

using namespace Microsoft::VisualStudio::CppUnitTestFramework;


namespace SyncUnitTest
{
    TEST_CLASS(JsonConverterTest)
    {
    public:
        TEST_METHOD(TestCaseListFromJson)
        {
            JsonConverter converter;
            std::string json("[{\"id\":\"guid1\", \"caseids\":\"1\", \"data\":[\"11data1line1\",\"11data1line2\"],\"notes\":[{\"content\":\"This is note one\",\"field\":{\"name\":\"TEST_ITEM\",\"levelKey\":\"\",\"recordOccurrence\":2,\"itemOccurrence\":1,\"subitemOccurrence\":0},\"operatorId\":\"op1\",\"modifiedTime\":\"2016-10-19T22:07:23Z\"},{\"content\":\"This is note two\",\"field\":{\"name\":\"TESTDICT_ID\"},\"operatorId\":\"op1\",\"modifiedTime\":\"2000-05-05T10:15:00Z\"}],\"clock\":[{\"deviceId\":\"dev1\",\"revision\":1},{\"deviceId\":\"dev2\",\"revision\":2}]},{\"id\":\"guid2\",\"caseids\":\"2\",\"data\":[\"12data2\"],\"deleted\":true,\"verified\":true,\"partialSave\":{\"mode\":\"add\",\"field\":{\"name\":\"TEST_ITEM\",\"levelKey\":\"\",\"recordOccurrence\":1,\"itemOccurrence\":1,\"subitemOccurrence\":0}},\"clock\":[{\"deviceId\":\"dev1\",\"revision\":1},{\"deviceId\":\"dev2\",\"revision\":2}]}]");

            std::unique_ptr<const CDataDict> dict = createTestDict();
            auto case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dict);
            auto case_vector = converter.caseListFromJson(json, *case_access);
            Assert::AreEqual((size_t) 2, case_vector.size());

            Assert::AreEqual(L"guid1", case_vector[0]->GetUuid());
            auto& idRecord0 = case_vector[0]->GetRootCaseLevel().GetIdCaseRecord();
            Assert::AreEqual(1.0, static_cast<const NumericCaseItem*>(idRecord0.GetCaseItems().front())->GetValue(idRecord0.GetCaseItemIndex()));
            auto& dataRecord0 = case_vector[0]->GetRootCaseLevel().GetCaseRecord(0);
            Assert::AreEqual((size_t) 2, dataRecord0.GetNumberOccurrences());
            Assert::AreEqual(L"data1line1                    ", static_cast<const StringCaseItem*>(dataRecord0.GetCaseItems().front())->GetValue(dataRecord0.GetCaseItemIndex(0)));
            Assert::AreEqual(L"data1line2                    ", static_cast<const StringCaseItem*>(dataRecord0.GetCaseItems().front())->GetValue(dataRecord0.GetCaseItemIndex(1)));
            Assert::AreEqual(1, case_vector[0]->GetVectorClock().getVersion("dev1"));
            Assert::AreEqual(2, case_vector[0]->GetVectorClock().getVersion("dev2"));
            Assert::AreEqual((size_t) 2, case_vector[0]->GetNotes().size());
            Assert::AreEqual(L"This is note one", case_vector[0]->GetNotes()[0].GetContent());
            Assert::AreEqual(L"TEST_ITEM", case_vector[0]->GetNotes()[0].GetNamedReference().GetName());
            Assert::AreEqual(L"", case_vector[0]->GetNotes()[0].GetNamedReference().GetLevelKey());
            Assert::AreEqual((size_t) 2, case_vector[0]->GetNotes()[0].GetNamedReference().GetOneBasedOccurrences()[0]);
            Assert::AreEqual((size_t) 1, case_vector[0]->GetNotes()[0].GetNamedReference().GetOneBasedOccurrences()[1]);
            Assert::AreEqual((size_t) 0, case_vector[0]->GetNotes()[0].GetNamedReference().GetOneBasedOccurrences()[2]);
            Assert::AreEqual(L"op1", case_vector[0]->GetNotes()[0].GetOperatorId());
            Assert::AreEqual((int64_t) 1476914843, (int64_t)case_vector[0]->GetNotes()[0].GetModifiedDateTime());
            Assert::AreEqual(L"This is note two", case_vector[0]->GetNotes()[1].GetContent());
            Assert::AreEqual(L"TESTDICT_ID", case_vector[0]->GetNotes()[1].GetNamedReference().GetName());
            Assert::AreEqual(L"", case_vector[0]->GetNotes()[1].GetNamedReference().GetLevelKey());
            Assert::AreEqual((size_t)1, case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[0]);
            Assert::AreEqual((size_t)1, case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[1]);
            Assert::AreEqual((size_t)0, case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[2]);
            Assert::AreEqual((int64_t) 957521700, (int64_t)case_vector[0]->GetNotes()[1].GetModifiedDateTime());
            Assert::AreEqual(L"op1", case_vector[0]->GetNotes()[1].GetOperatorId());
            Assert::AreEqual(false, case_vector[0]->GetVerified());
            Assert::IsTrue(case_vector[0]->GetPartialSaveCaseItemReference() == nullptr);

            Assert::AreEqual(L"guid2", case_vector[1]->GetUuid());
            auto& idRecord1 = case_vector[1]->GetRootCaseLevel().GetIdCaseRecord();
            Assert::AreEqual(2.0, static_cast<const NumericCaseItem*>(idRecord1.GetCaseItems().front())->GetValue(idRecord1.GetCaseItemIndex()));
            auto& dataRecord1 = case_vector[1]->GetRootCaseLevel().GetCaseRecord(0);
            Assert::AreEqual((size_t)1, dataRecord1.GetNumberOccurrences());
            Assert::AreEqual(L"data2                         ", static_cast<const StringCaseItem*>(dataRecord1.GetCaseItems().front())->GetValue(dataRecord1.GetCaseItemIndex(0)));
            Assert::AreEqual(1, case_vector[1]->GetVectorClock().getVersion("dev1"));
            Assert::AreEqual(2, case_vector[1]->GetVectorClock().getVersion("dev2"));
            Assert::IsTrue(case_vector[1]->GetNotes().empty());
            Assert::AreEqual(true, case_vector[1]->GetDeleted());
            Assert::AreEqual(true, case_vector[1]->GetVerified());
            Assert::IsTrue(case_vector[1]->GetPartialSaveCaseItemReference() != nullptr);
            Assert::AreEqual(L"TEST_ITEM", case_vector[1]->GetPartialSaveCaseItemReference()->GetName());
            Assert::AreEqual((size_t)1, case_vector[1]->GetPartialSaveCaseItemReference()->GetOneBasedOccurrences()[0]);
            Assert::AreEqual((size_t)1, case_vector[1]->GetPartialSaveCaseItemReference()->GetOneBasedOccurrences()[1]);
            Assert::AreEqual((size_t)0, case_vector[1]->GetPartialSaveCaseItemReference()->GetOneBasedOccurrences()[2]);
            Assert::IsTrue(PartialSaveMode::Add == case_vector[1]->GetPartialSaveMode());
        }

        TEST_METHOD(TestCaseListToJson)
        {
            std::unique_ptr<const CDataDict> dict = createTestDict();
            auto case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dict);

            VectorClock clock;
            clock.increment(L"devid");

            std::vector<std::shared_ptr<Case>> cases;
            cases.push_back(makeCase(*case_access, L"guid1", 1, { L"somedata" }, false));
            cases[0]->SetVectorClock(clock);
            cases[0]->SetCaseLabel("label1");
            addItemNote(*case_access, *cases[0], L"Note 1", "TEST_ITEM", 0, 0, 0, L"opid", 123456789L);
            addItemNote(*case_access, *cases[0], L"Note 2", "TESTDICT_ID", 0, 0, 0, L"opid", 123456789L);

            cases.push_back(makeCase(*case_access, L"guid2", 1, { L"moredata" }, false));
            cases[1]->SetVectorClock(clock);
            cases[1]->SetCaseLabel("label2");
            setPartialSave(*case_access, *cases[1], PartialSaveMode::Add, "TEST_ITEM", 0, 0, 0);

            JsonConverter converter;
            auto result74 = converter.toJson(cases, false);
            std::string expectedJson74(R"([{"id":"guid1","label":"label1","caseids":"1","notes":[{"content":"Note 1","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"opid","modifiedTime":"1973-11-29T21:33:09Z"},{"content":"Note 2","field":{"name":"TESTDICT_ID","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"opid","modifiedTime":"1973-11-29T21:33:09Z"}],"clock":[{"deviceId":"devid","revision":1}],"level-1":{"id":{"TESTDICT_ID":1},"TESTDICT_REC":[{"TEST_ITEM":"somedata"}]}},{"id":"guid2","label":"label2","caseids":"1","clock":[{"deviceId":"devid","revision":1}],"partialSave":{"mode":"add","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0}},"level-1":{"id":{"TESTDICT_ID":1},"TESTDICT_REC":[{"TEST_ITEM":"moredata"}]}}])");
            Assert::AreEqual(expectedJson74, result74);

            auto result = converter.toJson(cases, true);
            std::string expectedJson(R"([{"id":"guid1","label":"label1","caseids":"1","deleted":false,"verified":false,"notes":[{"content":"Note 1","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"opid","modifiedTime":"1973-11-29T21:33:09Z"},{"content":"Note 2","field":{"name":"TESTDICT_ID","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"opid","modifiedTime":"1973-11-29T21:33:09Z"}],"clock":[{"deviceId":"devid","revision":1}],"data":["11somedata"]},{"id":"guid2","label":"label2","caseids":"1","deleted":false,"verified":false,"notes":[],"clock":[{"deviceId":"devid","revision":1}],"partialSave":{"mode":"add","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0}},"data":["11moredata"]}])");
            Assert::AreEqual(expectedJson, result);
        }

        TEST_METHOD(TestConnectResponseFromJson)
        {
            JsonConverter converter;
            std::string json("{\"deviceId\":\"myServer\"}");
            std::unique_ptr<ConnectResponse> response(converter.connectResponseFromJson(json));
            Assert::AreEqual(L"myServer", response->getServerDeviceId());
        }

        TEST_METHOD(TestTokenRequestToJson)
        {
            OauthTokenRequest request = OauthTokenRequest::CreatePasswordRequest(CSWebKeys::client_id, CSWebKeys::client_secret, L"savy", L"savypwd");
            JsonConverter converter;
            std::string json = converter.toJson(request);
            std::string expectedJson = UTF8Convert::WideToUTF8(FormatText(L"{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"password\",\"username\":\"savy\",\"password\":\"savypwd\"}", CSWebKeys::client_id, CSWebKeys::client_secret));
            Assert::AreEqual(expectedJson, json);
            request = OauthTokenRequest::CreateRefreshRequest(CSWebKeys::client_id, CSWebKeys::client_secret, L"myrefreshtoken");
            json = converter.toJson(request);
            expectedJson = UTF8Convert::WideToUTF8(FormatText(L"{\"client_id\":\"%s\",\"client_secret\":\"%s\",\"grant_type\":\"refresh_token\",\"refresh_token\":\"myrefreshtoken\"}", CSWebKeys::client_id, CSWebKeys::client_secret));
            Assert::AreEqual(expectedJson, json);
        }

        TEST_METHOD(TestTokenResponseFromJson)
        {
            JsonConverter converter;
            std::string json("{\"access_token\":\"45111c188ca0b183d8af772e1f1834add10cfcfb\",\"expires_in\":3600,\"token_type\":\"Bearer\",\"scope\":null,\"refresh_token\":\"5d66c0c358bd5f5c75ffb82b4c2d2767c7feaad1\"}");
            std::unique_ptr<OauthTokenResponse> response(converter.oauthTokenResponseFromJson(json));
            Assert::AreEqual(L"45111c188ca0b183d8af772e1f1834add10cfcfb", response->getAccessToken());
            Assert::AreEqual(3600, response->getExpiresIn());
            Assert::AreEqual(L"", response->getScope());
            Assert::AreEqual(L"5d66c0c358bd5f5c75ffb82b4c2d2767c7feaad1", response->getRefreshToken());
        }

        TEST_METHOD(TestFileInfoFromJson)
        {
            JsonConverter converter;
            std::string json("[{\"type\":\"directory\",\"name\":\"baz\",\"directory\":\"\\/foo\\/bar\\/\"},{\"type\":\"file\",\"name\":\"test.jpg\",\"directory\":\"\\/foo\\/bar\\/\",\"md5\":\"bbc362fa59b408847b2a96d3daaff041\",\"size\":3773888},{\"type\":\"file\",\"name\":\"test.txt\",\"directory\":\"\\/foo\\/bar\\/\",\"md5\":\"372c8f51bfb74e940268972168bf1490\",\"size\":36}]");
            std::unique_ptr< std::vector<FileInfo> > response(converter.fileInfoFromJson(json));
            Assert::AreEqual((size_t) 3, response->size());
            Assert::AreEqual((int) FileInfo::FileType::Directory, (int) response->at(0).getType());
            Assert::AreEqual(L"baz", response->at(0).getName());
            Assert::AreEqual(L"/foo/bar/", response->at(0).getDirectory());
            Assert::AreEqual((int) FileInfo::FileType::File, (int) response->at(1).getType());
            Assert::AreEqual(L"test.jpg", response->at(1).getName());
            Assert::AreEqual(L"/foo/bar/", response->at(1).getDirectory());
            Assert::AreEqual(L"bbc362fa59b408847b2a96d3daaff041", response->at(1).getMd5().c_str());
            Assert::AreEqual((int64_t) 3773888, response->at(1).getSize());
            Assert::AreEqual((int) FileInfo::FileType::File, (int) response->at(2).getType());
            Assert::AreEqual(L"test.txt", response->at(2).getName());
            Assert::AreEqual(L"/foo/bar/", response->at(2).getDirectory());
            Assert::AreEqual(L"372c8f51bfb74e940268972168bf1490", response->at(2).getMd5().c_str());
            Assert::AreEqual((int64_t) 36, response->at(2).getSize());
        }

        TEST_METHOD(TestApplicationPackageFromJson)
        {
            JsonConverter converter;
            std::string json("{\"name\":\"package name\",\"description\":\"package description\",\"buildTime\":\"2017-12-19T23:58:37Z\"}");
            ApplicationPackage package = converter.applicationPackageFromJson(json);
            Assert::AreEqual(L"package name", package.getName());
            Assert::AreEqual(L"package description", package.getDescription());
            Assert::AreEqual((time_t)1513727917, package.getBuildTime());
        }

        TEST_METHOD(TestApplicationPackageListFromJson)
        {
            JsonConverter converter;
            std::string json("[{\"name\":\"package name\",\"description\":\"package description\",\"buildTime\":\"2017-12-19T23:58:37Z\"},{\"name\":\"package name2\",\"description\":\"package description2\",\"buildTime\":\"2018-01-01T00:00:00Z\"}]");
            std::vector<ApplicationPackage> packages = converter.applicationPackageListFromJson(json);
            Assert::AreEqual((size_t)2, packages.size());
            Assert::AreEqual(L"package name", packages[0].getName());
            Assert::AreEqual(L"package description", packages[0].getDescription());
            Assert::AreEqual((time_t)1513727917, packages[0].getBuildTime());
            Assert::AreEqual(L"package name2", packages[1].getName());
            Assert::AreEqual(L"package description2", packages[1].getDescription());
            Assert::AreEqual((time_t)1514764800, packages[1].getBuildTime());
        }

    };
}
