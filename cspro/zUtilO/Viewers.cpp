#include "StdAfx.h"
#include "Viewers.h"
#include <zToolsO/Encoders.h>
#include <zToolsO/PortableFunctions.h>
#include <zHtml/VirtualFileMapping.h>
#include <zAction/OnGetInputDataListener.h>
#include <zEngineF/EngineUI.h>


Viewer& Viewer::UseEmbeddedViewer()
{
    m_data.use_embedded_viewers = true;
    return *this;
}


Viewer& Viewer::UseSharedHtmlLocalFileServer()
{
    ASSERT(m_data.use_embedded_viewers);
    m_data.use_shared_html_local_file_server = true;
    return *this;
}


Viewer& Viewer::SetAccessInvokerAccessTokenOverride(std::wstring action_invoker_access_token_override)
{
    ASSERT(m_data.action_invoker_access_token_override == nullptr);
    m_data.action_invoker_access_token_override = std::make_unique<std::wstring>(std::move(action_invoker_access_token_override));
    return *this;
}


Viewer& Viewer::SetOptions(ViewerOptions options)
{
    // options should be set before SetTitle is called
    ASSERT(!m_options.title.has_value());

    m_options = std::move(options);

    return *this;
}


Viewer& Viewer::SetTitle(std::wstring title)
{
    m_options.title = std::move(title);
    return *this;
}


bool Viewer::ViewFile(const std::wstring& filename)
{
    if( !PortableFunctions::FileIsRegular(filename) )
        return false;

    m_data.content_type = Data::Type::Filename;
    m_data.content = filename;

    if( m_data.use_embedded_viewers )
    {
        if( FileExtensions::IsFilenameHtml(filename) )
        {
            if( m_data.use_shared_html_local_file_server )
            {
                m_data.local_file_server_root_directory = PortableFunctions::PathGetDirectory(filename);
            }

            // if not using a local file server, view the file using its file URL
            else
            {
                m_data.content_type = Data::Type::HtmlUrl;
                m_data.content = Encoders::ToFileUrl(filename);
            }
        }
    }

    return View();
}


bool Viewer::ViewFileInEmbeddedBrowser(std::wstring filename)
{
    ASSERT(m_data.use_embedded_viewers && m_data.use_shared_html_local_file_server);

    if( !PortableFunctions::FileIsRegular(filename) )
        return false;

    m_data.content_type = Data::Type::Filename;
    m_data.content = std::move(filename);
    m_data.local_file_server_root_directory = PortableFunctions::PathGetDirectory(m_data.content);

    return View();
}


bool Viewer::ViewHtmlUrl(const std::wstring& url)
{
    // check if this URL is a file URL, and if so, view it as a file
    std::optional<std::wstring> filename = Encoders::FromFileUrl(url);

    if( filename.has_value() )
    {
        return ViewFile(*filename);
    }

    else
    {
        m_data.content_type = Data::Type::HtmlUrl;
        m_data.content = url.c_str();

        return View();
    }
}


bool Viewer::ViewHtmlContent(std::string html, const std::wstring& local_file_server_root_directory/* = std::wstring()*/)
{
    ASSERT(m_data.use_embedded_viewers);
    ASSERT(m_data.use_shared_html_local_file_server || local_file_server_root_directory.empty());

    EngineUI::CreateVirtualFileMappingAroundViewHtmlContentNode node
    {
        std::move(html),
        local_file_server_root_directory
    };

    if( SendEngineUIMessage(EngineUI::Type::CreateVirtualFileMappingAroundViewHtmlContent, node) == 1 )
    {
        ASSERT(node.virtual_file_mapping != nullptr);

        m_data.content_type = Data::Type::HtmlUrl;
        m_data.content = node.virtual_file_mapping->GetUrl();

        return View();
    }

    return false;
}


bool Viewer::ViewHtmlContent(const wstring_view html_sv, const std::wstring& local_file_server_root_directory/* = std::wstring()*/)
{
    return ViewHtmlContent(UTF8Convert::WideToUTF8(html_sv), local_file_server_root_directory);
}


bool Viewer::View()
{
    std::unique_ptr<ActionInvoker::ListenerHolder> on_get_input_data_listener;

    // set up an Action Invoker listener for the UI.getInputData action
    if( m_options.action_invoker_ui_get_input_data != nullptr )
    {
        on_get_input_data_listener = ActionInvoker::ListenerHolder::Create<ActionInvoker::OnGetInputDataListener>(
            [&]()
            {
                return *m_options.action_invoker_ui_get_input_data;
            });
    }

    // use embedded views on Android or when explicity requested on Windows
    if( ( m_data.use_embedded_viewers || !OnWindowsDesktop() ) && SendEngineUIMessage(EngineUI::Type::View, *this) == 1 )
    {
        return true;
    }

#ifdef WIN_DESKTOP
    // on Windows we will try to open files and URLs in external viewers
    // if not using embedded viewers (or if they could not handle the viewing)
    else
    {
        const TCHAR* operation = ( m_data.content_type == Data::Type::HtmlUrl ) ? _T("open") : nullptr;
        HINSTANCE result = ShellExecute(nullptr, operation, m_data.content.c_str(), nullptr, nullptr, SW_SHOWNORMAL);

        if( reinterpret_cast<INT_PTR>(result) >= 32 )
            return true;
    }
#endif

    return false;
}
