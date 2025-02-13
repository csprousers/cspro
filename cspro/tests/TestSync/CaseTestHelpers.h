#pragma once

#include <zToolsO/PortableFunctions.h>
#include <zUtilO/BinaryDataMetadata.h>
#include <zDictO/DDClass.h>
#include <zCaseO/BinaryCaseItem.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseAccess.h>
#include <zCaseO/CaseItemReference.h>
#include <zCaseO/NumericCaseItem.h>
#include <zCaseO/StringCaseItem.h>
#include <zSyncO/JsonConverter.h>
#include <rxcpp/rx-lite.hpp>
#include <fstream>


inline std::unique_ptr<CDataDict> createTestDict()
{
    auto dictionary = std::make_unique<CDataDict>();
    try {
        dictionary->Open(L"TestDict.dcf", true);
    }
    catch( const CSProException& ) {
        ASSERT(false);
    }
    return dictionary;
}

inline CString makeUuid()
{
    UUID uuid;
    UuidCreate(&uuid);
    WCHAR* str;
    UuidToStringW(&uuid, (RPC_WSTR*)&str);
    CString guid(str);
    RpcStringFreeW((RPC_WSTR*)&str);
    return guid;
}

inline std::shared_ptr<Case> makeCase(const CaseAccess& case_access, const CString& uuid, int caseid, const std::vector<CString>& data, bool bUseBinary = true)
{
    auto data_case = std::make_shared<Case>(case_access.GetCaseMetadata());
    data_case->SetUuid(CS2WS(uuid));
    auto& id_record = data_case->GetRootCaseLevel().GetIdCaseRecord();
    auto index = id_record.GetCaseItemIndex();
    static_cast<const NumericCaseItem*>(id_record.GetCaseItems().front())->SetValue(index, caseid);
    auto& data_record = data_case->GetRootCaseLevel().GetCaseRecord(0);
    data_record.SetNumberOccurrences(data.size());

    //read image content for setting the binary item
    std::ifstream fileStream(L"cspro-logo.png", std::ios::binary);
    const std::streampos current = fileStream.tellg();
    fileStream.seekg(0, std::ios::end);
    const std::streampos size = fileStream.tellg();
    fileStream.seekg(current, std::ios::beg);


    std::shared_ptr<std::vector<std::byte>> imageContent = std::make_shared<std::vector<std::byte>>(size);
    fileStream.read(reinterpret_cast<char*>(imageContent.get()->data()), size);


    for (size_t i = 0; i < data.size(); ++i) {
        auto rindex = data_record.GetCaseItemIndex(i);

        //The first item in the test record is a string
        static_cast<const StringCaseItem*>(data_record.GetCaseItems().front())->SetValue(rindex, data[i]);

        if (bUseBinary) {
            //the second item in the test record is a binary item. Set its content
            const BinaryCaseItem* binary_case_item = dynamic_cast<const BinaryCaseItem*>(data_record.GetCaseItems()[1]);
            BinaryDataMetadata metadata;
            metadata.SetFilename(_T("CSPro Logo.png"));
            metadata.SetBinaryDataKey(PortableFunctions::BinaryMd5(imageContent->data(), static_cast<size_t>(size)));
            metadata.SetProperty(FormatTextCS2WS(_T("Property%d"), i), FormatTextCS2WS(_T("Value%d"), i));
            binary_case_item->GetBinaryDataAccessor(rindex).SetBinaryData(imageContent, std::move(metadata));
        }
    }
    return data_case;
}

inline Note createNote(const CaseAccess& case_access, const CString& content,
    const CString& field_name, int rec_occ, int item_occ, int sub_occ,
    const CString& opid, time_t mod_time)
{
    auto item = case_access.LookupCaseItem(field_name);
    auto item_ref = std::make_shared<CaseItemReference>(*item, L"", rec_occ, item_occ, sub_occ);
    return Note(content, item_ref, opid, mod_time);
}

inline void addItemNote(const CaseAccess& case_access, Case& data_case, const CString& content,
    const CString& field_name, int rec_occ, int item_occ, int sub_occ,
    const CString& opid, time_t mod_time)
{
    auto notes = data_case.GetNotes();
    notes.emplace_back(createNote(case_access, content, field_name, rec_occ, item_occ, sub_occ, opid, mod_time));
    data_case.SetNotes(notes);
}

inline void setPartialSave(const CaseAccess& case_access, Case& data_case, PartialSaveMode mode,
    const CString& field_name, int rec_occ, int item_occ, int sub_occ)
{
    auto item = case_access.LookupCaseItem(field_name);
    auto item_ref = std::make_shared<CaseItemReference>(*item, L"", rec_occ, item_occ, sub_occ);
    data_case.SetPartialSaveStatus(mode, item_ref);
}

inline std::string convertCasesToJSON(JsonConverter& jsonConverter, std::vector<std::shared_ptr<Case>> caseList)
{
    std::vector<std::pair<const BinaryCaseItem*, CaseItemIndex>> binaryCaseItems;
    if (caseList.size() > 0) {
        //hardcoded to add the binary item created by makecase
        const BinaryCaseItem* servercase_binary_case_item = dynamic_cast<const BinaryCaseItem*>(caseList[0]->GetRootCaseLevel().GetCaseRecord(0).GetCaseItems()[1]);
        auto servercase_binary_case_item_index = caseList[0]->GetRootCaseLevel().GetCaseRecord(0).GetCaseItemIndex(0);
        std::pair<const BinaryCaseItem*, CaseItemIndex> binary = std::make_pair(servercase_binary_case_item, servercase_binary_case_item_index);
        binaryCaseItems.push_back(binary);
    }
    std::ostringstream oss;
    const std::string casesJSON = jsonConverter.toJson(caseList);
    jsonConverter.writeBinaryJson(oss, casesJSON, binaryCaseItems);
    return oss.str();
}
inline std::vector<std::shared_ptr<Case>> getCaseListFromJson(JsonConverter& jsonConverter, std::string_view json,
    const CaseAccess& case_access)
{
    // read if binary content and process cases
    std::istringstream iss;
    std::string casesJson ;
    bool isBinaryFormat = false;
    std::vector<std::shared_ptr<Case>> cases;
    if (jsonConverter.IsBinaryJson(json)) {
        //read binary cases json
        isBinaryFormat = true;
        iss.str(std::string(json));
        casesJson = jsonConverter.readCasesJsonFromBinary(iss);
    }
    else {
        casesJson = json;
    }
    cases = jsonConverter.caseListFromJson(casesJson, case_access);

    if (isBinaryFormat) {//read binary items into cases
        jsonConverter.readBinaryCaseItems(iss, cases);
    }

    return cases;
}


inline void checkCaseList(const std::vector<std::shared_ptr<Case>>& expected, const std::vector<std::shared_ptr<Case>>& actual, bool bUseBinary = true)
{
    if (bUseBinary) {
        bool foundFirstMatch = false;
        for (const auto& actual_case : actual) {
            auto match = std::find_if(expected.begin(), expected.end(), [&actual_case](const std::shared_ptr<Case>& i) {return actual_case->GetUuid() == i->GetUuid(); });
            Assert::IsTrue(match != expected.end());

            const BinaryCaseItem* actual_binary_case_item = dynamic_cast<const BinaryCaseItem*>(actual_case->GetRootCaseLevel().GetCaseRecord(0).GetCaseItems()[1]);
            const BinaryCaseItem* match_binary_case_item = dynamic_cast<const BinaryCaseItem*>((*match)->GetRootCaseLevel().GetCaseRecord(0).GetCaseItems()[1]);

            CaseItemIndex actualBinaryCaseItemIndex = actual_case->GetRootCaseLevel().GetCaseRecord(0).GetCaseItemIndex(0);
            CaseItemIndex matchBinaryCaseItemIndex = (*match)->GetRootCaseLevel().GetCaseRecord(0).GetCaseItemIndex(0);
            Assert::IsTrue(actual_binary_case_item->GetBinaryDataMetadata_noexcept(actualBinaryCaseItemIndex)->GetBinaryDataKey() ==
                match_binary_case_item->GetBinaryDataMetadata_noexcept(matchBinaryCaseItemIndex)->GetBinaryDataKey());

            const BinaryData* actualBinaryData = actual_binary_case_item->GetBinaryData_noexcept(actualBinaryCaseItemIndex);
            const BinaryData* matchBinaryData = match_binary_case_item->GetBinaryData_noexcept(matchBinaryCaseItemIndex);
            if (actualBinaryData != nullptr && matchBinaryData != nullptr && !foundFirstMatch) {
                //as duplicate binary content is not sent, the subsequent cases are not populated with binary content from read
                //so this check is valid only for the first case
                Assert::IsTrue(!actualBinaryData->GetContent().empty() &&
                    actualBinaryData->GetContent().size() == matchBinaryData->GetContent().size() &&
                    memcmp(actualBinaryData->GetContent().data(), matchBinaryData->GetContent().data(), actualBinaryData->GetContent().size()) == 0);
                foundFirstMatch = true;
            }

        }
    }
    Assert::AreEqual(expected.size(), actual.size());
}

inline void mergeCaseList(std::vector<std::shared_ptr<Case>>& initial, const std::vector<std::shared_ptr<Case>>& toAdd)
{
    for (auto c : toAdd) {
        auto match = std::find_if(initial.begin(), initial.end(), [&c](const std::shared_ptr<Case>& i) {return c->GetUuid() == i->GetUuid(); });
        if (match == initial.end())
            initial.push_back(c);
        else
            *match = c;
    }
}

inline CString getCaseData(const Case& data_case)
{
    auto& data_record = data_case.GetRootCaseLevel().GetCaseRecord(0);
    auto index = data_record.GetCaseItemIndex(0);
    CString data = static_cast<const StringCaseItem*>(data_record.GetCaseItems().front())->GetValue(index);
    return data.TrimRight();
}

template <typename T>
std::vector<T> ObservableToVector(rxcpp::observable<T> observable)
{
    std::vector<T> vec;
    observable.subscribe(
        [&vec](T t) { vec.emplace_back(t); },
        [](std::exception_ptr eptr) {
            std::rethrow_exception(eptr);
        });
    return vec;
}
