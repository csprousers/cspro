#pragma once

#include <zDiffO/zDiffO.h>
#include <zCaseO/CaseKey.h>
#include <zDataO/DataRepositoryDefines.h>

class CaseItemPrinter;
class CDataDict;
class CDictRecord;
class DiffSpec;
class PFF;
namespace FileIO { class TextFile; }


class ZDIFFO_API Differ
{
public:
    Differ(std::shared_ptr<DiffSpec> diff_spec = nullptr);
    virtual ~Differ();

    bool Run(const PFF& pff, bool silent, std::shared_ptr<const CDataDict> embedded_dictionary = nullptr);

protected:
    virtual void HandleNoDifferences(const PFF& pff);

private:
    struct CaseCompareData;
    struct RunData;

    struct UuidMatchingData
    {
        CaseKey case_key;
        std::string uuid; // UUID filled in on demand

        UuidMatchingData(const CaseKey& case_key_) : case_key(case_key_) {}
    };

    template<bool UseUuidMatching>
    using MatchIdentifier = std::conditional_t<UseUuidMatching, UuidMatchingData, std::string>;

    void InitializeComparison();

    void Run(const ConnectionString& input_connection_string, const ConnectionString& output_connection_string);

    template<bool UseUuidMatching>
    void RunCompare(RunData& rd);

    static void CheckAndUpdateProcessBar(RunData& rd, const std::string& key, size_t counts);

    template<typename T>
    static const std::string& GetCaseKey(const T& identifier);

    template<bool UseUuidMatching>
    std::vector<MatchIdentifier<UseUuidMatching>> GetIdentifiers(
        RunData& rd,
        DataRepository& repository,
        CaseIterationMethod iteration_method) const;

    static bool IdentifiersContainDuplicates(
        const std::vector<MatchIdentifier<true>>& input_identifiers,
        const std::vector<MatchIdentifier<true>>& reference_identifiers);

    std::vector<Differ::MatchIdentifier<true>>::const_iterator MatchCaseByUuid(
        RunData& rd,
        std::vector<MatchIdentifier<true>>& input_identifiers,
        std::vector<MatchIdentifier<true>>& reference_identifiers);

    void CompareCase(const CaseCompareData& ccd);

    std::string CompareLevel(const CaseLevel& input_case_level, const CaseLevel& reference_case_level);

private:
    std::shared_ptr<DiffSpec> m_diffSpec;

    std::unique_ptr<FileIO::TextFile> m_log;

    std::unique_ptr<CaseItemPrinter> m_caseItemPrinter;
    std::shared_ptr<CaseAccess> m_caseAccess;
    std::map<const CDictRecord*, std::vector<std::tuple<const CaseItem*, size_t>>> m_recordsCaseItemsMap;

    bool m_differencesExist;
};
