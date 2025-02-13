#pragma once

#include <zReformatO/zReformatO.h>
#include <zDictO/DictionaryComparer.h>
#include <zDataO/DataRepository.h>


class ZREFORMATO_API Reformatter : public DictionaryComparer
{
public:
    Reformatter(std::shared_ptr<const CDataDict> initial_dictionary, std::shared_ptr<const CDataDict> final_dictionary);

    bool RequiresReformat(DataRepositoryType data_repository_type) const;

    std::tuple<std::unique_ptr<CaseAccess>, std::shared_ptr<CaseAccess>> Initialize();

    void ReformatCase(const Case& initial_case, Case& final_case);

private:
    static void ReformatCaseItem(const CaseItem& initial_case_item, const CaseItemIndex& initial_index,
                                 const CaseItem& final_case_item, CaseItemIndex& final_index);

private:
    std::shared_ptr<const CDataDict> m_initialDictionary;
    std::shared_ptr<const CDataDict> m_finalDictionary;

    std::shared_ptr<CaseAccess> m_finalCaseAccess;

    struct CaseItemPair
    {
        const CaseItem& initial_case_item;
        size_t initial_record_number;
        const CaseItem& final_case_item;
        size_t final_record_number;
    };

    std::vector<std::vector<std::shared_ptr<const CaseItemPair>>> m_caseItemPairsByLevel;
};
