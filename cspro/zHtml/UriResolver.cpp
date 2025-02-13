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
    UserUriResolver(std::wstring uri);

    bool HasDomain() const override { return false; }

    bool DomainMatches(const std::wstring& /*uri*/) const override { return ReturnProgrammingError(false); }

    std::wstring GetSourceText(const std::wstring& /*uri*/) const override { return ReturnProgrammingError(std::wstring()); }

private:
    void Navigate(HtmlViewCtrl& sender, const std::function<HRESULT(const wchar_t*)>& navigate_function) override;

private:
    std::wstring m_uri;
};


UserUriResolver::UserUriResolver(std::wstring uri)
    :   m_uri(std::move(uri))
{
}


void UserUriResolver::Navigate(HtmlViewCtrl& sender, const std::function<HRESULT(const wchar_t*)>& navigate_function)
{
    if( navigate_function(m_uri.c_str()) != E_INVALIDARG )
        return;

    // from the example WebView2 example, try the URI with http:// at the front
    // https://learn.microsoft.com/en-us/microsoft-edge/webview2/reference/win32/icorewebview2?view=webview2-1.0.1418.22#navigate
    if( m_uri.find('.') != std::wstring::npos && m_uri.find(' ') == std::wstring::npos )
    {
        std::wstring uri_with_http = _T("http://") + m_uri;

        if( navigate_function(uri_with_http.c_str()) != E_INVALIDARG )
            return;
    }

    // if here, the URI could not be resolved, so show a 404-like page
    HtmlStringWriter html_writer;
    html_writer.WriteDefaultHeader(_T("Unknown Address"), Html::CSS::Common);

    html_writer << _T("<body><p>Unknown address: ")
                << m_uri
                << _T("</p></body></html>");

    sender.SetHtml(html_writer.str());
}



// --------------------------------------------------------------------------
// DomainUriResolver
// --------------------------------------------------------------------------

class DomainUriResolver : public UriResolver
{
public:
    DomainUriResolver(std::wstring uri, std::wstring uri_prefix, std::wstring source_text_override);

    bool HasDomain() const override { return true; }

    bool DomainMatches(const std::wstring& uri) const override { return SO::StartsWithNoCase(uri, m_uriPrefix); }

    std::wstring GetSourceText(const std::wstring& uri) const override;

private:
    void Navigate(HtmlViewCtrl& sender, const std::function<HRESULT(const wchar_t*)>& navigate_function) override;

private:
    std::wstring m_uri;
    std::wstring m_uriPrefix;
    std::wstring m_sourceTextOverride;
};


DomainUriResolver::DomainUriResolver(std::wstring uri, std::wstring uri_prefix, std::wstring source_text_override)
    :   m_uri(std::move(uri)),
        m_uriPrefix(std::move(uri_prefix)),
        m_sourceTextOverride(std::move(source_text_override))
{
    ASSERT(!m_uri.empty() && DomainMatches(m_uri) && !m_sourceTextOverride.empty());
}


std::wstring DomainUriResolver::GetSourceText(const std::wstring& uri) const
{
    ASSERT(DomainMatches(uri));

    return m_sourceTextOverride + uri.substr(m_uriPrefix.length());
}


void DomainUriResolver::Navigate(HtmlViewCtrl& /*sender*/, const std::function<HRESULT(const wchar_t*)>& navigate_function)
{
    navigate_function(m_uri.c_str());
}



// --------------------------------------------------------------------------
// UriResolver creation methods
// --------------------------------------------------------------------------

std::unique_ptr<UriResolver> UriResolver::CreateFromUserUri(std::wstring uri)
{
    return std::unique_ptr<UriResolver>(new UserUriResolver(std::move(uri)));
}


std::unique_ptr<UriResolver> UriResolver::CreateUriDomain(std::wstring uri, std::wstring uri_prefix, std::wstring source_text_override)
{
    return std::unique_ptr<UriResolver>(new DomainUriResolver(std::move(uri), std::move(uri_prefix), std::move(source_text_override)));
}
