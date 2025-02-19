#include "StdAfx.h"
#include "DelimitedTextCreator.h"
#include "Encoders.h"

namespace
{
    constexpr size_t InitialBufferSize = 1024;
}


DelimitedTextCreator::DelimitedTextCreator(const Type type, const NewlineType newline_type)
    :   m_delimiter(( type == Type::CSV )       ? ',' :
                    ( type == Type::Semicolon ) ? ';' :
                  /*( type == Type::Tab )*/       '\t'),
        m_newlineType(newline_type),
        m_buffer(InitialBufferSize, '\0')
{
    // semicolon output doesn't support newlines
    ASSERT(type != Type::Semicolon || m_newlineType == NewlineType::Remove);

    ResetBufferPositionFull(0);
}


void DelimitedTextCreator::ResetBufferPosition(const size_t current_position)
{
    m_bufferCurrent = m_bufferStart + current_position;

    ASSERT(m_bufferCurrent < m_bufferEnd);
}


void DelimitedTextCreator::ResetBufferPositionFull(const size_t current_position)
{
    m_bufferStart = m_buffer.data();
    m_bufferEnd = m_buffer.data() + m_buffer.size();

    ResetBufferPosition(current_position);
}


void DelimitedTextCreator::AddText(const std::string_view text_sv)
{
    std::unique_ptr<std::string> delimited_text;

    auto set_delimited_text = [&](const std::string_view text_to_delimit_sv)
    {
        delimited_text = ( m_delimiter == '\t' ) ? Encoders::ToTsvWorker(text_to_delimit_sv) :
                                                   Encoders::ToCsvWorker(text_to_delimit_sv, m_delimiter);
        return ( delimited_text != nullptr );
    };

    if( m_newlineType == NewlineType::Remove && SO::ContainsNewlineCharacter(text_sv) )
    {
        auto text_without_newlines = std::make_unique<std::string>(text_sv);

        text_without_newlines->erase(std::remove_if(text_without_newlines->begin(), text_without_newlines->end(), is_crlf));

        // if the text was not delimited, swap it with the text without newlines
        if( !set_delimited_text(*text_without_newlines) )
            delimited_text = std::move(text_without_newlines);
    }

    else
    {
        if( set_delimited_text(text_sv) && m_newlineType == NewlineType::WriteAsCRLF )
            SO::MakeNewlineCRLF(*delimited_text);
    }    

    if( delimited_text == nullptr )
    {
        AddAlreadyDelimitedText(text_sv);
    }

    else
    {
        AddAlreadyDelimitedText(*delimited_text);
    }
}


void DelimitedTextCreator::AddTextNoNeedToDelimit(const std::string_view text_sv)
{
    ASSERT(!SO::ContainsNewlineCharacter(text_sv));
    ASSERT(( m_delimiter == '\t' ) ? ( Encoders::ToTsvWorker(text_sv) == nullptr ) :
                                     ( Encoders::ToCsvWorker(text_sv, m_delimiter) == nullptr ));

    AddAlreadyDelimitedText(text_sv);
}


void DelimitedTextCreator::AddAlreadyDelimitedText(const std::string_view text_sv)
{
    // makes sure the buffer is large enough for the delimiter and the text
    const bool need_to_write_delimiter = ( m_bufferCurrent > m_bufferStart );

    char* buffer_pos_after_adding_text;

    auto calculate_buffer_pos_after_adding_text = [&]()
    {
        buffer_pos_after_adding_text = m_bufferCurrent + ( need_to_write_delimiter ? 1 : 0 ) + text_sv.length();
    };

    calculate_buffer_pos_after_adding_text();

    // if necessary, resize the output buffer (to more than necessary to minimize these allocations)
    if( buffer_pos_after_adding_text >= m_bufferEnd )
    {
        const size_t current_length_used = GetTextLength();

        m_buffer.resize(m_buffer.size() * 2 + text_sv.length());
        ResetBufferPositionFull(current_length_used);

        calculate_buffer_pos_after_adding_text();
    }

    // add the delimiter if this isn't the first entry on the line
    if( need_to_write_delimiter )
        *(m_bufferCurrent++) = m_delimiter;

    // copy the text
    memcpy(m_bufferCurrent, text_sv.data(), text_sv.length());

    m_bufferCurrent = buffer_pos_after_adding_text;
}
