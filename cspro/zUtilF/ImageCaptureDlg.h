#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF ImageCaptureDlg : public CSHtmlDlgRunner
{
public:
    enum class ImageCaptureType { Signature, Photo };

    ImageCaptureDlg(ImageCaptureType image_capture_type, const std::optional<std::wstring>& message, const std::optional<std::wstring>& image_localhost_url);

    const std::wstring& GetImageDataUrl() const { return m_imageDataUrl; }

protected:
    const TCHAR* GetDialogName() override;
    std::wstring GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode<wchar_t>& json_results) override;

private:
    const TCHAR* const m_dialogName;
    std::wstring m_jsonArgumentsText;

    std::wstring m_imageDataUrl;
};
