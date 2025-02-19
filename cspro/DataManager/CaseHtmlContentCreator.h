#pragma once

#include <DataManager/CaseContentCreatorBase.h>


class CaseHtmlContentCreator : public CaseContentCreatorBase
{
    friend class CaseHtmlContentCreatorSettings;

public:
    CaseHtmlContentCreator(CaseHoldingDoc& case_holding_doc);

    UINT GetCommandId() const override { return ID_VIEW_CASE_HTML; }

    const wchar_t* GetSaveTitle() const override;

    UINT GetViewOptionsMenuResourceId() const override;
    bool ProcessViewOptionsMenu(std::variant<UINT, CCmdUI*> data) override;

    void ProcessWebViewMessage(const JsonNode& json_node) override;

protected:
    SharableString GetHtmlContentWorker(bool embed_resources) override;

private:
    std::shared_ptr<const std::vector<std::byte>> GetBinaryData(std::string_view access_key_sv);

    const std::string& CreateBinaryDataHandler(const std::string& access_key, const std::string& mime_type, const std::string& suggested_filename);

private:
    std::shared_ptr<CaseHtmlContentCreatorSettings> m_caseHtmlContentCreatorSettings;
    bool m_embedResources;
    double m_lastProcessedDataCasePositionInRepository;
    std::map<std::string, std::unique_ptr<VirtualFileMappingHandler>> m_binaryDataVirtualFileMappingHandlers;
};
