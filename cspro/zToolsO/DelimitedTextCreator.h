#pragma once

#include <zToolsO/zToolsO.h>


class CLASS_DECL_ZTOOLSO DelimitedTextCreator
{
public:
    enum class Type { CSV, Semicolon, Tab };

    enum class NewlineType { WriteAsPresentInText, WriteAsCRLF, Remove };

    DelimitedTextCreator(Type type, NewlineType newline_type);

    const TCHAR* GetTextBuffer() const { return m_bufferStart; }
    size_t GetTextLength() const       { return ( m_bufferCurrent - m_bufferStart ); }

    void ResetText()                   { m_bufferCurrent = m_bufferStart; }

    void AddText(wstring_view text_sv);

private:
    void ResetBufferPositions(size_t current_position);

private:
    const TCHAR m_delimiter;
    const NewlineType m_newlineType;
    std::vector<TCHAR> m_buffer;
    TCHAR* m_bufferStart;
    TCHAR* m_bufferCurrent;
    TCHAR* m_bufferEnd;
};
