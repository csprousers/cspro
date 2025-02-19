#include "StdAfx.h"
#include "ImageCaptureDlg.h"


ImageCaptureDlg::ImageCaptureDlg(const ImageCaptureType image_capture_type, const SharableString& message, const SharableString& image_localhost_url)
    :   m_dialogName(( image_capture_type == ImageCaptureType::Signature ) ? "Image-captureSignature" : "Image-takePhoto")
{
    // create the JSON arguments text
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(m_jsonArgumentsText.MakeModifiable());

    json_writer->BeginObject();

    json_writer->WriteIfHasValue(JK::message, message);
    json_writer->WriteIfHasValue(JK::url, image_localhost_url);

    json_writer->EndObject();
}


std::string ImageCaptureDlg::GetDialogName()
{
    return m_dialogName;
}


SharableString ImageCaptureDlg::GetJsonArgumentsText()
{
    return m_jsonArgumentsText;
}


void ImageCaptureDlg::ProcessJsonResults(const JsonNode& json_results)
{
    m_imageDataUrl = json_results.Get<std::string>(JK::url);
}
