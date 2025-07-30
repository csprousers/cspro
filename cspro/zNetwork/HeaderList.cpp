#include "stdafx.h"
#include "HeaderList.h"
#include <zToolsO/base64.h>
#include <zJson/ValidJsonAsserter.h>
#include <zUtilO/Versioning.h>
#include <zZip/ZLib.h>


std::string HeaderList::GetValue(const std::string_view name_sv) const
{
    for( const std::string& header : m_headers )
    {
        if( SO::StartsWithNoCase(header, name_sv) )
        {
            const std::string_view post_name_sv = std::string_view(header).substr(name_sv.length());

            if( !post_name_sv.empty() && post_name_sv.front() == ':' )
                return std::string(SO::Trim(post_name_sv.substr(1)));
        }
    }

    return std::string();
}


HeaderList& HeaderList::AddJson(const std::string_view name_sv, const std::string_view json_text_sv)
{
    AssertValidJson(json_text_sv);

    const char* json_text_itr = json_text_sv.data();
    const char* const json_text_end = json_text_itr + json_text_sv.length();

    std::unique_ptr<std::string> encoded_json_text;

    // determine if the text needs to be encoded
    auto char_requires_encoding = [](const char ch)
    {
        // allowable characters are printable ASCII (33-126), spaces (32), and horizontal tabs (9)
        return ( ( ch < 32 || ch > 126 ) && ch != '\t' );
    };

    for( ; json_text_itr != json_text_end; ++json_text_itr )
    {
        if( char_requires_encoding(*json_text_itr) )
        {
            // copy over the string not requiring encoding
            encoded_json_text = std::make_unique<std::string>(json_text_sv.data(), json_text_itr - json_text_sv.data());
            break;
        }
    }

    // add the text directly if no encoding is necessary
    if( encoded_json_text == nullptr )
        return Add(name_sv, json_text_sv);

    // encode any invalid characters
    for( ; json_text_itr < json_text_end; )
    {
        const char ch = *json_text_itr;

        if( !char_requires_encoding(ch) )
        {
            // ensure that the JSON does not use any unexpected control characters
            ASSERT(ch >= 32);
            encoded_json_text->push_back(ch);

            ++json_text_itr;
        }

        // newlines should not appear in string literals, so they can be safely removed
        else if( is_crlf(ch) )
        {
            ++json_text_itr;
        }

        // other characters are presumably UTF-8 multibyte characters appearing in string literals, so they will be escaped using \uXXXX
        else
        {
            ASSERT(!TC::IsUtf8SingleByte(ch));
            const size_t ch_utf8_length = TC::Utf8BytesFromFirstByte(ch);
            const wchar_t wide_ch = TC::GetWideCharFromUtf8Sequence(json_text_itr, ch_utf8_length);

            encoded_json_text->append(FormatText("\\u%04x", static_cast<int>(wide_ch)));

            json_text_itr += ch_utf8_length;
        }
    }

    AssertValidJson(*encoded_json_text);
    ASSERT81(Json::Parse(json_text_sv).GetNodeAsString() == Json::Parse(*encoded_json_text).GetNodeAsString());

    return Add(name_sv, *encoded_json_text);
}


HeaderList& HeaderList::AddAsBase64(std::string_view name_sv, const std::string_view value_sv)
{
    return Add(name_sv, Base64::Encode(value_sv));
}


HeaderList& HeaderList::AddAsDeflatedBase64(std::string_view name_sv, std::string value)
{
    const bool success = ZLib::Deflate(value);
    ASSERT(success);

    return AddAsBase64(name_sv, value);
}


void HeaderList::Append(const HeaderList& header_list)
{
    m_headers.insert(m_headers.end(), header_list.m_headers.cbegin(), header_list.m_headers.cend());
}


void HeaderList::Append(HeaderList&& header_list)
{
    m_headers.insert(m_headers.end(), std::make_move_iterator(header_list.m_headers.begin()),
                                      std::make_move_iterator(header_list.m_headers.end()));

}


HeaderList& HeaderList::Add_UserAgent_CSProSyncClient()
{
    static const std::string user_agent_header = "User-Agent: CSPro sync client/" + Versioning::GetVersionDetailedString();
    return Add(user_agent_header);
}
