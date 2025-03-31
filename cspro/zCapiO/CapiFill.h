#pragma once


class CapiFill
{
public:
    CapiFill(std::string text_to_replace, size_t delimiter_length, bool escape_fill);

    // Returns the complete fill text, including the delimiters.
    const std::string& GetTextToReplace() const { return m_textToReplace; }

    // Returns the fill text without the delimiters.
    std::string_view GetTextToEvaluate_sv() const;

    // Returns whether the fill should be escaped.
    bool EscapeFill() const { return m_escapeFill; }

    bool operator<(const CapiFill& rhs) const;

private:
    std::string m_textToReplace;
    std::size_t m_delimiterLength;
    bool m_escapeFill;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline CapiFill::CapiFill(std::string text_to_replace, const size_t delimiter_length, const bool escape_fill)
    :   m_textToReplace(std::move(text_to_replace)),
        m_delimiterLength(delimiter_length),
        m_escapeFill(escape_fill)
{
    ASSERT(m_delimiterLength == 2 || m_delimiterLength == 3);
    ASSERT(m_textToReplace.length() >= ( m_delimiterLength * 2 ));
}


inline std::string_view CapiFill::GetTextToEvaluate_sv() const
{
    return std::string_view(m_textToReplace.data() + m_delimiterLength,
                            m_textToReplace.length() - ( 2 * m_delimiterLength ));
}


inline bool CapiFill::operator<(const CapiFill& rhs) const
{
    return ( m_textToReplace < rhs.m_textToReplace &&
             m_delimiterLength < rhs.m_delimiterLength &&
             m_escapeFill < rhs.m_escapeFill );
}
