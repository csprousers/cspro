#pragma once

#include <zToolsO/zToolsO.h>


class CLASS_DECL_ZTOOLSO DelimitedTextCreator
{
public:
    enum class Type { CSV, Semicolon, Tab };

    enum class NewlineType { WriteAsPresentInText, WriteAsCRLF, Remove };

    DelimitedTextCreator(Type type, NewlineType newline_type);

    const char* GetTextBuffer() const { return m_bufferStart; }
    size_t GetTextLength() const      { return ( m_bufferCurrent - m_bufferStart ); }

    std::string_view GetSV() const { return std::string_view(GetTextBuffer(), GetTextLength()); }

    void ResetText() { m_bufferCurrent = m_bufferStart; }

    void ResetBufferPosition(size_t current_position);

    // delimits the text and adds it to the buffer, adding a delimiter between the buffer and the new text
    void AddText(std::string_view text_sv);

    // adds text that does not need delimiting, adding a delimiter between the buffer and the new text
    void AddTextNoNeedToDelimit(std::string_view text_sv);

    // adds the already-delimited text to the buffer, adding a delimiter between the buffer and the new text
    void AddAlreadyDelimitedText(std::string_view text_sv);

private:
    void ResetBufferPositionFull(size_t current_position);

private:
    const char m_delimiter;
    const NewlineType m_newlineType;
    std::vector<char> m_buffer;
    char* m_bufferStart;
    char* m_bufferCurrent;
    char* m_bufferEnd;
};
