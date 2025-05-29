#pragma once

#include <zUtilO/zUtilO.h>

class ExceptionHolder;


// additional options that can be used by viewers
struct ViewerOptions
{
    std::optional<CSize> requested_size;
    SharableString title;
    std::optional<bool> show_close_button;
    std::shared_ptr<const JsonNode> display_options_node;
    SharableString action_invoker_ui_get_input_data;
};


class CLASS_DECL_ZUTILO Viewer
{
public:
    // display the content in an embedded viewer if possible (as opposed to only using external viewers)
    Viewer& UseEmbeddedViewer();

    // when displaying HTML content, run a local file server (that has access to the shared HTML folder)
    // to serve the content
    Viewer& UseSharedHtmlLocalFileServer();

    // in an environment where exceptions may be thrown by a viewer (such as by an Action Invoker action),
    // these exceptions will be held by the supplied ExceptionHolder (or a created one if passed null)
    Viewer& UseExceptionHolder(std::shared_ptr<ExceptionHolder> exception_holder);

    // sets an Action Invoker access token override that will be associated with ActionInvoker::WebCaller
    Viewer& SetAccessInvokerAccessTokenOverride(std::string action_invoker_access_token_override);
    Viewer& SetAccessInvokerAccessTokenOverride(const std::string* action_invoker_access_token_override);

    bool ViewFile(const std::string& file_path);

    bool ViewFileInEmbeddedBrowser(std::string file_path);

    bool ViewHtmlUrl(const std::string& url);

    bool ViewHtmlContent(SharableString html, const std::string& local_file_server_root_directory = std::string());

    struct Data
    {
        enum class Type { FilePath, HtmlUrl };

        bool use_embedded_viewers = false;
        bool use_shared_html_local_file_server = false;
        std::shared_ptr<ExceptionHolder> exception_holder;
        std::unique_ptr<std::string> action_invoker_access_token_override;

        Type content_type = Type::FilePath;
        std::string content;
        std::string local_file_server_root_directory;
    };

    const Data& GetData() const { return m_data; }

    const ViewerOptions& GetOptions() const { return m_options; }
    ViewerOptions& GetOptions()             { return m_options; }
    Viewer& SetOptions(ViewerOptions options);
    Viewer& SetOptions(const ViewerOptions* options);

    // set the title of the Options structure
    Viewer& SetTitle(SharableString title);

private:
    bool View();

private:
    Data m_data;
    ViewerOptions m_options;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline Viewer& Viewer::SetAccessInvokerAccessTokenOverride(const std::string* const action_invoker_access_token_override)
{
    return ( action_invoker_access_token_override != nullptr ) ? SetAccessInvokerAccessTokenOverride(*action_invoker_access_token_override) :
                                                                 *this;
}

inline Viewer& Viewer::SetOptions(const ViewerOptions* const options)
{
    return ( options != nullptr ) ? SetOptions(*options) :
                                    *this;
}
