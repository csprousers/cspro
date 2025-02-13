#include "stdafx.h"
#include "JsonStream.h"
#include "JsonConsExceptionRethrower.h"
#include "JsonSpecFile.h"
#include <zToolsO/Utf8FileStream.h>
#include <external/jsoncons/json_cursor.hpp>
#include <fstream>


using BasicJson = jsoncons::basic_json<wchar_t, jsoncons::order_preserving_policy, std::allocator<char>>;


// --------------------------------------------------
// JsonStreamData
// --------------------------------------------------

struct JsonStreamData
{
    std::unique_ptr<std::wistream> stream;
    std::unique_ptr<std::wstring> owned_text;
    std::unique_ptr<jsoncons::wjson_stream_cursor> cursor;
    size_t current_level = 0;
    bool cursor_started = false;

    void InitializeCursor();
};


void JsonStreamData::InitializeCursor()
{
    try
    {
        cursor = std::make_unique<jsoncons::wjson_stream_cursor>(*stream);
        current_level = 0;
        cursor_started = false;
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}



// --------------------------------------------------
// JsonStream
// --------------------------------------------------

JsonStream::JsonStream(std::unique_ptr<JsonStreamData> data)
    :   m_data(std::move(data))
{
    ASSERT(m_data != nullptr && m_data->stream != nullptr && m_data->cursor == nullptr);

    m_data->InitializeCursor();
}


JsonStream::JsonStream(JsonStream&& rhs) noexcept = default;


JsonStream::~JsonStream()
{
}


JsonStream JsonStream::FromStream(std::unique_ptr<std::wistream> stream)
{
    return JsonStream(std::make_unique<JsonStreamData>(JsonStreamData { std::move(stream) }));
}


JsonStream JsonStream::FromString(std::wstring text)
{
    auto owned_text = std::make_unique<std::wstring>(std::move(text));
    auto string_stream = std::make_unique<std::wstringstream>(*owned_text);

    return JsonStream(std::make_unique<JsonStreamData>(JsonStreamData { std::move(string_stream), std::move(owned_text) }));
}


JsonStream JsonStream::FromFile(NullTerminatedString filename)
{
    return FromStream(FileIO::OpenWideTextInputFileStream(filename));
}


JsonStream JsonStream::FromSpecFile(NullTerminatedString filename, const std::function<std::wstring()>& pre_80_spec_file_converter)
{
    FileIO::FileAndSize file_and_size = FileIO::OpenFile(filename);

    if( JsonSpecFile::IsPre80SpecFile(file_and_size) )
    {
        fclose(file_and_size.file);
        return FromString(pre_80_spec_file_converter());
    }

    else
    {
        fseek(file_and_size.file, 0, SEEK_SET);
        return FromStream(std::make_unique<Utf8InputFileStream>(filename, std::move(file_and_size)));
    }                                                     
}


bool JsonStream::RestartStream()
{
    ASSERT(dynamic_cast<Utf8InputFileStream*>(m_data->stream.get()) != nullptr);

    if( m_data->stream->seekg(0) )
    {
        m_data->InitializeCursor();
        return true;
    }

    return false;
}


JsonNode<wchar_t> JsonStream::ReadUntilKey(wstring_view key, bool parse_keys_only_at_this_level/* = true*/)
{
    try
    {
        bool key_found = false;
        size_t starting_level = m_data->current_level;

        for( ; !m_data->cursor->done(); m_data->cursor->next() )
        {
            const auto& event = m_data->cursor->current();

            if( key_found )
            {
                jsoncons::json_decoder<jsoncons::wojson> json_decoder;
                m_data->cursor->read_to(json_decoder);

                return JsonNode<wchar_t>(std::make_shared<BasicJson>(json_decoder.get_result()));
            }

            else if( event.event_type() == jsoncons::staj_event_type::key && event.get<std::wstring_view>() == key )
            {
                if( !parse_keys_only_at_this_level || starting_level == m_data->current_level )
                    key_found = true;
            }

            else if( event.event_type() == jsoncons::staj_event_type::begin_array ||
                     event.event_type() == jsoncons::staj_event_type::begin_object )
            {
                ++m_data->current_level;

                // if the first event is the beginning of the root object, reset the starting level
                if( !m_data->cursor_started && event.event_type() == jsoncons::staj_event_type::begin_object )
                {
                    ASSERT(m_data->current_level == 1);
                    starting_level = 1;
                }
            }

            else if( event.event_type() == jsoncons::staj_event_type::end_array ||
                     event.event_type() == jsoncons::staj_event_type::end_object )
            {
                ASSERT(m_data->current_level > 0);
                --m_data->current_level;
            }

            m_data->cursor_started = true;
        }

        throw JsonParseException(_T("'%s' key not found"), std::wstring(key).c_str());
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}



// --------------------------------------------------
// JsonStreamObjectArrayIterator +
// JsonStream::CreateObjectArrayIterator
// --------------------------------------------------

JsonStreamObjectArrayIterator::JsonStreamObjectArrayIterator(JsonStreamData& data)
    :   m_data(data),
        m_streamAsUtf8InputFileStream(dynamic_cast<Utf8InputFileStream*>(m_data.stream.get()))
{
}


std::optional<JsonNode<wchar_t>> JsonStreamObjectArrayIterator::Next()
{
    try
    {
        ASSERT(!m_data.cursor->done());

        m_data.cursor->next();

        if( !m_data.cursor->done() )
        {
            const auto& event = m_data.cursor->current();

            if( event.event_type() == jsoncons::staj_event_type::end_array )
            {
                m_data.cursor->next();
                return std::nullopt;
            }

            else if( event.event_type() == jsoncons::staj_event_type::begin_object )
            {
                jsoncons::json_decoder<jsoncons::wojson> json_decoder;
                m_data.cursor->read_to(json_decoder);

                return JsonNode<wchar_t>(std::make_shared<BasicJson>(json_decoder.get_result()));
            }
        }

        throw JsonParseException("The next element in the JSON stream must be an object");
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}


int JsonStreamObjectArrayIterator::GetPercentRead() const
{
    // creating the cursor on a Utf8InputFileStream object led to tellg always returning -1 and
    // overriden methods like seekoff not being called, so we will query for the percent read
    // directly (for the 99% use case where the stream is a Utf8InputFileStream)
    return ( m_streamAsUtf8InputFileStream != nullptr ) ? m_streamAsUtf8InputFileStream->GetPercentRead() :
                                                          0;
}


bool JsonStreamObjectArrayIterator::AtEndOfStream() const
{
    if( !m_data.cursor->eof() )
    {
        std::error_code ec;
        m_data.cursor->check_done(ec);

        if( ec )
            return false;
    }

    return true;
}


JsonStreamObjectArrayIterator JsonStream::CreateObjectArrayIterator()
{
    try
    {
        if( !m_data->cursor->done() )
        {
            const auto& event = m_data->cursor->current();

            if( event.event_type() == jsoncons::staj_event_type::begin_array )
                return JsonStreamObjectArrayIterator(*m_data);
        }

        throw JsonParseException("The next element in the JSON stream must be an array");
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}
