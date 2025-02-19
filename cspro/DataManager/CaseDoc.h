#pragma once

#include <DataManager/CaseHoldingDoc.h>


class CaseDoc : public CaseHoldingDoc
{
    DECLARE_DYNCREATE(CaseDoc)

protected:
    CaseDoc() { }

public:
    bool IsDocumentCaseOnly() const override { return true; }

    UINT GetDefaultPageCommandId() override;

protected:
    BOOL OnNewDocument() override;

private:
    UINT m_initialPageCommandId;
};
