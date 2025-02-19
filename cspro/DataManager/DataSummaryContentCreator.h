#pragma once

#include <DataManager/ContentCreator.h>


class DataSummaryContentCreator : public ContentCreator
{
public:
    DataSummaryContentCreator(DataSourceDoc& data_source_doc);

    UINT GetCommandId() const override { return ID_VIEW_DATA_SUMMARY; }

    const wchar_t* GetSaveTitle() const override;
    std::string GetSaveSuggestedFilename() const override;

    bool ContentChangesOnCaseListingSettingsChange() const override   { return false; }
    bool ContentChangesOnCaseListingSelectionsChange() const override { return false; }

    SharableString GetHtmlContent(bool embed_resources = false) override;

private:
    DataSourceDoc& m_dataSourceDoc;
};
