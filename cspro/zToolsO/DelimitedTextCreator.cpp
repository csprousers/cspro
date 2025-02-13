#include "StdAfx.h"
#include "DelimitedTextCreator.h"
#include "Encoders.h"

namespace
{
    constexpr size_t InitialBufferSize = 1024;
}


DelimitedTextCreator::DelimitedTextCreator(Type type, NewlineType newline_type)
    :   m_delimiter(( type == Type::CSV )       ? ',' :
                    ( type == Type::Semicolon ) ? ';' :
                  /*( type == Type::Tab )*/       '\t'),
        m_newlineType(newline_type)
{
    // semicolon output doesn't support newlines
    ASSERT(type != Type::Semicolon || m_newlineType == NewlineType::Remove);

    m_buffer.resize(InitialBufferSize);
    ResetBufferPositions(0);
}


void DelimitedTextCreator::ResetBufferPositions(size_t current_position)
{
    m_bufferStart = m_buffer.data();
    m_bufferCurrent = m_bufferStart + current_position;
    m_bufferEnd = m_buffer.data() + m_buffer.size();

    ASSERT(m_bufferCurrent < m_bufferEnd);
}


void DelimitedTextCreator::AddText(wstring_view text_sv)
{
    std::unique_ptr<std::wstring> delimited_text;

    auto set_delimited_text = [&](wstring_view text_sv)
    {
        delimited_text = ( m_delimiter == '\t' ) ? Encoders::ToTsvWorker(text_sv) :
                                                   Encoders::ToCsvWorker(text_sv, m_delimiter);
        return ( delimited_text != nullptr );
    };

    if( m_newlineType == NewlineType::Remove && SO::ContainsNewlineCharacter(text_sv) )
    {
        auto text_without_newlines = std::make_unique<std::wstring>(text_sv);

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

    if( delimited_text != nullptr )
        text_sv = *delimited_text;

    // makes sure the buffer is large enough for the delimiter and the text
    const bool need_to_write_delimiter = ( m_bufferCurrent > m_bufferStart );

    TCHAR* buffer_pos_after_adding_text;

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
        ResetBufferPositions(current_length_used);

        calculate_buffer_pos_after_adding_text();
    }

    // add the delimiter if this isn't the first entry on the line
    if( need_to_write_delimiter )
        *(m_bufferCurrent++) = m_delimiter;

    // copy the text
    _tmemcpy(m_bufferCurrent, text_sv.data(), text_sv.length());

    m_bufferCurrent = buffer_pos_after_adding_text;
}
