#pragma once

#include <DataManager/CaseContentCreatorBase.h>

class CaseJsonWriterSerializerHelper;


class CaseJsonContentCreator : public CaseContentCreatorBase
{
public:
    CaseJsonContentCreator(CaseHoldingDoc& case_holding_doc);
    ~CaseJsonContentCreator();

    UINT GetCommandId() const override { return ID_VIEW_CASE_JSON; }

    const wchar_t* GetSaveTitle() const override;
    std::vector<const char*> GetSaveFormats() const override;

    UINT GetViewOptionsMenuResourceId() const override;
    bool ProcessViewOptionsMenu(std::variant<UINT, CCmdUI*> data) override;

protected:
    SharableString GetTextContentWorker() override;
    SharableString GetHtmlContentWorker(bool embed_resources) override;

private:
    std::shared_ptr<CaseJsonContentCreatorSettings> m_caseJsonContentCreatorSettings;
    std::shared_ptr<CaseJsonWriterSerializerHelper> m_caseJsonWriterSerializerHelper;
};
