#pragma once

#include <DataManager/ContentCreator.h>


class CaseContentCreatorBase : public ContentCreator
{
public:
    CaseContentCreatorBase(CaseHoldingDoc& case_holding_doc);

    bool ContentChangesOnCaseListingSettingsChange() const override   { return false; }
    bool ContentChangesOnCaseListingSelectionsChange() const override { return true; }

    std::string GetSaveSuggestedFilename() const override;

    SharableString GetTextContent() override final;
    SharableString GetHtmlContent(bool embed_resources = false) override final;

protected:
    virtual SharableString GetTextContentWorker();
    virtual SharableString GetHtmlContentWorker(bool embed_resources);

protected:
    void UpdateCurrentCase();

protected:
    CaseHoldingDoc& m_caseHoldingDoc;
    std::shared_ptr<const Case> m_dataCase;
};
