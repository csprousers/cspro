#include "stdafx.h"
#include "CaseTestHelpers.h"
#include <zToolsO/VectorHelpers.h>
#include <zUtilO/ConnectionString.h>
#include <zCaseO/CaseItemReference.h>
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zDictO/DDClass.h>
#include <zDataO/DictionarySource.h>
#include <zDataO/SyncCaseIncrementalParser.h>
#include <zDataO/SyncCaseJsonSerializer.h>


namespace SyncUnitTest
{
    TEST_CLASS(JsonStreamParserTest)
    {
    private:
        static std::vector<std::shared_ptr<Case>> ParseIncrementally(const std::string_view json_sv, SyncCaseSerializer sync_case_serializer)
        {
            SyncCaseIncrementalParser sync_case_incremental_parser(std::move(sync_case_serializer));
            size_t incremental_length = json_sv.length() % 3;

            for( size_t i = 0; i < json_sv.length(); )
            {
                sync_case_incremental_parser.Update(json_sv.substr(i, incremental_length));
                i += incremental_length;
                incremental_length = ( incremental_length + 361 ) % 7;
            }

            return sync_case_incremental_parser.Finish();
        }

    public:
        TEST_METHOD(TestCaseWithBlobData)
        {
            const std::string json = R"([{"id":"guid1", "caseids":"  1", "data":["1  1data1line1","1  1data1line2"],"notes":[{"content":"This is note one","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":2,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"op1","modifiedTime":"2016-10-19T22:07:23Z"},{"content":"This is note two","field":{"name":"TESTDICT_ID"},"operatorId":"op1","modifiedTime":"2000-05-05T10:15:00Z"}],"clock":[{"deviceId":"dev1","revision":1},{"deviceId":"dev2","revision":2}]},{"id":"guid2","caseids":"  2","data":["1  2data2"],"deleted":true,"verified":true,"partialSave":{"mode":"add","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0}},"clock":[{"deviceId":"dev1","revision":1},{"deviceId":"dev2","revision":2}]}])";

            std::unique_ptr<const CDataDict> dictionary = CreateTestDictionary();
            const std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

            const std::vector<std::shared_ptr<Case>> case_vector = ParseIncrementally(json, SyncCaseSerializer(case_access, SyncCaseSerializer::Version::V1));
            Assert::AreEqual(size_t(2), case_vector.size());

            Assert::AreEqual("guid1", case_vector[0]->GetUuid().c_str());
            CaseRecord& idRecord0 = case_vector[0]->GetRootCaseLevel().GetIdCaseRecord();
            Assert::AreEqual(1.0, static_cast<const NumericCaseItem*>(idRecord0.GetCaseItems().front())->GetValue(idRecord0.GetCaseItemIndex()));
            CaseRecord& dataRecord0 = case_vector[0]->GetRootCaseLevel().GetCaseRecord(0);
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
            Assert::AreEqual(int64_t(1476914843), case_vector[0]->GetNotes()[0].GetModifiedDateTime());
            Assert::AreEqual("This is note two", case_vector[0]->GetNotes()[1].GetContent().c_str());
            Assert::AreEqual("TESTDICT_ID", case_vector[0]->GetNotes()[1].GetNamedReference().GetName().c_str());
            Assert::AreEqual("", case_vector[0]->GetNotes()[1].GetNamedReference().GetLevelKey().c_str());
            Assert::AreEqual(size_t(1), case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[0]);
            Assert::AreEqual(size_t(1), case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[1]);
            Assert::AreEqual(size_t(0), case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[2]);
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


        TEST_METHOD(TestCaseBrokenOutData)
        {
            const std::string json = R"([{"id":"997f1b93-b3c7-468e-ba29-7c9b799a0281","caseids":"01010010011 1","clock":[{"deviceId":"0a0027000009","revision":3}],"level-1":{"id":{"PROVINCE":1,"DISTRICT":1,"VILLAGE":"001","EA":1,"UR":1,"COMPOUND":1},"COMPOUND_INFO_REC":{"LATITUDE":234.0,"LONGITUDE":234234.567,"HOUSING_UNITS":[1,2,3,4,5],"NUM_HUS":1},"level-2":[{"id":{"HOUSING_UNIT_NUMBER":1},"HOUSING_UNIT_REC":{"HU01_TYPE":1,"HU02_WALL":1,"HU03_ROOF":1,"HU04_FLOOR":1,"HU05_NUM_HOUSEHOLDS":1},"level-3":[{"id":{"HOUSEHOLD_NUMBER":1},"HOUSEHOLD_REC":{"H06_TENURE":1,"H07_RENT":123,"H08_TOILET":4,"H09_BATH":3,"H10_WATER":2,"H11_LIGHT":1,"H12_FUEL":2,"H13_PERSONS":1},"PERSON_REC":[{"FIRST_NAME":"newdude","P02_REL":1,"P03_SEX":8,"P04_DOB":2000101,"P04DOB_YEAR":200,"P04DOB_MONTH":1,"P04DOB_DAY":1,"P04_AGE":99,"P05_MS":1,"P06_MOTHER":2,"P07_BIRTH":3,"P08_RES95":3,"P09_ATTEND":2,"P11_LITERACY":1,"P12_WORKING":2,"P13_LOOKING":1,"P14_WHY_NOT":2,"P15_OCC":1,"P15A_OCC":0,"SINGLE_PARENT_OF_REPEATING_SUBITEM":"12*4","REPEATING_SUBITEM":[1,2,"DEFAULT"],"NOT_REPEATING_SUBITEM":4,"REPEATING_PARENT":["","2"],"SUB_ONE":["",2],"P25_MORE":2}]}]}]}}])";

            ConnectionString test_file(Path::Combine(GetTestFilesDirectory(), "ThreeLevelTest.csdb"));
            const std::unique_ptr<CDataDict> dictionary = DictionarySource::GetEmbeddedDictionary(test_file);
            const std::shared_ptr<const CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

            SyncCaseSerializer sync_case_serializer_v2(case_access, SyncCaseSerializer::Version::V2);

            const std::vector<std::shared_ptr<Case>> cases = ParseIncrementally(json, sync_case_serializer_v2);
            Assert::AreEqual(size_t(1), cases.size());
            Assert::AreEqual("997f1b93-b3c7-468e-ba29-7c9b799a0281", cases.front()->GetUuid().c_str());

            assert_cast<SyncCaseV2JsonSerializer&>(sync_case_serializer_v2.GetSyncCaseJsonSerializer()).GetSyncCaseV2JsonWriter().SetStringifyLevelData(false);
            const std::string round_trip_json = sync_case_serializer_v2.GetSyncableJson(SpanHelpers::CreatePointersSpan(cases));
            Assert::AreEqual(json, round_trip_json);
        }


        TEST_METHOD(TestCaseStringifiedData)
        {
            const std::string json = R"([{"id":"997f1b93-b3c7-468e-ba29-7c9b799a0281","caseids":"01010010011 1","clock":[{"deviceId":"0a0027000009","revision":3}],"level-1":"{\"id\":{\"PROVINCE\":1,\"DISTRICT\":1,\"VILLAGE\":\"001\",\"EA\":1,\"UR\":1,\"COMPOUND\":1},\"COMPOUND_INFO_REC\":{\"LATITUDE\":234.0,\"LONGITUDE\":234234.567,\"HOUSING_UNITS\":[1,2,3,4,5],\"NUM_HUS\":1},\"level-2\":[{\"id\":{\"HOUSING_UNIT_NUMBER\":1},\"HOUSING_UNIT_REC\":{\"HU01_TYPE\":1,\"HU02_WALL\":1,\"HU03_ROOF\":1,\"HU04_FLOOR\":1,\"HU05_NUM_HOUSEHOLDS\":1},\"level-3\":[{\"id\":{\"HOUSEHOLD_NUMBER\":1},\"HOUSEHOLD_REC\":{\"H06_TENURE\":1,\"H07_RENT\":123,\"H08_TOILET\":4,\"H09_BATH\":3,\"H10_WATER\":2,\"H11_LIGHT\":1,\"H12_FUEL\":2,\"H13_PERSONS\":1},\"PERSON_REC\":[{\"FIRST_NAME\":\"newdude\",\"P02_REL\":1,\"P03_SEX\":8,\"P04_DOB\":2000101,\"P04DOB_YEAR\":200,\"P04DOB_MONTH\":1,\"P04DOB_DAY\":1,\"P04_AGE\":99,\"P05_MS\":1,\"P06_MOTHER\":2,\"P07_BIRTH\":3,\"P08_RES95\":3,\"P09_ATTEND\":2,\"P11_LITERACY\":1,\"P12_WORKING\":2,\"P13_LOOKING\":1,\"P14_WHY_NOT\":2,\"P15_OCC\":1,\"P15A_OCC\":0,\"SINGLE_PARENT_OF_REPEATING_SUBITEM\":\"12*4\",\"REPEATING_SUBITEM\":[1,2,\"DEFAULT\"],\"NOT_REPEATING_SUBITEM\":4,\"REPEATING_PARENT\":[\"\",\"2\"],\"SUB_ONE\":[\"\",2],\"P25_MORE\":2}]}]}]}"}])";

            ConnectionString test_file(Path::Combine(GetTestFilesDirectory(), "ThreeLevelTest.csdb"));
            const std::unique_ptr<CDataDict> dictionary = DictionarySource::GetEmbeddedDictionary(test_file);
            std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

            SyncCaseSerializer sync_case_serializer_v2(case_access, SyncCaseSerializer::Version::V2);

            const std::vector<std::shared_ptr<Case>> case_vector = ParseIncrementally(json, sync_case_serializer_v2);
            Assert::AreEqual(size_t(1), case_vector.size());
            Assert::AreEqual("997f1b93-b3c7-468e-ba29-7c9b799a0281", case_vector[0]->GetUuid().c_str());

            const std::string round_trip_json = sync_case_serializer_v2.GetSyncableJson(*case_vector.front(), true);
            Assert::AreEqual(json, round_trip_json);
        }


        TEST_METHOD(TestIgnoreUnknownKeys)
        {
            const std::string json = R"([{"id":"guid1", "unknownkey": 12, "caseids":"  1", "level-1":{"id":{"TESTDICT_ID":1, "NEWID":1}, "TESTDICT_REC":{"TEST_ITEM":"BLAAAAA", "NEW_ITEM":[1,2,3,4]}, "NEW_RECORD":[{"FOO":1, "BAR":2},{"FOO":1, "BAR":2}]}, "anotherunknown": {"foo":1, "bar":[1,2,3,4], "baz":{"color":"blue", "value":[255,0,0]}}, "notes":[{"content":"This is note one","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":2,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"op1","modifiedTime":"2016-10-19T22:07:23Z"},{"content":"This is note two","field":{"name":"TESTDICT_ID"},"operatorId":"op1","modifiedTime":"2000-05-05T10:15:00Z"}],"clock":[{"deviceId":"dev1","revision":1, "unknown3":{"foo":[{"bar1":1},{"bar2":2}]}},{"deviceId":"dev2","revision":2}]},{"id":"guid2","caseids":"  2","data":["1  2data2"],"deleted":true,"verified":true,"partialSave":{"mode":"add","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0}},"clock":[{"deviceId":"dev1","revision":1},{"deviceId":"dev2","revision":2}]}])";

            std::unique_ptr<const CDataDict> dictionary = CreateTestDictionary();
            const std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

            const std::vector<std::shared_ptr<Case>> case_vector = ParseIncrementally(json, SyncCaseSerializer(case_access, SyncCaseSerializer::Version::V2));
            Assert::AreEqual(size_t(2), case_vector.size());
        }


        TEST_METHOD(TestTooManyRecordOccs)
        {
            const std::string json = R"([{"id":"guid1", "caseids":"  1", "level-1":{"id":{"TESTDICT_ID":1}, "TESTDICT_REC":[{"TEST_ITEM":"1"},{"TEST_ITEM":"2"},{"TEST_ITEM":"3"},{"TEST_ITEM":"4"},{"TEST_ITEM":"5"},{"TEST_ITEM":"6"},{"TEST_ITEM":"7"}]},"clock":[{"deviceId":"dev1","revision":1}]}])";

            std::unique_ptr<CDataDict> dictionary = CreateTestDictionary();
            dictionary->GetLevel(0).GetRecord(0)->SetMaxRecs(5);

            const std::shared_ptr<CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

            const std::vector<std::shared_ptr<Case>> case_vector = ParseIncrementally(json, SyncCaseSerializer(case_access, SyncCaseSerializer::Version::V2));
            Assert::AreEqual(size_t(1), case_vector.size());

            const size_t max_record_occs = case_access->GetCaseMetadata().GetCaseLevelsMetadata()[0].GetCaseRecordsMetadata()[0].GetDictRecord().GetMaxRecs();
            const size_t actual_record_occs = case_vector[0]->GetRootCaseLevel().GetCaseRecord(0).GetNumberOccurrences();
            Assert::AreEqual(max_record_occs, actual_record_occs);
        }


        TEST_METHOD(TestBinaryDataInStream)
        {
            const std::unique_ptr<const CDataDict> dictionary = CreateTestDictionary();
            const std::shared_ptr<const CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

            const std::vector<std::string> record_data = { "record data" };

            const std::vector<std::shared_ptr<Case>> cases =
            {
                CreateCase(*case_access, CreateUuid(), 1, record_data, false),
                CreateCase(*case_access, CreateUuid(), 1, record_data, false),
                CreateCase(*case_access, CreateUuid(), 1, record_data, true),
                CreateCase(*case_access, CreateUuid(), 1, record_data, false)
            };

            SyncCaseSerializer sync_case_serializer(case_access, SyncCaseSerializer::Version::V3);
            const std::string case_json_only = sync_case_serializer.GetSyncableCaseData(SpanHelpers::CreatePointersSpan(cases), nullptr);
            const std::string case_data = GetSyncableCaseData(case_access, SpanHelpers::CreatePointersSpan(cases), sync_case_serializer);
            ASSERT(( case_json_only.length() * 4 / 3 ) < case_data.length());

            const std::string_view case_data_to_parse_sv_1 = std::string_view(case_data).substr(0, case_json_only.length() / 3);
            const std::string_view case_data_to_parse_sv_2 = std::string_view(case_data).substr(case_data_to_parse_sv_1.length(), case_json_only.length());
            const std::string_view case_data_to_parse_sv_3 = std::string_view(case_data).substr(case_data_to_parse_sv_1.length() + case_data_to_parse_sv_2.length());

            // test 1: serializing only a third of the case JSON should result in the ability to parse the first case, which has no binary data
            SyncCaseIncrementalParser sync_case_incremental_parser(sync_case_serializer);
            sync_case_incremental_parser.Update(case_data_to_parse_sv_1);
            Assert::AreEqual(size_t(1), sync_case_incremental_parser.GetParsedCaseCount());

            const std::unique_ptr<const std::vector<std::shared_ptr<Case>>> parsed_cases_1 = sync_case_incremental_parser.ReleaseParseableCases();
            Assert::IsNotNull(parsed_cases_1.get());
            Assert::AreEqual(size_t(1), parsed_cases_1->size());

            // test 2: serializing the rest of the case JSON should result in four cases parsed, but with only the second case ready
            // for parsing because the third case has unprocessed binary data
            sync_case_incremental_parser.Update(case_data_to_parse_sv_2);
            Assert::AreEqual(size_t(4), sync_case_incremental_parser.GetParsedCaseCount());

            const std::unique_ptr<const std::vector<std::shared_ptr<Case>>> parsed_cases_2 = sync_case_incremental_parser.ReleaseParseableCases();
            Assert::IsNotNull(parsed_cases_2.get());
            Assert::AreEqual(size_t(1), parsed_cases_2->size());

            // test 3: after serializing all of the case data, all four cases should be parsed
            sync_case_incremental_parser.Update(case_data_to_parse_sv_3);
            Assert::IsNull(sync_case_incremental_parser.ReleaseParseableCases().get());

            const std::vector<std::shared_ptr<Case>> parsed_cases_3 = sync_case_incremental_parser.Finish();
            Assert::AreEqual(size_t(2), parsed_cases_3.size());

            // test 4: the cases should be equal
            CompareCases(SpanHelpers::CreatePointersSpan(cases),
                         { parsed_cases_1->front().get(), parsed_cases_2->front().get(),
                           parsed_cases_3.front().get(), parsed_cases_3.back().get(), },
                         CompareCasesType::CountsAndBinaryData);
        }
    };
}
