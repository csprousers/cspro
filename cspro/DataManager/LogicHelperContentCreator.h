#pragma once

#include <DataManager/ContentCreator.h>


class LogicHelperContentCreator : public ContentCreator
{
public:
    LogicHelperContentCreator(DataSourceDoc& data_source_doc);

    UINT GetCommandId() const override { return ID_VIEW_LOGIC_HELPER; }

    const wchar_t* GetSaveTitle() const override;
    std::vector<const char*> GetSaveFormats() const override;
    std::string GetSaveSuggestedFilename() const override;

    bool ContentChangesOnCaseListingSettingsChange() const override   { return true; }
    bool ContentChangesOnCaseListingSelectionsChange() const override { return true; }

    SharableString GetTextContent() override;
    SharableString GetHtmlContent(bool embed_resources = false) override;

private:
    DataSourceDoc& m_dataSourceDoc;
    std::shared_ptr<const Case> m_currentCase;
};
