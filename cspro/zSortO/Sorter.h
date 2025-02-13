#pragma once

#include <zSortO/zSortO.h>

class CaseAccess;
class CDataDict;
class CDictRecord;
class CStdioFileUnicode;
class DataRepository;
class PFF;
class SortableKeyDatabase;
struct SortCaseItem;
class SortSpec;


class ZSORTO_API Sorter
{
public:
    enum class RunSuccess { Success, SuccessWithStructuralErrors, UserCanceled, Errors };

public:
    Sorter(std::shared_ptr<SortSpec> sort_spec = nullptr);
    ~Sorter();

    RunSuccess Run(const PFF& pff, bool silent, std::shared_ptr<const CDataDict> embedded_dictionary = nullptr);

private:
    RunSuccess Run(const PFF& pff);

    void InitializeCaseAccess(const CDataDict& dictionary);

    RunSuccess RunCaseSort();

    void CreateFlattenedRecordSortDictionary();

    RunSuccess RunRecordSort();

private:
    std::shared_ptr<SortSpec> m_sortSpec;

    std::unique_ptr<CStdioFileUnicode> m_log;

    std::shared_ptr<CaseAccess> m_firstPassCaseAccess;
    std::shared_ptr<CaseAccess> m_caseAccess;
    std::unique_ptr<DataRepository> m_inputRepository;
    std::unique_ptr<DataRepository> m_sortedRepository;

    std::unique_ptr<CDataDict> m_flattenedRecordSortDictionary;
    std::set<const CDictRecord*> m_requiredFlattenedDictRecords;

    std::vector<std::unique_ptr<SortCaseItem>> m_sortCaseItems;

    std::unique_ptr<SortableKeyDatabase> m_sortableKeyDatabase;
};
