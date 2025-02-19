#pragma once

#include <zToolsO/PortableFunctions.h>


struct ParsedUri
{
    // the default scheme should be provided without the :// (e.g., "ftp");
    // if the URI is invalid, path will be empty
    ParsedUri(std::string_view uri_sv, const char* default_scheme_if_none_provided = nullptr);

    std::string ToUri() const            { ASSERT(!path.empty());   return SO::Concatenate(scheme, "://", domain, path); }
    std::string ToUriWithoutPath() const { ASSERT(!domain.empty()); return SO::Concatenate(scheme, "://", domain); }


    std::string scheme; // provided in lowercase without the ://
    std::string domain;
    std::string path;   // / if none is explicitly specified
};



inline ParsedUri::ParsedUri(const std::string_view uri_sv, const char* const default_scheme_if_none_provided)
{
    ASSERT(default_scheme_if_none_provided == nullptr || std::string_view(default_scheme_if_none_provided).find("://") == std::string_view::npos);

    size_t scheme_start_pos = uri_sv.find("://");
    size_t domain_start_pos;

    if( scheme_start_pos != std::string_view::npos )
    {
        scheme = SO::ToLower(uri_sv.substr(0, scheme_start_pos));
        domain_start_pos = scheme_start_pos + 3;
    }

    else if( default_scheme_if_none_provided != nullptr )
    {
        scheme = default_scheme_if_none_provided;
        domain_start_pos = 0;
    }

    // invalid scheme
    else
    {
        return; 
    }

    const size_t domain_end_pos = uri_sv.find('/', domain_start_pos);
    domain = SO::Trim(uri_sv.substr(domain_start_pos, domain_end_pos - domain_start_pos));

    // invalid domain
    if( domain.empty() )
        return;

    path = ( domain_end_pos != std::string_view::npos ) ? PortableFunctions::PathEnsureTrailingForwardSlash(std::string(SO::Trim(uri_sv.substr(domain_end_pos)))) :
                                                          "/";
}
