#include "stdafx.h"
#include "JsonStream.h"
#include "JsonConsExceptionRethrower.h"
#include "JsonSpecFile.h"
#include <external/jsoncons/json_cursor.hpp>
#include <fstream>


using BasicJson = jsoncons::basic_json<char, jsoncons::order_preserving_policy, std::allocator<char>>;


// --------------------------------------------------------------------------
// JsonStreamData
// --------------------------------------------------------------------------

struct JsonStreamData
{
    std::unique_ptr<std::istream> stream;
    std::optional<std::streampos> stream_size;
    std::unique_ptr<jsoncons::json_stream_cursor> cursor;
    size_t current_level = 0;
    bool cursor_started = false;

    void InitializeCursor();
};


void JsonStreamData::InitializeCursor()
{
    try
    {
        cursor = std::make_unique<jsoncons::json_stream_cursor>(*stream);
        current_level = 0;
        cursor_started = false;
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}



// --------------------------------------------------------------------------
// JsonStream
// --------------------------------------------------------------------------

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


JsonStream JsonStream::FromStream(std::unique_ptr<std::istream> stream, std::optional<std::streampos> stream_size/* = std::nullopt*/)
{
    return JsonStream(std::make_unique<JsonStreamData>(JsonStreamData { std::move(stream), std::move(stream_size) }));
}


JsonStream JsonStream::FromString(const std::string& text)
{
    return FromStream(std::make_unique<std::stringstream>(text));
}


JsonStream JsonStream::FromString(std::string&& text)
{
    static_assert(__cplusplus < 202002L, "move text into the created std::stringstream");
    return FromString(text);
}


JsonStream JsonStream::FromFile(const InterfaceString file_path)
{
    std::streampos file_size;
    std::unique_ptr<std::ifstream> stream = FileIO::OpenTextInputFileStream(file_path, &file_size);
    
    return FromStream(std::move(stream), file_size);
}


JsonStream JsonStream::FromSpecFile(const InterfaceString file_path, const std::function<std::string()>& pre_80_spec_file_converter)
{
    return JsonSpecFile::IsPre80SpecFile(file_path) ? FromString(pre_80_spec_file_converter()) :
                                                      FromFile(file_path);
}


bool JsonStream::RestartStream()
{
    if( m_data->stream->seekg(0) )
    {
        m_data->InitializeCursor();
        return true;
    }

    return false;
}


JsonNode JsonStream::ReadUntilKey(const std::string_view key_sv, const bool parse_keys_only_at_this_level/* = true*/)
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
                jsoncons::json_decoder<jsoncons::ojson> json_decoder;
                m_data->cursor->read_to(json_decoder);

                return JsonNode(std::make_unique<BasicJson>(json_decoder.get_result()));
            }

            else if( event.event_type() == jsoncons::staj_event_type::key && event.get<std::string_view>() == key_sv )
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

        throw JsonParseException("'%s' key not found", std::string(key_sv).c_str());
    }

    catch( const jsoncons::json_exception& exception )
    {
        RethrowJsonConsException(exception);
    }
}



// --------------------------------------------------------------------------
// JsonStreamObjectArrayIterator +
// JsonStream::CreateObjectArrayIterator
// --------------------------------------------------------------------------

JsonStreamObjectArrayIterator::JsonStreamObjectArrayIterator(JsonStreamData& data)
    :   m_data(data)
{
}


std::optional<JsonNode> JsonStreamObjectArrayIterator::Next()
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
                jsoncons::json_decoder<jsoncons::ojson> json_decoder;
                m_data.cursor->read_to(json_decoder);

                return JsonNode(std::make_unique<BasicJson>(json_decoder.get_result()));
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
    if( m_data.stream_size.has_value() )
    {
        const std::streampos current_pos = m_data.stream->tellg();

        if( current_pos != -1 )
            return CreatePercent(current_pos, *m_data.stream_size);
    }

    return m_data.stream->eof() ? 100 :
                                  ReturnProgrammingError(0);
}


bool JsonStreamObjectArrayIterator::AtEndOfStream() const
{
    if( !m_data.cursor->eof() )
    {
        std::error_code ec;
        m_data.cursor->check_done(ec);

        if( ec )
        {
            std::string rest = static_cast<const std::stringstream*>(m_data.stream.get())->str();
            return false;
        }
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
