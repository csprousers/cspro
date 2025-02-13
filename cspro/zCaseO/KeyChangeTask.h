#pragma once

#include <zCaseO/IPostSetValueTask.h>
#include <zCaseO/CaseLevel.h>
#include <zCaseO/CaseRecord.h>


class KeyChangeTask : public IPostSetValueTask
{
public:
    void Do(const CaseItem& /*modified_case_item*/, CaseItemIndex& index) override
    {
        index.GetCaseRecord().GetCaseLevel().RecalculateLevelIdentifier();
    }
};
