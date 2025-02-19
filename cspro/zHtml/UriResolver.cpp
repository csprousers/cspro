#include "stdafx.h"
#include "UriResolver.h"
#include "HtmlViewCtrl.h"
#include "HtmlWriter.h"


// --------------------------------------------------------------------------
// UserUriResolver
// --------------------------------------------------------------------------

class UserUriResolver : public UriResolver
{
public:
    UserUriResolver(std::string uri);

    bool HasDomain() const override { return false; }

    bool DomainMatches(const std::string& /*uri*/) const override { return ReturnProgrammingError(false); }

    std::string GetSourceText(const std::string& /*uri*/) const override { return ReturnProgrammingError(std::string()); }

private:
    void Navigate(HtmlViewCtrl& sender, const std::function<HRESULT(const std::string&)>& navigate_function) override;

private:
    std::string m_uri;
};


UserUriResolver::UserUriResolver(std::string uri)
    :   m_uri(std::move(uri))
{
}


void UserUriResolver::Navigate(HtmlViewCtrl& sender, const std::function<HRESULT(const std::string&)>& navigate_function)
{
    if( navigate_function(m_uri) != E_INVALIDARG )
        return;

    // from the example WebView2 example, try the URI with http:// at the front
    // https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2?view=webview2-1.0.1418.22#navigate
    if( m_uri.find('.') != std::string::npos && m_uri.find(' ') == std::string::npos )
    {
        const std::string uri_with_http = "http://" + m_uri;

        if( navigate_function(uri_with_http) != E_INVALIDARG )
            return;
    }

    // if here, the URI could not be resolved, so show a 404-like page
    HtmlStringWriter html_writer;
    html_writer.WriteDefaultHeader("Unknown Address", Html::CSS::Common);

    html_writer << "<body><p>Unknown address: "
                << m_uri
                << "</p></body></html>";

    sender.SetHtml(html_writer.str());
}



// --------------------------------------------------------------------------
// DomainUriResolver
// --------------------------------------------------------------------------

class DomainUriResolver : public UriResolver
{
public:
    DomainUriResolver(std::string uri, std::string uri_prefix, std::string source_text_override);

    bool HasDomain() const override { return true; }

    bool DomainMatches(const std::string& uri) const override { return SO::StartsWithNoCase(uri, m_uriPrefix); }

    std::string GetSourceText(const std::string& uri) const override;

private:
    void Navigate(HtmlViewCtrl& sender, const std::function<HRESULT(const std::string&)>& navigate_function) override;

private:
    std::string m_uri;
    std::string m_uriPrefix;
    std::string m_sourceTextOverride;
};


DomainUriResolver::DomainUriResolver(std::string uri, std::string uri_prefix, std::string source_text_override)
    :   m_uri(std::move(uri)),
        m_uriPrefix(std::move(uri_prefix)),
        m_sourceTextOverride(std::move(source_text_override))
{
    ASSERT(!m_uri.empty() && DomainMatches(m_uri) && !m_sourceTextOverride.empty());
}


std::string DomainUriResolver::GetSourceText(const std::string& uri) const
{
    ASSERT(DomainMatches(uri));

    return SO::Concatenate(m_sourceTextOverride, std::string_view(uri).substr(m_uriPrefix.length()));
}


void DomainUriResolver::Navigate(HtmlViewCtrl& /*sender*/, const std::function<HRESULT(const std::string&)>& navigate_function)
{
    navigate_function(m_uri);
}



// --------------------------------------------------------------------------
// UriResolver creation methods
// --------------------------------------------------------------------------

std::unique_ptr<UriResolver> UriResolver::CreateFromUserUri(std::string uri)
{
    return std::unique_ptr<UriResolver>(new UserUriResolver(std::move(uri)));
}


std::unique_ptr<UriResolver> UriResolver::CreateUriDomain(std::string uri, std::string uri_prefix, std::string source_text_override)
{
    return std::unique_ptr<UriResolver>(new DomainUriResolver(std::move(uri), std::move(uri_prefix), std::move(source_text_override)));
}
