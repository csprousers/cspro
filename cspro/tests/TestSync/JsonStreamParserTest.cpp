#include "stdafx.h"
#include "CppUnitTest.h"
#include "CaseTestHelpers.h"
#include <zToolsO/VectorHelpers.h>
#include <zUtilO/ConnectionString.h>
#include <zDictO/DDClass.h>
#include <zDataO/DataRepositoryHelpers.h>
#include <zSyncO/JsonStreamParser.h>
#include <zSyncO/CaseJsonStreamParser.h>
#include <zSyncO/CaseJsonWriter.h>
#include <rxcpp/rx.hpp>
#include <rxcpp/operators/rx-map.hpp>
#include <rxcpp/operators/rx-reduce.hpp>

using namespace JsonStreamParser;

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SyncUnitTest
{

    class NumberedThing {
    public:
        NumberedThing()
        {
            Logger::WriteMessage("Construct");
        }

        ~NumberedThing()
        {
            Logger::WriteMessage("Destruct");
        }

        NumberedThing(std::string_view s)
            : thing(s)
        {
            Logger::WriteMessage("Construct from string");
        }

        NumberedThing(std::string&& s)
        {
            std::swap(thing, s);
            Logger::WriteMessage("Move Construct from string");
        }

        NumberedThing(const NumberedThing& rhs)
        {
            thing = rhs.thing;
            Logger::WriteMessage("Copy Construct");
        }

        NumberedThing(NumberedThing&& rhs) noexcept
        {
            std::swap(thing, rhs.thing);
            Logger::WriteMessage("Move Construct");
        }

        NumberedThing& operator=(const NumberedThing& rhs)
        {
            Logger::WriteMessage("Assign");
            thing = rhs.thing;
            return *this;
        }

        NumberedThing& operator=(NumberedThing&& rhs) noexcept
        {
            Logger::WriteMessage("Move Assign");
            std::swap(thing, rhs.thing);
            return *this;
        }


        std::string thing;
    };

    class ATestClass {
    public:
        std::string string1;
        double number1;
        std::vector<NumberedThing> list1;
        NumberedThing sub;
    };


    TEST_CLASS(JsonStreamParserTest)
    {
    public:

        TEST_METHOD(TestCaseWithBlobData)
        {
            auto json(R"([{"id":"guid1", "caseids":"1", "data":["11data1line1","11data1line2"],"notes":[{"content":"This is note one","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":2,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"op1","modifiedTime":"2016-10-19T22:07:23Z"},{"content":"This is note two","field":{"name":"TESTDICT_ID"},"operatorId":"op1","modifiedTime":"2000-05-05T10:15:00Z"}],"clock":[{"deviceId":"dev1","revision":1},{"deviceId":"dev2","revision":2}]},{"id":"guid2","caseids":"2","data":["12data2"],"deleted":true,"verified":true,"partialSave":{"mode":"add","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0}},"clock":[{"deviceId":"dev1","revision":1},{"deviceId":"dev2","revision":2}]}])");

            std::unique_ptr<const CDataDict> dict = createTestDict();
            auto case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dict);
            CaseJsonStreamParser parser(*case_access);

            std::vector<std::shared_ptr<Case>> case_vector = parser.Parse(json);
            VectorHelpers::Append(case_vector, parser.EndParse());

            Assert::AreEqual((size_t)2, case_vector.size());

            Assert::AreEqual(L"guid1", case_vector[0]->GetUuid());
            auto& idRecord0 = case_vector[0]->GetRootCaseLevel().GetIdCaseRecord();
            Assert::AreEqual(1.0, static_cast<const NumericCaseItem*>(idRecord0.GetCaseItems().front())->GetValue(idRecord0.GetCaseItemIndex()));
            auto& dataRecord0 = case_vector[0]->GetRootCaseLevel().GetCaseRecord(0);
            Assert::AreEqual((size_t)2, dataRecord0.GetNumberOccurrences());
            Assert::AreEqual(L"data1line1                    ", static_cast<const StringCaseItem*>(dataRecord0.GetCaseItems().front())->GetValue(dataRecord0.GetCaseItemIndex(0)));
            Assert::AreEqual(L"data1line2                    ", static_cast<const StringCaseItem*>(dataRecord0.GetCaseItems().front())->GetValue(dataRecord0.GetCaseItemIndex(1)));
            Assert::AreEqual(1, case_vector[0]->GetVectorClock().getVersion("dev1"));
            Assert::AreEqual(2, case_vector[0]->GetVectorClock().getVersion("dev2"));
            Assert::AreEqual((size_t)2, case_vector[0]->GetNotes().size());
            Assert::AreEqual(L"This is note one", case_vector[0]->GetNotes()[0].GetContent());
            Assert::AreEqual(L"TEST_ITEM", case_vector[0]->GetNotes()[0].GetNamedReference().GetName());
            Assert::AreEqual(L"", case_vector[0]->GetNotes()[0].GetNamedReference().GetLevelKey());
            Assert::AreEqual((size_t)2, case_vector[0]->GetNotes()[0].GetNamedReference().GetOneBasedOccurrences()[0]);
            Assert::AreEqual((size_t)1, case_vector[0]->GetNotes()[0].GetNamedReference().GetOneBasedOccurrences()[1]);
            Assert::AreEqual((size_t)0, case_vector[0]->GetNotes()[0].GetNamedReference().GetOneBasedOccurrences()[2]);
            Assert::AreEqual(L"op1", case_vector[0]->GetNotes()[0].GetOperatorId());
            Assert::AreEqual((int64_t)1476914843, (int64_t)case_vector[0]->GetNotes()[0].GetModifiedDateTime());
            Assert::AreEqual(L"This is note two", case_vector[0]->GetNotes()[1].GetContent());
            Assert::AreEqual(L"TESTDICT_ID", case_vector[0]->GetNotes()[1].GetNamedReference().GetName());
            Assert::AreEqual(L"", case_vector[0]->GetNotes()[1].GetNamedReference().GetLevelKey());
            Assert::AreEqual((size_t)1, case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[0]);
            Assert::AreEqual((size_t)1, case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[1]);
            Assert::AreEqual((size_t)0, case_vector[0]->GetNotes()[1].GetNamedReference().GetOneBasedOccurrences()[2]);
            Assert::AreEqual((int64_t)957521700, (int64_t)case_vector[0]->GetNotes()[1].GetModifiedDateTime());
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

        TEST_METHOD(TestCaseBrokenOutData)
        {
            auto json = R"([{"id":"997f1b93-b3c7-468e-ba29-7c9b799a0281","caseids":"01010010011 1","clock":[{"deviceId":"0a0027000009","revision":3}],"level-1":{"id":{"PROVINCE":1,"DISTRICT":1,"VILLAGE":"001","EA":1,"UR":1,"COMPOUND":1},"COMPOUND_INFO_REC":{"LATITUDE":234.0,"LONGITUDE":234234.567,"HOUSING_UNITS":[1,2,3,4,5],"NUM_HUS":1},"level-2":[{"id":{"HOUSING_UNIT_NUMBER":1},"HOUSING_UNIT_REC":{"HU01_TYPE":1,"HU02_WALL":1,"HU03_ROOF":1,"HU04_FLOOR":1,"HU05_NUM_HOUSEHOLDS":1},"level-3":[{"id":{"HOUSEHOLD_NUMBER":1},"HOUSEHOLD_REC":{"H06_TENURE":1,"H07_RENT":123,"H08_TOILET":4,"H09_BATH":3,"H10_WATER":2,"H11_LIGHT":1,"H12_FUEL":2,"H13_PERSONS":1},"PERSON_REC":[{"FIRST_NAME":"newdude","P02_REL":1,"P03_SEX":8,"P04_DOB":2000101,"P04DOB_YEAR":200,"P04DOB_MONTH":1,"P04DOB_DAY":1,"P04_AGE":99,"P05_MS":1,"P06_MOTHER":2,"P07_BIRTH":3,"P08_RES95":3,"P09_ATTEND":2,"P11_LITERACY":1,"P12_WORKING":2,"P13_LOOKING":1,"P14_WHY_NOT":2,"P15_OCC":1,"P15A_OCC":0,"SINGLE_PARENT_OF_REPEATING_SUBITEM":"12*4","REPEATING_SUBITEM":[1,2,"DEFAULT"],"NOT_REPEATING_SUBITEM":4,"REPEATING_PARENT":["","2"],"SUB_ONE":["",2],"P25_MORE":2}]}]}]}}])";

            ConnectionString test_file("ThreeLevelTest.csdb");
            auto dictionary = DataRepositoryHelpers::GetEmbeddedDictionary(test_file);
            auto case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);
            CaseJsonStreamParser parser(*case_access);

            std::vector<std::shared_ptr<Case>> case_vector = parser.Parse(json);
            VectorHelpers::Append(case_vector, parser.EndParse());

            Assert::AreEqual((size_t)1, case_vector.size());
            Assert::AreEqual(L"997f1b93-b3c7-468e-ba29-7c9b799a0281", case_vector[0]->GetUuid());

            std::ostringstream os;
            CaseJsonWriter case_writer;
            case_writer.ToJson(os, case_vector);
            auto round_trip_json = os.str();
            Assert::AreEqual(json, round_trip_json.c_str());

        }

        TEST_METHOD(TestCaseStringifiedData)
        {
            auto json = R"({"id":"997f1b93-b3c7-468e-ba29-7c9b799a0281","caseids":"01010010011 1","clock":[{"deviceId":"0a0027000009","revision":3}],"level-1":"{\"id\":{\"PROVINCE\":1,\"DISTRICT\":1,\"VILLAGE\":\"001\",\"EA\":1,\"UR\":1,\"COMPOUND\":1},\"COMPOUND_INFO_REC\":{\"LATITUDE\":234.0,\"LONGITUDE\":234234.567,\"HOUSING_UNITS\":[1,2,3,4,5],\"NUM_HUS\":1},\"level-2\":[{\"id\":{\"HOUSING_UNIT_NUMBER\":1},\"HOUSING_UNIT_REC\":{\"HU01_TYPE\":1,\"HU02_WALL\":1,\"HU03_ROOF\":1,\"HU04_FLOOR\":1,\"HU05_NUM_HOUSEHOLDS\":1},\"level-3\":[{\"id\":{\"HOUSEHOLD_NUMBER\":1},\"HOUSEHOLD_REC\":{\"H06_TENURE\":1,\"H07_RENT\":123,\"H08_TOILET\":4,\"H09_BATH\":3,\"H10_WATER\":2,\"H11_LIGHT\":1,\"H12_FUEL\":2,\"H13_PERSONS\":1},\"PERSON_REC\":[{\"FIRST_NAME\":\"newdude\",\"P02_REL\":1,\"P03_SEX\":8,\"P04_DOB\":2000101,\"P04DOB_YEAR\":200,\"P04DOB_MONTH\":1,\"P04DOB_DAY\":1,\"P04_AGE\":99,\"P05_MS\":1,\"P06_MOTHER\":2,\"P07_BIRTH\":3,\"P08_RES95\":3,\"P09_ATTEND\":2,\"P11_LITERACY\":1,\"P12_WORKING\":2,\"P13_LOOKING\":1,\"P14_WHY_NOT\":2,\"P15_OCC\":1,\"P15A_OCC\":0,\"SINGLE_PARENT_OF_REPEATING_SUBITEM\":\"12*4\",\"REPEATING_SUBITEM\":[1,2,\"DEFAULT\"],\"NOT_REPEATING_SUBITEM\":4,\"REPEATING_PARENT\":[\"\",\"2\"],\"SUB_ONE\":[\"\",2],\"P25_MORE\":2}]}]}]}"})";

            ConnectionString test_file("ThreeLevelTest.csdb");
            auto dictionary = DataRepositoryHelpers::GetEmbeddedDictionary(test_file);
            auto case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);
            CaseJsonStreamParser parser(*case_access);

            std::vector<std::shared_ptr<Case>> case_vector = parser.Parse(json);
            VectorHelpers::Append(case_vector, parser.EndParse());

            Assert::AreEqual((size_t)1, case_vector.size());
            Assert::AreEqual(L"997f1b93-b3c7-468e-ba29-7c9b799a0281", case_vector[0]->GetUuid());

            std::ostringstream os;
            CaseJsonWriter case_writer;
            case_writer.SetStringifyData(true);
            case_writer.ToJson(os, *case_vector[0]);
            auto round_trip_json = os.str();
            Assert::AreEqual(json, round_trip_json.c_str());

        }

        void SetupTestClassParser(Parser<ObjectType<ATestClass>>& parser)
        {
            auto numbered_thing_type = std::make_shared<ObjectType<NumberedThing>>([](ObjectFieldStorage& field_values) {
                return NumberedThing{ std::move(*field_values.Get<std::string>("thing"))
                }; });
            numbered_thing_type->AddField<std::string>("thing");
            auto numbered_thing_array_type = std::make_shared<ArrayType<ObjectType<NumberedThing>>>();

            auto test_class_type = std::make_shared<ObjectType<ATestClass>>(
                [](ObjectFieldStorage& field_values) {
                    return ATestClass{
                        field_values.Get<std::string>("string1", std::string()),
                        field_values.Get<double>("number1", 0),
                        field_values.Get<std::vector<NumberedThing>>("list1", std::vector<NumberedThing>()),
                        field_values.Get<NumberedThing>("sub", NumberedThing()) };
                });
            test_class_type->AddField<std::string>("string1");
            test_class_type->AddField<double>("number1");
            test_class_type->AddField<std::vector<NumberedThing>>("list1");
            test_class_type->AddField<NumberedThing>("sub");

            parser.GetTypeRegistry().Put(test_class_type);
            parser.GetTypeRegistry().Put(numbered_thing_type);
            parser.GetTypeRegistry().Put(numbered_thing_array_type);
        }

        TEST_METHOD(TestIncrementalParse)
        {
            Parser<ObjectType<ATestClass>> parser;
            SetupTestClassParser(parser);

            std::vector<ATestClass> result;

            auto json_text = rxcpp::observable<>::create<std::string_view>([](rxcpp::subscriber<std::string_view> s) {
                auto json = R"([{"string1":"foo", "number1":67, "sub":{"thing":"one"}},{"string1":"bar", "list1":[{"thing":"one"},{"thing":"two"},{"thing":"three"},{"thing":"four"}]},{"string1":"baz"}])";
                while (strlen(json)) {
                    size_t size = std::min(strlen(json), 50U);
                    s.on_next(std::string_view(json, size));
                    json += size;
                }
                s.on_completed();
                });

            auto foo = json_text | rxcpp::operators::map([&parser](std::string_view text) {return parser.Parse(text); });
            auto bar = foo.reduce(std::vector<ATestClass>(), [](std::vector<ATestClass> v, std::vector<ATestClass> a) {VectorHelpers::Append(v, a); return v; });
            bar.subscribe([&result](std::vector<ATestClass> v) {
                result = v;
                },
                []() {
                    Logger::WriteMessage("OnComplete");
                });
            Assert::AreEqual(3U, result.size());
        }

        TEST_METHOD(TestIgnoreUnknownKeys)
        {
            auto json(R"([{"id":"guid1", "unknownkey": 12, "caseids":"1", "level-1":{"id":{"TESTDICT_ID":1, "NEWID":1}, "TESTDICT_REC":{"TEST_ITEM":"BLAAAAA", "NEW_ITEM":[1,2,3,4]}, "NEW_RECORD":[{"FOO":1, "BAR":2},{"FOO":1, "BAR":2}]}, "anotherunknown": {"foo":1, "bar":[1,2,3,4], "baz":{"color":"blue", "value":[255,0,0]}}, "notes":[{"content":"This is note one","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":2,"itemOccurrence":1,"subitemOccurrence":0},"operatorId":"op1","modifiedTime":"2016-10-19T22:07:23Z"},{"content":"This is note two","field":{"name":"TESTDICT_ID"},"operatorId":"op1","modifiedTime":"2000-05-05T10:15:00Z"}],"clock":[{"deviceId":"dev1","revision":1, "unknown3":{"foo":[{"bar1":1},{"bar2":2}]}},{"deviceId":"dev2","revision":2}]},{"id":"guid2","caseids":"2","data":["12data2"],"deleted":true,"verified":true,"partialSave":{"mode":"add","field":{"name":"TEST_ITEM","levelKey":"","recordOccurrence":1,"itemOccurrence":1,"subitemOccurrence":0}},"clock":[{"deviceId":"dev1","revision":1},{"deviceId":"dev2","revision":2}]}])");

            std::unique_ptr<const CDataDict> dict = createTestDict();
            auto case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dict);
            CaseJsonStreamParser parser(*case_access);

            std::vector<std::shared_ptr<Case>> case_vector = parser.Parse(json);
            VectorHelpers::Append(case_vector, parser.EndParse());

            Assert::AreEqual((size_t)2, case_vector.size());

        }

        TEST_METHOD(TestTooManyRecordOccs)
        {
            auto json(R"({"id":"guid1", "caseids":"1", "level-1":{"id":{"TESTDICT_ID":1}, "TESTDICT_REC":[{"TEST_ITEM":"1"},{"TEST_ITEM":"2"},{"TEST_ITEM":"3"},{"TEST_ITEM":"4"},{"TEST_ITEM":"5"},{"TEST_ITEM":"6"},{"TEST_ITEM":"7"}]},"clock":[{"deviceId":"dev1","revision":1}]})");

            std::unique_ptr<const CDataDict> dict = createTestDict();
            auto case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dict);
            CaseJsonStreamParser parser(*case_access);

            std::vector<std::shared_ptr<Case>> case_vector = parser.Parse(json);
            VectorHelpers::Append(case_vector, parser.EndParse());

            Assert::AreEqual((size_t)1, case_vector.size());
            auto max_record_occs = case_access->GetCaseMetadata().GetCaseLevelsMetadata()[0]->GetCaseRecordsMetadata()[0]->GetDictionaryRecord().GetMaxRecs();
            auto actual_record_occs = case_vector[0]->GetRootCaseLevel().GetCaseRecord(0).GetNumberOccurrences();
            Assert::AreEqual(max_record_occs, actual_record_occs);
        }
    };
}

