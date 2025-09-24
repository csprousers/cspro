#pragma once

#include <zNetwork/zNetwork.h>


// List of HTTP style headers (key and value separated by colon).

class ZNETWORK_API HeaderList
{
public:
    // Returns all headers.
    const std::vector<std::string>& GetHeaders() const noexcept { return m_headers; }

    // Returns a header value by index.
    const std::string& GetHeader(size_t index) const;

    // Returns a header value by name (e.g., "Content-Length"), returning blank if not defined.
    std::string GetValue(std::string_view name_sv) const;

    // Adds a header defined using the colon-separated name/value pair.
    HeaderList& Add(std::string header);

    // Adds a header constructed from the name and value.
    HeaderList& Add(std::string_view name_sv, std::string_view value_sv);

    // Adds a header constructed from the name and value only if the value is not blank.
    HeaderList& AddIfNotBlank(std::string_view name_sv, std::string_view value_sv);

    // Adds a header whose value is specified as JSON. The JSON is escaped to be "HTTP header safe".
    HeaderList& AddJson(std::string_view name_sv, std::string_view json_text_sv);

    // Adds a header with the value encoded as Base64 prior to adding.
    HeaderList& AddAsBase64(std::string_view name_sv, std::string_view value_sv);

    // Adds a header with the value deflated (using ZLib::Deflate) and encoded as Base64 prior to adding.
    HeaderList& AddAsDeflatedBase64(std::string_view name_sv, std::string value);

    // Adds all headers from another header list.
    void Append(const HeaderList& header_list);
    void Append(HeaderList&& header_list);

    // Adds: User-Agent: CSPro sync client/[X.X.X detailed version number]
    HeaderList& Add_UserAgent_CSProSyncClient();

    // Functions to get common headers:

    std::string GetValue_ContentType() const              { return GetValue("Content-Type"); }

    // Functions to add common headers:

    HeaderList& Add_Accept_Json()                         { return Add("Accept: application/json"); }
    HeaderList& Add_Accept_OctetStream()                  { return Add("Accept: application/octet-stream"); }

    HeaderList& Add_ContentEncoding_Deflate()             { return Add("Content-Encoding: deflate"); }

    template<typename T>
    HeaderList& Add_ContentLength(T length)               { return Add(FormatText("Content-Length: %d", static_cast<int>(length))); }

    HeaderList& Add_ContentMD5(std::string_view md5_sv)   { return Add("Content-MD5", md5_sv); }

    HeaderList& Add_ContentType(std::string_view type_sv) { return Add("Content-Type", type_sv); }
    HeaderList& Add_ContentType_Json()                    { return Add("Content-Type: application/json; charset=utf-8"); }
    HeaderList& Add_ContentType_OctetStream()             { return Add("Content-Type: application/octet-stream"); }
    HeaderList& Add_ContentType_FormUrlEncoded()          { return Add("Content-Type: application/x-www-form-urlencoded"); }

    HeaderList& Add_IfNoneMatch(std::string_view etag_sv) { return Add("If-None-Match", etag_sv); }

private:
    std::vector<std::string> m_headers;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline const std::string& HeaderList::GetHeader(const size_t index) const
{
    ASSERT(index < m_headers.size());
    return m_headers[index];
}


inline HeaderList& HeaderList::Add(std::string header)
{
    // the Accept-Encoding value will be handled automatically by the HttpConnection, etc. subclass
    ASSERT(!SO::StartsWithNoCase(header, "Accept-Encoding"));

    m_headers.emplace_back(std::move(header));

    return *this;
}


inline HeaderList& HeaderList::Add(const std::string_view name_sv, const std::string_view value_sv)
{
    ASSERT(!SO::ContainsNewlineCharacter(value_sv));

    return Add(SO::Concatenate(name_sv, ": ", value_sv));
}


inline HeaderList& HeaderList::AddIfNotBlank(const std::string_view name_sv, const std::string_view value_sv)
{
    return !SO::IsBlank(value_sv) ? Add(name_sv, value_sv) :
                                    *this;
}
