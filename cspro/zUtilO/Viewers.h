#pragma once

#include <zUtilO/zUtilO.h>
#include <zToolsO/PointerClasses.h>

template<typename CharType> class JsonNode;


// additional options that can be used by viewers
struct ViewerOptions
{
    std::optional<CSize> requested_size;
    std::optional<std::wstring> title;
    std::optional<bool> show_close_button;
    std::shared_ptr<const JsonNode<wchar_t>> display_options_node;
    cs::shared_or_raw_ptr<const std::wstring> action_invoker_ui_get_input_data;
};


class CLASS_DECL_ZUTILO Viewer
{
public:
    // display the content in an embedded viewer if possible (as opposed to only using external viewers)
    Viewer& UseEmbeddedViewer();

    // when displaying HTML content, run a local file server (that has access to the shared HTML folder)
    // to serve the content
    Viewer& UseSharedHtmlLocalFileServer();

    // sets an Action Invoker access token override that will be associated with ActionInvoker::WebCaller
    Viewer& SetAccessInvokerAccessTokenOverride(std::wstring action_invoker_access_token_override);
    Viewer& SetAccessInvokerAccessTokenOverride(const std::wstring* action_invoker_access_token_override);

    bool ViewFile(const std::wstring& filename);

    bool ViewFileInEmbeddedBrowser(std::wstring filename);

    bool ViewHtmlUrl(const std::wstring& url);

    bool ViewHtmlContent(std::string html, const std::wstring& local_file_server_root_directory = std::wstring());
    bool ViewHtmlContent(wstring_view html_sv, const std::wstring& local_file_server_root_directory = std::wstring());

    struct Data
    {
        enum class Type { Filename, HtmlUrl };

        bool use_embedded_viewers = false;
        bool use_shared_html_local_file_server = false;
        std::unique_ptr<std::wstring> action_invoker_access_token_override;

        Type content_type = Type::Filename;
        std::wstring content;
        std::wstring local_file_server_root_directory;
    };

    const Data& GetData() const { return m_data; }

    const ViewerOptions& GetOptions() const { return m_options; }
    ViewerOptions& GetOptions()             { return m_options; }
    Viewer& SetOptions(ViewerOptions options);
    Viewer& SetOptions(const ViewerOptions* options);

    // set the title of the Options structure
    Viewer& SetTitle(std::wstring title);

private:
    bool View();

private:
    Data m_data;
    ViewerOptions m_options;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Viewer& Viewer::SetAccessInvokerAccessTokenOverride(const std::wstring* action_invoker_access_token_override)
{
    return ( action_invoker_access_token_override != nullptr ) ? SetAccessInvokerAccessTokenOverride(*action_invoker_access_token_override) :
                                                                 *this;
}

inline Viewer& Viewer::SetOptions(const ViewerOptions* options)
{
    return ( options != nullptr ) ? SetOptions(*options) :
                                    *this;
}
