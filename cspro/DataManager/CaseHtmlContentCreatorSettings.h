#pragma once

#include <zCaseO/CaseToHtmlConverter.h>

class CaseHtmlContentCreator;


class CaseHtmlContentCreatorSettings : public CaseToHtmlConverter
{
public:
    CaseHtmlContentCreatorSettings();

    void SetCaseHtmlContentCreator(CaseHtmlContentCreator& creator) { m_creator = &creator; }

    bool ProcessViewOptionsMenu(const std::variant<UINT, CCmdUI*>& data);

    static CaseHtmlContentCreatorSettings CreateFromJson(const JsonNode& json_node);
    void WriteJson(JsonWriter& json_writer) const;

protected:
    const std::vector<std::string>* GetCaseConstructionErrors() const override;

    bool EmbedResources() const override;
    std::string CreateFileUrl(const std::string& file_path) override;

    bool AddBinaryDataOpenAndSaveUrls() const override;
    std::tuple<std::string, std::string> CreateBinaryDataOpenAndSaveUrls(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index,
                                                                         const std::string& suggested_filename) override;

    std::string CreateBinaryDataUrl(const BinaryCaseItem& binary_case_item, const CaseItemIndex& index,
                                    const std::string& mime_type, const std::string& suggested_filename) override;

private:
    CaseItemPrinter::Format m_caseItemPrinterFormat;
    CaseHtmlContentCreator* m_creator;
};
