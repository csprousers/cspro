#pragma once

#include <zHtml/zHtml.h>


// a UriResolver object is primarily used for two things:
// - resolving URIs that a user enters; e.g., if csprousers.org is not valid, http://csprousers.org will be tried
// - creating an object, part of a "domain," that can be used to override the source text (e.g., for localhost URIs)

class UriResolver
{
    friend class HtmlViewCtrl;

protected:
    UriResolver() { }

public:
    virtual ~UriResolver() { }

    ZHTML_API static std::unique_ptr<UriResolver> CreateFromUserUri(std::string uri);
    ZHTML_API static std::unique_ptr<UriResolver> CreateUriDomain(std::string uri, std::string uri_prefix, std::string source_text_override);

    virtual bool HasDomain() const = 0;
    virtual bool DomainMatches(const std::string& uri) const = 0;
    virtual std::string GetSourceText(const std::string& uri) const = 0;

private:
    virtual void Navigate(HtmlViewCtrl& sender, const std::function<HRESULT(const std::string&)>& navigate_function) = 0;
};
