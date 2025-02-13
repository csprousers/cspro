#include "StdAfx.h"
#include "ImageViewDlg.h"
#include <zHtml/PortableLocalhost.h>
#include <zUtilO/MimeType.h>
#include <zUtilO/Viewers.h>


ImageViewDlg::ImageViewDlg(const std::vector<std::byte>& image_content, wstring_view image_content_type_sv,
                           const std::optional<std::tuple<int, int>>& image_width_and_height, const std::wstring& image_filename)
    :   m_virtualFileMappingHandlerToImage(image_content, image_content_type_sv)
{
    const std::wstring image_filename_only = PortableFunctions::PathGetFilename(image_filename);
    PortableLocalhost::CreateVirtualFile(m_virtualFileMappingHandlerToImage, image_filename_only);

    // create the JSON arguments text
    auto json_writer = Json::CreateStringWriter(m_jsonArgumentsText);

    json_writer->BeginObject();

    json_writer->Write(JK::title, image_filename_only);
    json_writer->Write(JK::url, m_virtualFileMappingHandlerToImage.GetUrl());

    if( image_width_and_height.has_value() )
    {
        json_writer->Write(JK::width, std::get<0>(*image_width_and_height));
        json_writer->Write(JK::height, std::get<1>(*image_width_and_height));
    }

    json_writer->EndObject();
}


const TCHAR* ImageViewDlg::GetDialogName()
{
    return _T("Image-view");
}


std::wstring ImageViewDlg::GetJsonArgumentsText()
{
    return m_jsonArgumentsText;
}


void ImageViewDlg::ProcessJsonResults(const JsonNode<wchar_t>& /*json_results*/)
{
}


void ImageViewDlg::ShowDialogUsingViewer(ViewerOptions viewer_options)
{
    const NavigationAddress navigation_address = GetNavigationAddress();
    ASSERT(navigation_address.IsHtmlFilename());

    viewer_options.action_invoker_ui_get_input_data = &m_jsonArgumentsText;

    Viewer viewer;
    viewer.UseEmbeddedViewer()
          .UseSharedHtmlLocalFileServer()
          .SetOptions(std::move(viewer_options))
          .SetAccessInvokerAccessTokenOverride(RegisterActionInvokerAccessTokenOverride(navigation_address.GetHtmlFilename()))
          .ViewFileInEmbeddedBrowser(navigation_address.GetHtmlFilename());
}
