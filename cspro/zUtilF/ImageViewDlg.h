#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>
#include <zHtml/VirtualFileMapping.h>

struct ViewerOptions;


class CLASS_DECL_ZUTILF ImageViewDlg : public CSHtmlDlgRunner
{
public:
    ImageViewDlg(const std::vector<std::byte>& image_content, wstring_view image_content_type_sv,
                 const std::optional<std::tuple<int, int>>& image_width_and_height, const std::wstring& image_filename);

    void ShowDialogUsingViewer(ViewerOptions viewer_options);

protected:
    const TCHAR* GetDialogName() override;
    std::wstring GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode<wchar_t>& json_results) override;

private:
    DataVirtualFileMappingHandler<const std::vector<std::byte>&> m_virtualFileMappingHandlerToImage;
    std::wstring m_jsonArgumentsText;
};
