#pragma once

#include <zToolsO/StringView.h>

class NullTerminatedStringView;


// ------------------------------------------------------
// NullTerminatedString
//
// this class wraps a string object, storing its null-terminated string;
// the length is stored if immediately known, or calculated on demand;
//
// this object can be used in function parameters as a way to wrap
// either std::wstring, CString, or pointers to text without the
// calculation hit of immediately calculating the length of a pointer
// to text (which would occur if constructing a string view)
// ------------------------------------------------------

class NullTerminatedString
{
public:
    NullTerminatedString(const std::wstring& text) noexcept
        :   m_text(text.c_str()),
            m_length(text.length())
    {
    }

    // defined below NullTerminatedStringView
    NullTerminatedString(const NullTerminatedStringView& text) noexcept;

    NullTerminatedString(const CString& text)
        :   m_text(text.GetString()),
            m_length(text.GetLength())
    {
    }

    NullTerminatedString(const wchar_t* const text) noexcept
        :   m_text(text),
            m_length(SIZE_MAX)
    {
        ASSERT(m_text != nullptr);
    }

    [[nodiscard]] const wchar_t* c_str() const noexcept
    {
        return m_text;
    }

    [[nodiscard]] const wchar_t* data() const noexcept
    {
        return m_text;
    }

    size_t length() const
    {
        if( m_length == SIZE_MAX )
            m_length = wcslen(m_text);

        return m_length;
    }

    [[nodiscard]] bool empty() const
    {
        return ( length() == 0 );
    }

    [[nodiscard]] wchar_t front() const
    {
        ASSERT(!empty());
        return *m_text;
    }

    [[nodiscard]] wchar_t back() const
    {
        ASSERT(!empty());
        return m_text[length() - 1];
    }

    [[nodiscard]] wchar_t operator[](size_t offset) const
    {
        ASSERT(offset <= length());
        return m_text[offset];
    }

    operator wstring_view() const
    {
        return wstring_view(c_str(), length());
    }

    operator std::wstring() const
    {
        return std::wstring(c_str(), length());
    }

    operator CStringW() const
    {
        return CStringW(c_str(), length());
    }

private:
    const wchar_t* const m_text;
    mutable size_t m_length;
};



// ------------------------------------------------------
// NullTerminatedStringView
// 
// this class is similar to the above one except that it
// wraps the null-terminated string as a string view, so 
// the calculation hit of determining the length of the
// string, if necessary, is immediately incurred
// ------------------------------------------------------

class NullTerminatedStringView : public wstring_view
{
public:
    NullTerminatedStringView(const std::wstring& text)
        :   wstring_view(text)
    {
        ASSERT80(c_str()[length()] == 0);
    }

    NullTerminatedStringView(const CString& text)
        :   wstring_view(text)
    {
        ASSERT80(c_str()[length()] == 0);
    }

    NullTerminatedStringView(const wchar_t* const text)
        :   wstring_view(text)
    {
        ASSERT80(c_str()[length()] == 0);
    }

    NullTerminatedStringView(NullTerminatedString text) noexcept
        :   wstring_view(text)
    {
        ASSERT80(c_str()[length()] == 0);
    }

    [[nodiscard]] constexpr const_pointer c_str() const noexcept
    {
        return data();
    }
};



inline NullTerminatedString::NullTerminatedString(const NullTerminatedStringView& text) noexcept
    :   m_text(text.c_str()),
        m_length(text.length())
{
}
