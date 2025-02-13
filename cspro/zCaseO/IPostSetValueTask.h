#pragma once

class CaseItem;
class CaseItemIndex;


class IPostSetValueTask
{
public:
    virtual ~IPostSetValueTask() { }

    virtual void Do(const CaseItem& modified_case_item, CaseItemIndex& index) = 0;
};
