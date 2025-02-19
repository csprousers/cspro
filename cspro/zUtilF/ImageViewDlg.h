#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>
#include <zHtml/VirtualFileMapping.h>

struct ViewerOptions;


class CLASS_DECL_ZUTILF ImageViewDlg : public CSHtmlDlgRunner
{
public:
    ImageViewDlg(const std::vector<std::byte>& image_content, std::string image_content_type,
                 const std::optional<std::tuple<int, int>>& image_width_and_height, const std::string& image_file_path);

    void ShowDialogUsingViewer(ViewerOptions viewer_options);

protected:
    std::string GetDialogName() override;
    SharableString GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode& json_results) override;

private:
    DataVirtualFileMappingHandler<const std::vector<std::byte>&> m_virtualFileMappingHandlerToImage;
    SharableString m_jsonArgumentsText;
};
