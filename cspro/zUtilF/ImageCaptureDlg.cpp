#include "StdAfx.h"
#include "ImageCaptureDlg.h"


ImageCaptureDlg::ImageCaptureDlg(const ImageCaptureType image_capture_type, const std::optional<std::wstring>& message, const std::optional<std::wstring>& image_localhost_url)
    :   m_dialogName(( image_capture_type == ImageCaptureType::Signature ) ? _T("Image-captureSignature") : _T("Image-takePhoto"))
{
    // create the JSON arguments text
    auto json_writer = Json::CreateStringWriter(m_jsonArgumentsText);

    json_writer->BeginObject();

    json_writer->WriteIfHasValue(JK::message, message);
    json_writer->WriteIfHasValue(JK::url, image_localhost_url);

    json_writer->EndObject();
}


const TCHAR* ImageCaptureDlg::GetDialogName()
{
    return m_dialogName;
}


std::wstring ImageCaptureDlg::GetJsonArgumentsText()
{
    return m_jsonArgumentsText;
}


void ImageCaptureDlg::ProcessJsonResults(const JsonNode<wchar_t>& json_results)
{
    m_imageDataUrl = json_results.Get<std::wstring>(JK::url);
}
