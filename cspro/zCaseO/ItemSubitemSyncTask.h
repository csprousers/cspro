#pragma once

#include <zCaseO/IPostSetValueTask.h>


class ItemSubitemSyncTask : public IPostSetValueTask
{
public:
    ItemSubitemSyncTask(CaseItem& parent_case_item);

    void AddSubitem(CaseItem& subitem_case_item);

    void Do(const CaseItem& modified_case_item, CaseItemIndex& index) override;

private:
    CaseItem& m_parentCaseItem;
    std::vector<CaseItem*> m_subitemCaseItems;
};
