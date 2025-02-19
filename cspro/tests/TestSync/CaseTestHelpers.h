#pragma once

#include <rxcpp/rx-lite.hpp>
#include <zDataO/SyncCaseSerializer.h>
#include <zSyncO/SyncExceptionRethrower.h>

class Case;
class CaseAccess;
class CDataDict;
class JsonConverter;
class Note;
enum class PartialSaveMode;
class SyncBinaryDataUploadManager;


// returns a new device ID that is unique so we can test sync on an existing server as if we started with a new device
DeviceId MakeUniqueDeviceId(std::string prefix);

std::string GetTestFilesDirectory();

std::unique_ptr<CDataDict> CreateTestDictionary();
std::unique_ptr<CDataDict> CreateTestDictionaryWithUniqueName();

std::shared_ptr<const std::vector<std::byte>> GetHtmlImage(const std::string& filename);

std::unique_ptr<Case> CreateCase(const CaseAccess& case_access, std::string uuid, int numeric_case_id,
                                 const std::vector<std::string>& string_values, bool add_data_to_image_item = true);

Note createNote(const CaseAccess& case_access, std::string content,
                const std::string& field_name, int rec_occ, int item_occ, int sub_occ,
                std::string opid, int64_t mod_time);

void addItemNote(const CaseAccess& case_access, Case& data_case, std::string content,
                 const std::string& field_name, int rec_occ, int item_occ, int sub_occ,
                 std::string opid, int64_t mod_time);

void SetPartialSave(const CaseAccess& case_access, Case& data_case, PartialSaveMode mode, const std::string& field_name,
                    size_t record_occurrence = 0, size_t item_occurrence = 0, size_t subitem_occurrence = 0);

std::unique_ptr<SyncBinaryDataUploadManager> CreateSyncBinaryDataUploadManager(std::shared_ptr<const CaseAccess> case_access,
                                                                               cs::span<const Case* const> cases_to_analyze = cs::span<const Case* const>());

std::string GetSyncableCaseData(std::shared_ptr<const CaseAccess> case_access, cs::span<const Case* const> cases, SyncCaseSerializer& sync_case_serializer);
std::string GetSyncableCaseData(std::shared_ptr<const CaseAccess> case_access, cs::span<const Case* const> cases, SyncCaseSerializer::Version version);

enum class CompareCasesType { Counts, CountsAndBinaryData, CountsAndFullCase };
void CompareCases(cs::span<const Case* const> expected_cases, cs::span<const Case* const> actual_cases, CompareCasesType compare_cases_type);

void MergeCaseList(std::vector<std::shared_ptr<Case>>& cases, const std::vector<std::shared_ptr<Case>>& cases_to_merge);

std::string getCaseData(const Case& data_case);


template <typename T>
std::vector<T> ObservableToVector(rxcpp::observable<T> observable)
{
    std::vector<T> values;
    std::exception_ptr caught_exception;

    observable.subscribe(
        [&](T value)
        {
            values.emplace_back(std::move(value));
        },
        [&](std::exception_ptr exception)
        {
            caught_exception = exception;
        });

    if( caught_exception )
        RethrowAsSyncError(caught_exception);

    return values;
}
