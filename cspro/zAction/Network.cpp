#include "stdafx.h"
#include <zToolsO/MemoryStream.h>
#include <zNetwork/HttpConnection.h>


CREATE_JSON_KEY(body)
CREATE_JSON_KEY(bodyFormat)
CREATE_JSON_KEY(fetchId)
CREATE_JSON_KEY(headers)
CREATE_JSON_KEY(ok)
CREATE_JSON_KEY(responseBodyFormat)
CREATE_JSON_KEY(responseHeadersFormat)

CREATE_ENUM_JSON_SERIALIZER(HttpRequestMethod,
    { HttpRequestMethod::HTTP_GET,    HttpRequestMethodToString(HttpRequestMethod::HTTP_GET) },
    { HttpRequestMethod::HTTP_POST,   HttpRequestMethodToString(HttpRequestMethod::HTTP_POST) },
    { HttpRequestMethod::HTTP_PUT,    HttpRequestMethodToString(HttpRequestMethod::HTTP_PUT) },
    { HttpRequestMethod::HTTP_DELETE, HttpRequestMethodToString(HttpRequestMethod::HTTP_DELETE) })



// --------------------------------------------------------------------------
// FetchWrapper
// --------------------------------------------------------------------------

class ActionInvoker::Runtime::FetchWrapper
{
public:
    FetchWrapper(Runtime& runtime);

    void ParseAndSubmitRequest(const JsonNode& json_node);

    bool IsResponseOk() const;
    void WriteResponseDetails(JsonWriter& json_writer);

    std::string GetResponseContentType() const;
    std::string GetResponseBody(bool allow_bad_response);

    static JsonNode ValidateJsonBody(const std::string& body);

    template<typename CF>
    Result RunSingleActionFetch(const JsonNode& json_node, CF process_body_callback_function);

private:
    void ParseRequestHeadersAsObject(const JsonNode& headers_node);
    void ParseRequestHeadersAsArray(const JsonNode& headers_node);

private:
    Runtime& m_runtime;
    std::unique_ptr<HttpConnection> m_httpConnection;
    HttpRequest m_request;
    bool m_writeResponseHeadersAsObject;
    std::optional<std::variant<std::string, std::shared_ptr<const std::vector<std::byte>>>> m_requestBody;
    std::unique_ptr<MemoryStream> m_requestBodyStream;
    std::optional<HttpResponse> m_response;
};


ActionInvoker::Runtime::FetchWrapper::FetchWrapper(Runtime& runtime)
    :   m_runtime(runtime),
        m_httpConnection(HttpConnection::Create()),
        m_writeResponseHeadersAsObject(true)
{
    ASSERT(m_httpConnection != nullptr);
}


void ActionInvoker::Runtime::FetchWrapper::ParseAndSubmitRequest(const JsonNode& json_node)
{
    // url
    m_request.url = json_node.Get<std::string>(JK::url);

    // method
    m_request.method = json_node.GetOrDefault(JK::method, HttpRequestMethod::HTTP_GET);

    // request headers
    if( json_node.Contains(JK::headers) )
    {
        const JsonNode headers_node = json_node.Get(JK::headers);

        // headers can be specified as an object with name/value pairs or as an array of strings/objects
        headers_node.IsObject() ? ParseRequestHeadersAsObject(headers_node) :
        headers_node.IsArray()  ? ParseRequestHeadersAsArray(headers_node) :
                                  throw CSProException("HTTP request headers must be specified as an object, or an array of strings or objects.");
    }

    // response headers (which are written as objects by default)
    if( json_node.Contains(JK::responseHeadersFormat) )
        m_writeResponseHeadersAsObject = ( json_node.GetFromStringOptions(JK::responseHeadersFormat, { "object", "array" }) == 0 );

    // body
    if( json_node.Contains(JK::body) )
    {
        const std::string content_type_from_header = m_request.headers.GetValue_ContentType();

        auto set_stream = [&](const auto& body)
        {
            m_requestBodyStream = std::make_unique<MemoryStream>(body.data(), body.size());
            m_request.upload_data = m_requestBodyStream.get();
            m_request.upload_data_size_bytes = body.size();
        };

        // the body can be specified as JSON
        if( json_node.Contains(JK::bodyFormat) ? ( json_node.Get<std::string_view>(JK::bodyFormat) == JK::json ) :
                                                 ( json_node.Get(JK::body).IsObject() || ( content_type_from_header.find(MimeType::Type::Json) != std::string::npos ) ) )
        {
            m_requestBody = json_node.Get<std::string>(JK::body);
            set_stream(std::get<std::string>(*m_requestBody));

            if( content_type_from_header.empty() )
                m_request.headers.Add_ContentType_Json();
        }

        // otherwise it is specified using a binary format
        else
        {
            std::tuple<BinaryEncodingResolvedInput, std::string> binary_encoding_resolved_input_and_data_url_mediatype;
            const std::string_view body_sv = json_node.Get<std::string_view>(JK::body);

            m_requestBody = StringToBytesConverter::Convert(m_runtime, body_sv, json_node, JK::bodyFormat, &binary_encoding_resolved_input_and_data_url_mediatype);
            set_stream(*std::get<std::shared_ptr<const std::vector<std::byte>>>(*m_requestBody));

            // specify a content type if not set in the request headers
            if( content_type_from_header.empty() )
            {
                // text will be added as text/plain
                if( std::get<0>(binary_encoding_resolved_input_and_data_url_mediatype) == BinaryEncodingResolvedInput::Text )
                {
                    m_request.headers.Add_ContentType(MimeType::Type::Text);
                }

                // a specified mediatype from a data URL can be used
                else if( !std::get<1>(binary_encoding_resolved_input_and_data_url_mediatype).empty() )
                {
                    m_request.headers.Add_ContentType(std::get<1>(binary_encoding_resolved_input_and_data_url_mediatype));
                }

                // otherwise the content type will be an octet stream
                else
                {
                    m_request.headers.Add_ContentType_OctetStream();
                }
            }
        }
    }

    // make sure that there's a body for POST
    if( m_request.method == HttpRequestMethod::HTTP_POST && !m_requestBody.has_value() )
        throw CSProException("You must specify a body when submitting a POST request.");

    // submit the request
    m_response = m_httpConnection->Request(m_request);
}


void ActionInvoker::Runtime::FetchWrapper::ParseRequestHeadersAsObject(const JsonNode& headers_node)
{
    ASSERT(headers_node.IsObject());

    headers_node.ForeachNode(
        [&](const std::string_view key_sv, const JsonNode& attribute_value_node)
        {
            m_request.headers.Add(key_sv, attribute_value_node.GetOnlyString());
        });
}


void ActionInvoker::Runtime::FetchWrapper::ParseRequestHeadersAsArray(const JsonNode& headers_node)
{
    ASSERT(headers_node.IsArray());

    for( const JsonNode& header_node : headers_node.GetArray() )
    {
        if( header_node.IsObject() )
        {
            ParseRequestHeadersAsObject(header_node);
        }

        else
        {
            m_request.headers.Add(header_node.GetOnlyString());
        }
    }
}


bool ActionInvoker::Runtime::FetchWrapper::IsResponseOk() const
{
    ASSERT(m_response.has_value());
    return ( m_response->http_status >= 200 && m_response->http_status <= 299 );
}


void ActionInvoker::Runtime::FetchWrapper::WriteResponseDetails(JsonWriter& json_writer)
{
    ASSERT(m_response.has_value());

    json_writer.Write(JK::status, m_response->http_status)
               .Write(JK::ok, IsResponseOk());

    json_writer.Key(JK::headers);

    if( m_writeResponseHeadersAsObject )
    {
        json_writer.BeginObject();

        for( const std::string& header : m_response->headers.GetHeaders() )
        {
            const auto [name_sv, value_sv] = SO::GetTextOnEitherSideOfCharacter(header, ':');
            json_writer.Write(SO::Trim(name_sv), SO::Trim(value_sv));
        }

        json_writer.EndObject();
    }

    else
    {
        json_writer.Write(m_response->headers.GetHeaders());
    }
}


std::string ActionInvoker::Runtime::FetchWrapper::GetResponseContentType() const
{
    ASSERT(m_response.has_value());
    return m_response->headers.GetValue_ContentType();
}


std::string ActionInvoker::Runtime::FetchWrapper::GetResponseBody(const bool allow_bad_response)
{
    ASSERT(m_response.has_value());

    if( !allow_bad_response && !IsResponseOk() )
        throw CSProException("The body cannot be returned because the request ended in failure with status code '%d'", m_response->http_status);

    return m_response->body.ToString();
}


JsonNode ActionInvoker::Runtime::FetchWrapper::ValidateJsonBody(const std::string& body)
{
    try
    {
        return Json::Parse(body);
    }

    catch( const JsonParseException& exception )
    {
        throw CSProException("The body does not contain valid JSON: %s", exception.what());
    }
}


template<typename CF>
ActionInvoker::Result ActionInvoker::Runtime::FetchWrapper::RunSingleActionFetch(const JsonNode& json_node, const CF process_body_callback_function)
{
    // in detailed mode, the body is returned along the status, headers, and other details
    const std::unique_ptr<JsonStringWriter> json_writer = json_node.GetOrDefault(JK::detailed, false) ? Json::CreateStringWriter() :
                                                                                                        nullptr;

    // parse and submit the request
    ParseAndSubmitRequest(json_node);

    // add details about the response
    if( json_writer != nullptr )
    {
        json_writer->BeginObject();
        WriteResponseDetails(*json_writer);
        json_writer->Key(JK::body);
    }

    // process the body
    std::optional<Result> body_result;
    process_body_callback_function(json_writer.get(), body_result, GetResponseBody(false));

    if( json_writer != nullptr )
    {
        ASSERT(!body_result.has_value());

        json_writer->EndObject();
        return Result::JsonText(*json_writer);
    }

    else
    {
        ASSERT(body_result.has_value());

        return std::move(*body_result);
    }
}



// --------------------------------------------------------------------------
// Network actions
// --------------------------------------------------------------------------

ActionInvoker::Result ActionInvoker::Runtime::Network_fetch(const JsonNode& json_node, Caller& caller)
{
    // parse and submit the request
    auto fetch_wrapper = std::make_unique<FetchWrapper>(*this);
    fetch_wrapper->ParseAndSubmitRequest(json_node);

    // use a resource ID to identify this fetch request
    const std::optional<int> fetch_id = fetch_wrapper->IsResponseOk() ? std::make_optional(CreateResourceId(Resource::FetchBody, caller)) :
                                                                        std::nullopt;

    // return details about the response
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .WriteIfHasValue(JK::fetchId, fetch_id);

    fetch_wrapper->WriteResponseDetails(*json_writer);

    if( fetch_id.has_value() )
    {
        m_fetchWrappers.try_emplace(*fetch_id, std::move(fetch_wrapper));
    }

    // on error, write the response body as the error
    else
    {
        try
        {
            json_writer->Write(JK::error, fetch_wrapper->GetResponseBody(true));
        }
        catch(...) { }
    }

    json_writer->EndObject();

    return Result::JsonText(*json_writer);
}


ActionInvoker::Result ActionInvoker::Runtime::Network_fetchBody(const JsonNode& json_node, Caller& caller)
{
    const int fetch_id = GetResourceId(Resource::FetchBody, json_node, caller, JK::fetchId,
                                       "You must specify which fetch response to access using '%s'.",
                                       "Multiple fetch responses are available so you must specify which one to access using '%s'.");

    const auto& fetch_lookup = m_fetchWrappers.find(fetch_id);

    if( fetch_lookup == m_fetchWrappers.cend() )
        throw CSProException("No fetch response is associated with the ID '%d'.", fetch_id);

    const std::shared_ptr<FetchWrapper> fetch_wrapper = fetch_lookup->second;

    m_fetchWrappers.erase(fetch_lookup);

    DestroyResourceId(fetch_id);

    // the body can be discarded...
    if( json_node.GetOrDefault(JK::cancel, false) )
        return Result::Undefined();

    std::string body = fetch_wrapper->GetResponseBody(false);

    // ...or returned as JSON, text, or as binary content
    struct BodyFormatJson { };
    struct BodyFormatText { };
    std::variant<BodyFormatJson, BodyFormatText, std::optional<BinaryEncodingOutput>> body_format = std::optional<BinaryEncodingOutput>();
    std::optional<std::string> content_type_from_header;

    if( json_node.Contains(JK::bodyFormat) )
    {
        const std::string_view body_format_sv = json_node.Get<std::string_view>(JK::bodyFormat);

        if( body_format_sv == JK::json )
        {
            body_format.emplace<BodyFormatJson>();
        }

        else if( body_format_sv == JK::text )
        {
            body_format.emplace<BodyFormatText>();
        }

        else
        {
            body_format = json_node.Get<BinaryEncodingOutput>(JK::bodyFormat);
        }
    }

    // if the body format is not specified, try to set it based on the the response content type
    else
    {
        content_type_from_header = fetch_wrapper->GetResponseContentType();

        if( !content_type_from_header->empty() )
        {
            if( content_type_from_header->find(MimeType::Type::Json) != std::string::npos )
            {
                body_format.emplace<BodyFormatJson>();
            }

            else if( MimeType::IsTextTypeOrTextBased(*content_type_from_header) )
            {
                body_format.emplace<BodyFormatText>();
            }
        }
    }

    // return JSON...
    if( std::holds_alternative<BodyFormatJson>(body_format) )
    {
        FetchWrapper::ValidateJsonBody(body);
        return Result::JsonText(std::move(body));
    }

    // ...text...
    else if( std::holds_alternative<BodyFormatText>(body_format) )
    {
        return Result::String(std::move(body));
    }

    // ...or bytes (as a string)
    else
    {
        ASSERT(std::holds_alternative<std::optional<BinaryEncodingOutput>>(body_format));
        BytesToStringConverter bytes_to_string_converter(this, std::get<std::optional<BinaryEncodingOutput>>(body_format));

        if( !content_type_from_header.has_value() && bytes_to_string_converter.GetBinaryEncodingOutput() == BinaryEncodingOutput::DataUrl )
            content_type_from_header = fetch_wrapper->GetResponseContentType();

        return Result::String(bytes_to_string_converter.Convert(SO::CreateByteVector(body), ValueOrDefault(std::move(content_type_from_header))));
    }
}


ActionInvoker::Result ActionInvoker::Runtime::Network_fetchBytes(const JsonNode& json_node, Caller& /*caller*/)
{
    BytesToStringConverter bytes_to_string_converter(this, json_node, JK::responseBodyFormat);
    FetchWrapper fetch_wrapper(*this);

    return fetch_wrapper.RunSingleActionFetch(json_node,
        [&](JsonStringWriter* const json_writer, std::optional<Result>& body_result, const std::string body)
        {
            std::string converted_bytes = bytes_to_string_converter.Convert(SO::CreateByteVector(body), fetch_wrapper.GetResponseContentType());

            if( json_writer != nullptr )
            {
                json_writer->Write(converted_bytes);
            }

            else
            {
                body_result.emplace(Result::String(std::move(converted_bytes)));
            }
        });
}


ActionInvoker::Result ActionInvoker::Runtime::Network_fetchJson(const JsonNode& json_node, Caller& /*caller*/)
{
    FetchWrapper fetch_wrapper(*this);

    return fetch_wrapper.RunSingleActionFetch(json_node,
        [](JsonStringWriter* const json_writer, std::optional<Result>& body_result, std::string body)
        {
            const JsonNode json_node = FetchWrapper::ValidateJsonBody(body);

            if( json_writer != nullptr )
            {
                json_writer->Write(json_node);
            }

            else
            {
                body_result.emplace(Result::JsonText(std::move(body)));
            }
        });
}


ActionInvoker::Result ActionInvoker::Runtime::Network_fetchText(const JsonNode& json_node, Caller& /*caller*/)
{
    FetchWrapper fetch_wrapper(*this);

    return fetch_wrapper.RunSingleActionFetch(json_node,
        [](JsonStringWriter* const json_writer, std::optional<Result>& body_result, std::string body)
        {
            if( json_writer != nullptr )
            {
                json_writer->Write(body);
            }

            else
            {
                body_result.emplace(Result::String(std::move(body)));
            }
        });
}
