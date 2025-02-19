#include "stdafx.h"
#include "CaseTestHelpers.h"
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/CaseConstructionHelpers.h>
#include <zDataO/DataRepository.h>
#include <zDataO/DictionarySource.h>
#include <zDataO/SyncCaseJsonSerializer.h>


namespace SyncUnitTest { class CaseJsonWriterTest; }

TEST_CLASS(SyncUnitTest::CaseJsonWriterTest)
{
public:
    TEST_METHOD(ThreeLevelTest);
    TEST_METHOD(BinaryDataNoDuplicateTest);

private:
    void ParseAndWriteTest(std::shared_ptr<const CaseAccess> case_access, const Case& source_data_case);
};


void SyncUnitTest::CaseJsonWriterTest::ThreeLevelTest()
{
    const ConnectionString three_level_test_connection_string(Path::Combine(GetTestFilesDirectory(), "ThreeLevelTest.csdb"));

    const std::unique_ptr<const CDataDict> dictionary = DictionarySource::GetEmbeddedDictionary(three_level_test_connection_string);
    const std::shared_ptr<const CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

    const std::unique_ptr<DataRepository> repository = DataRepository::CreateAndOpen(case_access, three_level_test_connection_string,
                                                                                     DataRepositoryAccess::ReadOnly, DataRepositoryOpenFlag::OpenMustExist);
    const std::unique_ptr<Case> data_case = case_access->CreateCase();
    repository->ReadCase(*data_case, "01010010011 1");

    // test 1: V2 JSON
    SyncCaseSerializer sync_case_serializer_v2(case_access, SyncCaseSerializer::Version::V2);
    assert_cast<SyncCaseV2JsonSerializer&>(sync_case_serializer_v2.GetSyncCaseJsonSerializer()).GetSyncCaseV2JsonWriter().SetStringifyLevelData(false);
    const std::string actual_v2_json = sync_case_serializer_v2.GetSyncableJson(*data_case, false);
    constexpr const char* expected_v2_json = R"({"id":"997f1b93-b3c7-468e-ba29-7c9b799a0281","caseids":"01010010011 1","clock":[{"deviceId":"0a0027000009","revision":3}],"level-1":{"id":{"PROVINCE":1,"DISTRICT":1,"VILLAGE":"001","EA":1,"UR":1,"COMPOUND":1},"COMPOUND_INFO_REC":{"LATITUDE":234.0,"LONGITUDE":234234.567,"HOUSING_UNITS":[1,2,3,4,5],"NUM_HUS":1},"level-2":[{"id":{"HOUSING_UNIT_NUMBER":1},"HOUSING_UNIT_REC":{"HU01_TYPE":1,"HU02_WALL":1,"HU03_ROOF":1,"HU04_FLOOR":1,"HU05_NUM_HOUSEHOLDS":1},"level-3":[{"id":{"HOUSEHOLD_NUMBER":1},"HOUSEHOLD_REC":{"H06_TENURE":1,"H07_RENT":123,"H08_TOILET":4,"H09_BATH":3,"H10_WATER":2,"H11_LIGHT":1,"H12_FUEL":2,"H13_PERSONS":1},"PERSON_REC":[{"FIRST_NAME":"newdude","P02_REL":1,"P03_SEX":8,"P04_DOB":2000101,"P04DOB_YEAR":200,"P04DOB_MONTH":1,"P04DOB_DAY":1,"P04_AGE":99,"P05_MS":1,"P06_MOTHER":2,"P07_BIRTH":3,"P08_RES95":3,"P09_ATTEND":2,"P11_LITERACY":1,"P12_WORKING":2,"P13_LOOKING":1,"P14_WHY_NOT":2,"P15_OCC":1,"P15A_OCC":0,"SINGLE_PARENT_OF_REPEATING_SUBITEM":"12*4","REPEATING_SUBITEM":[1,2,"DEFAULT"],"NOT_REPEATING_SUBITEM":4,"REPEATING_PARENT":["","2"],"SUB_ONE":["",2],"P25_MORE":2}]}]}]}})";
    Assert::AreEqual(expected_v2_json, actual_v2_json.c_str());

    // test 2: V3 JSON
    SyncCaseSerializer sync_case_serializer_v3(case_access, SyncCaseSerializer::Version::V3);
    const std::string actual_v3_json = sync_case_serializer_v3.GetSyncableJson(*data_case, false);
    constexpr const char* expected_v3_json = R"({"key":"01010010011 1","uuid":"997f1b93-b3c7-468e-ba29-7c9b799a0281","COMPOUND_LEVEL":{"PROVINCE":{"code":1},"DISTRICT":{"code":1},"VILLAGE":{"code":"001"},"EA":{"code":1},"UR":{"code":1},"COMPOUND":{"code":1},"COMPOUND_INFO_REC":[{"LATITUDE":{"code":234.0},"LONGITUDE":{"code":234234.567},"HOUSING_UNITS":[{"code":1},{"code":2},{"code":3},{"code":4},{"code":5}],"NUM_HUS":{"code":1}}],"HOUSING_UNIT_LEVEL":[{"HOUSING_UNIT_NUMBER":{"code":1},"HOUSING_UNIT_REC":[{"HU01_TYPE":{"code":1},"HU02_WALL":{"code":1},"HU03_ROOF":{"code":1},"HU04_FLOOR":{"code":1},"HU05_NUM_HOUSEHOLDS":{"code":1}}],"HOUSEHOLD_LEVEL":[{"HOUSEHOLD_NUMBER":{"code":1},"HOUSEHOLD_REC":[{"H06_TENURE":{"code":1},"H07_RENT":{"code":123},"H08_TOILET":{"code":4},"H09_BATH":{"code":3},"H10_WATER":{"code":2},"H11_LIGHT":{"code":1},"H12_FUEL":{"code":2},"H13_PERSONS":{"code":1}}],"PERSON_REC":[{"FIRST_NAME":{"code":"newdude"},"P02_REL":{"code":1},"P03_SEX":{"code":8},"P04_DOB":{"code":2000101},"P04DOB_YEAR":{"code":200},"P04DOB_MONTH":{"code":1},"P04DOB_DAY":{"code":1},"P04_AGE":{"code":99},"P05_MS":{"code":1},"P06_MOTHER":{"code":2},"P07_BIRTH":{"code":3},"P08_RES95":{"code":3},"P09_ATTEND":{"code":2},"P11_LITERACY":{"code":1},"P12_WORKING":{"code":2},"P13_LOOKING":{"code":1},"P14_WHY_NOT":{"code":2},"P15_OCC":{"code":1},"P15A_OCC":{"code":0},"SINGLE_PARENT_OF_REPEATING_SUBITEM":{"code":"12*4"},"REPEATING_SUBITEM":[{"code":1},{"code":2},{"code":"DEFAULT"}],"NOT_REPEATING_SUBITEM":{"code":4},"REPEATING_PARENT":[{},{"code":"2"}],"SUB_ONE":[{},{"code":2}],"P25_MORE":{"code":2}}]}]}]},"clock":[{"deviceId":"0a0027000009","revision":3}]})";
    Assert::AreEqual(expected_v3_json, actual_v3_json.c_str());

    // test 3: parse and write test
    ParseAndWriteTest(case_access, *data_case);

    // test 4: parse and write test, testing more features
    const size_t occurrences[] { 0, 0, 0};
    const std::shared_ptr<CaseItemReference> level0_case_item_reference = CaseConstructionHelpers::CreateCaseItemReference(*case_access, "", "LONGITUDE", occurrences);
    Assert::IsNotNull(level0_case_item_reference.get());
    const std::shared_ptr<CaseItemReference> level1_case_item_reference = CaseConstructionHelpers::CreateCaseItemReference(*case_access, "1", "HU02_WALL", occurrences);
    Assert::IsNotNull(level1_case_item_reference.get());
    const std::shared_ptr<CaseItemReference> level2_case_item_reference = CaseConstructionHelpers::CreateCaseItemReference(*case_access, "11", "H09_BATH", occurrences);
    Assert::IsNotNull(level2_case_item_reference.get());

    data_case->SetPartialSaveStatus(PartialSaveMode::Modify, level2_case_item_reference);

    data_case->GetNotes().emplace_back(u8"note --> 🐼 <-- text", level0_case_item_reference, u8"operator --> 天津 <-- id");
    data_case->GetNotes().emplace_back(u8"note --> 🐼 <-- text", level1_case_item_reference, u8"operator --> 天津 <-- id");
    data_case->GetNotes().emplace_back(u8"note --> 🐼 <-- text", level2_case_item_reference, u8"operator --> 天津 <-- id");

    data_case->GetVectorClock().increment(MakeUniqueDeviceId("CaseJsonWriterTest-"));

    ParseAndWriteTest(case_access, *data_case);
}


void SyncUnitTest::CaseJsonWriterTest::ParseAndWriteTest(const std::shared_ptr<const CaseAccess> case_access, const Case& source_data_case)
{
    // create V1 / V2 (stringified and not) / V3 JSON
    SyncCaseSerializer sync_case_serializer_v1(case_access, SyncCaseSerializer::Version::V1);
    const std::string v1_json = sync_case_serializer_v1.GetSyncableJson(source_data_case, false);

    SyncCaseSerializer sync_case_serializer_v2(case_access, SyncCaseSerializer::Version::V2);
    const std::string v2_json = sync_case_serializer_v2.GetSyncableJson(source_data_case, false);

    assert_cast<SyncCaseV2JsonSerializer&>(sync_case_serializer_v2.GetSyncCaseJsonSerializer()).GetSyncCaseV2JsonWriter().SetStringifyLevelData(false);
    const std::string v2_no_stringify_json = sync_case_serializer_v2.GetSyncableJson(source_data_case, false);

    Assert::AreNotEqual(v1_json, v2_json);
    Assert::AreNotEqual(v1_json, v2_no_stringify_json);
    Assert::AreNotEqual(v2_json, v2_no_stringify_json);

    SyncCaseSerializer sync_case_serializer_v3(case_access, SyncCaseSerializer::Version::V3);
    const std::string v3_json = sync_case_serializer_v3.GetSyncableJson(source_data_case, false);

    // create a case from the JSON and make sure that it matches
    auto test = [&](SyncCaseSerializer& sync_case_serializer, const std::string& json)
    {
        const std::unique_ptr<Case> test_case = case_access->CreateCase();
        sync_case_serializer.GetSyncCaseJsonSerializer().ParseCase(*test_case, Json::Parse(json));
        Assert::IsTrue(source_data_case == *test_case);

        // make sure values are reset in the parse routine
        sync_case_serializer.GetSyncCaseJsonSerializer().ParseCase(*test_case, Json::Parse(json));
        Assert::IsTrue(source_data_case == *test_case);
    };

    test(sync_case_serializer_v1, v1_json);
    test(sync_case_serializer_v2, v2_json);
    test(sync_case_serializer_v2, v2_no_stringify_json);
    test(sync_case_serializer_v3, v3_json);
}


void SyncUnitTest::CaseJsonWriterTest::BinaryDataNoDuplicateTest()
{
    const std::unique_ptr<const CDataDict> dictionary = CreateTestDictionary();
    const std::shared_ptr<const CaseAccess> case_access = CaseAccess::CreateAndInitializeFullCaseAccess(*dictionary);

    const std::string case_uuid = CreateUuid();
    const std::vector<std::string> record_data = { "record data" };
    const std::unique_ptr<const Case> case_without_binary_data = CreateCase(*case_access, case_uuid, 1, record_data, false);
    const std::unique_ptr<const Case> case_with_binary_data = CreateCase(*case_access, case_uuid, 1, record_data, true);

    uint64_t size_binary_content1;
    uint64_t size_binary_content2;

    auto create_case_and_set_second_binary_item = [&](const auto& set_function)
    {
        std::unique_ptr<Case> case_with_multiple_binary_data = case_access->CreateCase();
        *case_with_multiple_binary_data = *case_with_binary_data;

        CaseRecord& data_record = case_with_multiple_binary_data->GetRootCaseLevel().GetCaseRecord(0);
        CaseItemIndex index = data_record.GetCaseItemIndex(0);

        const BinaryCaseItem& binary_case_item1 = *assert_cast<const BinaryCaseItem*>(data_record.GetCaseItems()[1]);
        Assert::IsFalse(binary_case_item1.IsBlank(index));
        size_binary_content1 = binary_case_item1.GetBinaryDataAccessor(index).GetBinaryDataSize();

        const BinaryCaseItem& binary_case_item2 = *assert_cast<const BinaryCaseItem*>(data_record.GetCaseItems()[2]);
        Assert::IsTrue(binary_case_item2.IsBlank(index));

        set_function(binary_case_item1, binary_case_item2, index);
        Assert::IsFalse(binary_case_item2.IsBlank(index));

        return case_with_multiple_binary_data;
    };

    // create a case with the same binary data in two items
    const std::unique_ptr<const Case> case_with_repeated_binary_data = create_case_and_set_second_binary_item(
        [](const BinaryCaseItem& binary_case_item1, const BinaryCaseItem& binary_case_item2, CaseItemIndex& index)
        {
            binary_case_item2.SetValue(index, binary_case_item1.GetBinaryDataAccessor(index));
        });

    // create a case with different binary data in two items (but use the same metadata for both items)
    const std::unique_ptr<const Case> case_with_different_binary_data = create_case_and_set_second_binary_item(
        [&](const BinaryCaseItem& binary_case_item1, const BinaryCaseItem& binary_case_item2, CaseItemIndex& index)
        {
            binary_case_item2.SetValue(index, BinaryData(GetHtmlImage("pff-icon.png"),
                                                         binary_case_item1.GetBinaryDataAccessor(index).GetBinaryDataMetadata()));

            size_binary_content2 = binary_case_item2.GetBinaryDataAccessor(index).GetBinaryDataSize();
        });

    SyncCaseSerializer sync_case_serializer(case_access, SyncCaseSerializer::Version::V3);

    // test 1: without binary data, the syncable case data should equal the JSON
    const std::string case_data_for_case_without_binary_data = GetSyncableCaseData(case_access, { case_without_binary_data.get() }, sync_case_serializer);
    Assert::AreEqual(case_data_for_case_without_binary_data, sync_case_serializer.GetSyncableJson({ case_without_binary_data.get() }));

    // test 2: with binary data, the syncable case data should not equal the JSON
    const std::string case_data_for_case_with_binary_data = GetSyncableCaseData(case_access, { case_with_binary_data.get() }, sync_case_serializer);
    Assert::AreNotEqual(case_data_for_case_with_binary_data, sync_case_serializer.GetSyncableJson({ case_with_binary_data.get() }));
    Assert::IsTrue(case_data_for_case_with_binary_data.length() > size_binary_content1);

    // test 3: with repeated binary data, the syncable case data should not be much bigger
    constexpr size_t EstimatedLengthOfWrittenMetadataForBinaryData = 200;
    const std::string case_data_for_case_with_repeated_binary_data = GetSyncableCaseData(case_access, { case_with_repeated_binary_data.get() }, sync_case_serializer);
    const size_t repeated_binary_length_difference = case_data_for_case_with_repeated_binary_data.length() - case_data_for_case_with_binary_data.length();
    Assert::IsTrue(repeated_binary_length_difference < EstimatedLengthOfWrittenMetadataForBinaryData);

    // test 4: with different binary data, the syncable case data should be bigger by roughly the size of the second binary content
    const std::string case_data_for_case_with_different_binary_data = GetSyncableCaseData(case_access, { case_with_different_binary_data.get() }, sync_case_serializer);
    const size_t different_binary_length_difference = case_data_for_case_with_different_binary_data.length() - case_data_for_case_with_repeated_binary_data.length();
    Assert::IsTrue(different_binary_length_difference > size_binary_content2);
    Assert::IsTrue(different_binary_length_difference < ( size_binary_content2 + EstimatedLengthOfWrittenMetadataForBinaryData) );
}
