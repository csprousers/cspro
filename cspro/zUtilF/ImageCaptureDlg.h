#pragma once

#include <zUtilF/zUtilF.h>
#include <zHtml/CSHtmlDlgRunner.h>


class CLASS_DECL_ZUTILF ImageCaptureDlg : public CSHtmlDlgRunner
{
public:
    enum class ImageCaptureType { Signature, Photo };

    ImageCaptureDlg(ImageCaptureType image_capture_type, const SharableString& message, const SharableString& image_localhost_url);

    const std::string& GetImageDataUrl() const { return m_imageDataUrl; }

protected:
    std::string GetDialogName() override;
    SharableString GetJsonArgumentsText() override;
    void ProcessJsonResults(const JsonNode& json_results) override;

private:
    const char* const m_dialogName;
    SharableString m_jsonArgumentsText;

    std::string m_imageDataUrl;
};
