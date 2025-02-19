#pragma once

#include <DataManager/CaseContentCreatorBase.h>

struct CaseTextContentCreatorSettings;


class CaseTextContentCreator : public CaseContentCreatorBase
{
public:
    CaseTextContentCreator(CaseHoldingDoc& case_holding_doc);
    ~CaseTextContentCreator();

    UINT GetCommandId() const override { return ID_VIEW_CASE_TEXT; }

    const wchar_t* GetSaveTitle() const override;
    std::vector<const char*> GetSaveFormats() const override;

    UINT GetViewOptionsMenuResourceId() const override;
    bool ProcessViewOptionsMenu(std::variant<UINT, CCmdUI*> data) override;

protected:
    SharableString GetTextContentWorker() override;
    SharableString GetHtmlContentWorker(bool embed_resources) override;

private:
    std::string GetCaseText() const;

private:
    std::shared_ptr<CaseTextContentCreatorSettings> m_caseTextContentCreatorSettings;
    std::unique_ptr<TextToCaseConverter> m_textToCaseConverter;

    struct Formatter;
    std::unique_ptr<Formatter> m_formatter;
};
