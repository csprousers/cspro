#include "StdAfx.h"
#include "Viewers.h"
#include <zToolsO/ExceptionHolder.h>
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


Viewer& Viewer::UseExceptionHolder(std::shared_ptr<ExceptionHolder> exception_holder)
{
    ASSERT(m_data.exception_holder == nullptr);
    m_data.exception_holder = ( exception_holder != nullptr ) ? std::move(exception_holder) :
                                                                std::make_shared<ExceptionHolder>();
    return *this;
}


Viewer& Viewer::SetAccessInvokerAccessTokenOverride(std::string action_invoker_access_token_override)
{
    ASSERT(m_data.action_invoker_access_token_override == nullptr);
    m_data.action_invoker_access_token_override = std::make_unique<std::string>(std::move(action_invoker_access_token_override));
    return *this;
}


Viewer& Viewer::SetOptions(ViewerOptions options)
{
    // options should be set before SetTitle is called
    ASSERT(!m_options.title.IsSet());

    m_options = std::move(options);

    return *this;
}


Viewer& Viewer::SetTitle(SharableString title)
{
    m_options.title = std::move(title);
    return *this;
}


bool Viewer::ViewFile(const std::string& file_path)
{
    if( !PortableFunctions::FileIsRegular(file_path) )
        return false;

    m_data.content_type = Data::Type::FilePath;
    m_data.content = file_path;

    if( m_data.use_embedded_viewers )
    {
        if( FileExtensions::IsFileHtml(file_path) )
        {
            if( m_data.use_shared_html_local_file_server )
            {
                m_data.local_file_server_root_directory = PortableFunctions::PathGetDirectory(file_path);
            }

            // if not using a local file server, view the file using its file URL
            else
            {
                m_data.content_type = Data::Type::HtmlUrl;
                m_data.content = Encoders::ToFileUrl(file_path);
            }
        }
    }

    return View();
}


bool Viewer::ViewFileInEmbeddedBrowser(std::string file_path)
{
    ASSERT(m_data.use_embedded_viewers && m_data.use_shared_html_local_file_server);

    if( !PortableFunctions::FileIsRegular(file_path) )
        return false;

    m_data.content_type = Data::Type::FilePath;
    m_data.content = std::move(file_path);
    m_data.local_file_server_root_directory = PortableFunctions::PathGetDirectory(m_data.content);

    return View();
}


bool Viewer::ViewHtmlUrl(const std::string& url)
{
    // check if this URL is a file URL, and if so, view it as a file
    const std::optional<std::string> file_path = Encoders::FromFileUrl(url);

    if( file_path.has_value() )
    {
        return ViewFile(*file_path);
    }

    else
    {
        m_data.content_type = Data::Type::HtmlUrl;
        m_data.content = url;

        return View();
    }
}


bool Viewer::ViewHtmlContent(SharableString html, const std::string& local_file_server_root_directory/* = std::string()*/)
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


bool Viewer::View()
{
    std::unique_ptr<ActionInvoker::ListenerHolder> on_get_input_data_listener;

    // set up an Action Invoker listener for the UI.getInputData action
    if( m_options.action_invoker_ui_get_input_data.IsSet() )
    {
        on_get_input_data_listener = ActionInvoker::ListenerHolder::Create<ActionInvoker::OnGetInputDataListener>(
            [&]()
            {
                return m_options.action_invoker_ui_get_input_data;
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
        const wchar_t* operation = ( m_data.content_type == Data::Type::HtmlUrl ) ? L"open" : nullptr;
        HINSTANCE result = ShellExecute(nullptr, operation, TC::ToWide(m_data.content).c_str(), nullptr, nullptr, SW_SHOWNORMAL);

        if( reinterpret_cast<INT_PTR>(result) >= 32 )
            return true;
    }
#endif

    return false;
}
