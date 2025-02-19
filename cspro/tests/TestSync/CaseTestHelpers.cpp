#include "stdafx.h"
#include "CaseTestHelpers.h"
#include <zToolsO/PortableFunctions.h>
#include <zUtilO/BinaryDataMetadata.h>
#include <zUtilO/CSProExecutables.h>
#include <zDictO/DDClass.h>
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/CaseItemReference.h>
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zDataO/SyncBinaryDataUploadManager.h>
#include <fstream>


DeviceId MakeUniqueDeviceId(std::string prefix)
{
    // Currently device id is limited to 16 chars on CSWeb
    // so use prefix and fill rest with guid which should
    // unique enough.
    return prefix + CreateUuid().substr(0, 16 - prefix.length());
}


std::string GetTestFilesDirectory()
{
    return MakeFullPath(PortableFunctions::PathGetDirectory(__FILE__) , "Test Files");
}


std::unique_ptr<CDataDict> CreateTestDictionary()
{
    return CDataDict::InstantiateAndOpen(Path::Combine(GetTestFilesDirectory(), "TestDict.dcf"), true);
}


std::unique_ptr<CDataDict> CreateTestDictionaryWithUniqueName()
{
    // use the test dictionary but give it a unique name, truncated to 30 characters to keep it, and related tables, under the MySQL max identifier length of 64
    std::unique_ptr<CDataDict> dictionary = CreateTestDictionary();

    std::string dictionary_name = dictionary->GetName() + CreateUuid();
    SO::WideMakeExactLength(dictionary_name, 30);
    dictionary_name = CIMSAString::MakeName(dictionary_name);

    dictionary->SetName(std::move(dictionary_name));

    return dictionary;
}


std::shared_ptr<const std::vector<std::byte>> GetHtmlImage(const std::string& filename)
{
    static std::map<std::string, std::shared_ptr<const std::vector<std::byte>>> content;
    auto lookup = content.find(filename);

    if( lookup == content.cend() )
    {
        const std::string image_file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Images), filename);
        lookup = content.try_emplace(filename, FileIO::Read(image_file_path)).first;
    }

    return lookup->second;
}


std::unique_ptr<Case> CreateCase(const CaseAccess& case_access, std::string uuid, const int numeric_case_id,
                                 const std::vector<std::string>& string_values, const bool add_data_to_image_item/* = true*/)
{
    auto data_case = std::make_unique<Case>(case_access.GetCaseMetadata());
    data_case->SetUuid(std::move(uuid));

    CaseRecord& id_record = data_case->GetRootCaseLevel().GetIdCaseRecord();
    CaseItemIndex id_index = id_record.GetCaseItemIndex();
    static_cast<const NumericCaseItem*>(id_record.GetCaseItems().front())->SetValue(id_index, numeric_case_id);

    CaseRecord& data_record = data_case->GetRootCaseLevel().GetCaseRecord(0);
    data_record.SetNumberOccurrences(string_values.size());

    for( size_t i = 0; i < string_values.size(); ++i )
    {
        CaseItemIndex index = data_record.GetCaseItemIndex(i);

        // the first item in the test record is a string
        const StringCaseItem& string_case_item = *assert_cast<const StringCaseItem*>(data_record.GetCaseItems().front());
        string_case_item.SetValue(index, string_values[i]);

        // the second item in the test record is a binary item
        if( add_data_to_image_item )
        {
            BinaryDataMetadata binary_data_metadata;
            binary_data_metadata.SetFilename("CSPro Logo.png");
            binary_data_metadata.SetProperty(FormatText("Property%d", i), FormatText("Value%d", i));

            const BinaryCaseItem& binary_case_item = *assert_cast<const BinaryCaseItem*>(data_record.GetCaseItems()[1]);
            binary_case_item.SetValue(index, BinaryData(GetHtmlImage("cspro-logo-medium.png"), std::move(binary_data_metadata)));
        }
    }

    return data_case;
}


Note createNote(const CaseAccess& case_access, std::string content,
                const std::string& field_name, int rec_occ, int item_occ, int sub_occ,
                std::string opid, int64_t mod_time)
{
    const CaseItem* const case_item = case_access.LookupCaseItem(field_name);
    ASSERT(case_item != nullptr);
    auto case_item_reference = std::make_unique<CaseItemReference>(*case_item, std::string(), rec_occ, item_occ, sub_occ);
    return Note(std::move(content), std::move(case_item_reference), std::move(opid), mod_time);
}


void addItemNote(const CaseAccess& case_access, Case& data_case, std::string content,
                 const std::string& field_name, int rec_occ, int item_occ, int sub_occ,
                 std::string opid, int64_t mod_time)
{
    std::vector<Note>& notes = data_case.GetNotes();
    notes.emplace_back(createNote(case_access, std::move(content), field_name, rec_occ, item_occ, sub_occ, std::move(opid), mod_time));
}


void SetPartialSave(const CaseAccess& case_access, Case& data_case, const PartialSaveMode mode, const std::string& field_name,
                    const size_t record_occurrence/* = 0*/, const size_t item_occurrence/* = 0*/, const size_t subitem_occurrence/* = 0*/)
{
    const CaseItem* const case_item = case_access.LookupCaseItem(field_name);
    Assert::IsNotNull(case_item);

    data_case.SetPartialSaveStatus(mode, std::make_unique<CaseItemReference>(*case_item, std::string(), record_occurrence, item_occurrence, subitem_occurrence));
}


std::unique_ptr<SyncBinaryDataUploadManager> CreateSyncBinaryDataUploadManager(const std::shared_ptr<const CaseAccess> case_access,
                                                                               const cs::span<const Case* const> cases_to_analyze/* = cs::span<const Case* const>*/)
{
    ASSERT(case_access != nullptr);

    if( !case_access->GetCaseMetadata().UsesBinaryData() )
        return nullptr;

    // simulate uploading all binary data
    class TestSyncBinaryDataUploadManager : public SyncBinaryDataUploadManager
    {
    protected:
        void AddBinarySignaturesNotSyncedWithRemote(const Case& data_case, std::vector<std::string>& signatures_to_sync) override
        {
            data_case.ForeachDefinedBinaryCaseItem(
                [&](const BinaryCaseItem& binary_case_item, const CaseItemIndex& index)
                {
                    const BinaryDataAccessor& binary_data_accessor = binary_case_item.GetBinaryDataAccessor(index);
                    signatures_to_sync.emplace_back(binary_data_accessor.GetSignature());
                });
        }
    };

    std::unique_ptr<SyncBinaryDataUploadManager> sync_binary_data_upload_manager = std::make_unique<TestSyncBinaryDataUploadManager>();

    for( const Case* const data_case : cases_to_analyze )
        sync_binary_data_upload_manager->AnalyzeCaseBinaryData(*data_case);

    return sync_binary_data_upload_manager;
}


std::string GetSyncableCaseData(const std::shared_ptr<const CaseAccess> case_access, const cs::span<const Case* const> cases, SyncCaseSerializer& sync_case_serializer)
{
    ASSERT(case_access != nullptr);

    const std::unique_ptr<SyncBinaryDataUploadManager> sync_binary_data_upload_manager = CreateSyncBinaryDataUploadManager(case_access, cases);

    return sync_case_serializer.GetSyncableCaseData(cases, sync_binary_data_upload_manager.get());
}


std::string GetSyncableCaseData(std::shared_ptr<const CaseAccess> case_access, const cs::span<const Case* const> cases, const SyncCaseSerializer::Version version)
{
    ASSERT(case_access != nullptr);

    SyncCaseSerializer sync_case_serializer(case_access, version);

    return GetSyncableCaseData(std::move(case_access), cases, sync_case_serializer);
}


void CompareCases(const cs::span<const Case* const> expected_cases, const cs::span<const Case* const> actual_cases, const CompareCasesType compare_cases_type)
{
    Assert::AreEqual(expected_cases.size(), actual_cases.size());

    if( compare_cases_type == CompareCasesType::Counts )
        return;

    for( const Case* const expected_case : expected_cases )
    {
        const auto& lookup = std::find_if(actual_cases.cbegin(), actual_cases.cend(),
                                         [&](const Case* const actual_case) { return ( actual_case->GetUuid() == expected_case->GetUuid() ); });
        Assert::IsTrue(lookup != actual_cases.end());

        const Case* const actual_case = *lookup;

        if( compare_cases_type == CompareCasesType::CountsAndFullCase )
        {
            // compare without comparing the vector clocks
            Assert::IsTrue(expected_case->Equals(*actual_case, false));
            continue;
        }

        ASSERT(compare_cases_type == CompareCasesType::CountsAndBinaryData);

        const BinaryCaseItem& expected_binary_case_item = *assert_cast<const BinaryCaseItem*>(expected_case->GetRootCaseLevel().GetCaseRecord(0).GetCaseItems()[1]);
        const BinaryCaseItem& actual_binary_case_item = *assert_cast<const BinaryCaseItem*>(actual_case->GetRootCaseLevel().GetCaseRecord(0).GetCaseItems()[1]);

        const CaseItemIndex expected_index = expected_case->GetRootCaseLevel().GetCaseRecord(0).GetCaseItemIndex(0);
        const CaseItemIndex actual_index = actual_case->GetRootCaseLevel().GetCaseRecord(0).GetCaseItemIndex(0);

        Assert::AreEqual(expected_binary_case_item.IsBlank(expected_index), actual_binary_case_item.IsBlank(actual_index));

        if( expected_binary_case_item.IsBlank(expected_index) )
            continue;

        Assert::AreEqual(expected_binary_case_item.GetBinaryDataAccessor(expected_index).GetSignature(),
                         actual_binary_case_item.GetBinaryDataAccessor(actual_index).GetSignature());

        const BinaryData* const expected_binary_data = actual_binary_case_item.GetBinaryData_noexcept(expected_index);
        const BinaryData* const actual_binary_data = actual_binary_case_item.GetBinaryData_noexcept(actual_index);

        if( expected_binary_data != nullptr && actual_binary_data != nullptr )
        {
            Assert::AreEqual(expected_binary_data->GetContent().size(), actual_binary_data->GetContent().size());
            Assert::AreEqual(0, memcmp(expected_binary_data->GetContent().data(), actual_binary_data->GetContent().data(), expected_binary_data->GetContent().size()));
        }
    }
}


void MergeCaseList(std::vector<std::shared_ptr<Case>>& cases, const std::vector<std::shared_ptr<Case>>& cases_to_merge)
{
    for( const std::shared_ptr<Case>& case_to_merge : cases_to_merge )
    {
        auto lookup = std::find_if(cases.begin(), cases.end(),
                                   [&](const std::shared_ptr<Case>& data_case) { return ( data_case->GetUuid() == case_to_merge->GetUuid() ); });

        if( lookup == cases.end() )
        {
            cases.emplace_back(case_to_merge);
        }

        else
        {
            *lookup = case_to_merge;
        }
    }
}


std::string getCaseData(const Case& data_case)
{
    const CaseRecord& data_record = data_case.GetRootCaseLevel().GetCaseRecord(0);
    const CaseItemIndex index = data_record.GetCaseItemIndex();
    std::string value = assert_cast<const StringCaseItem*>(data_record.GetCaseItems().front())->GetValue(index);
    return SO::MakeTrimRight(value);
}
