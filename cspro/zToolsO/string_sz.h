#pragma once

namespace cs { class string_sz; class string_view_sz; }


// --------------------------------------------------------------------------
// cs::string_sz
// cs::string_view_sz
//
// these classes hold a pointer to a null-terminated string
//
// string_view_sz also stores the length of the string
//
// these are only intended to be used by function parameters as a
// convenience so that std::string objects can be passed to functions
// without needing to call c_str() on the object
//
// look at InterfaceString for a similar class intended for function
// parameters
// --------------------------------------------------------------------------

class cs::string_sz
{
public:
    string_sz(const std::string& text) noexcept;
    string_sz(const char* text) noexcept;

    string_sz(const string_sz& text) noexcept = default;
    string_sz(string_sz&& text) noexcept = default;

    string_sz& operator=(const string_sz& text) = delete;
    string_sz& operator=(string_sz&& text) = delete;

    [[nodiscard]] const char* c_str() const noexcept;

    [[nodiscard]] const char* data() const noexcept;

    [[nodiscard]] size_t length() const;

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] char front() const;

    [[nodiscard]] char operator[](size_t offset) const;

protected:
    const char* const m_text;
};


class cs::string_view_sz : public cs::string_sz
{
public:
    string_view_sz(const std::string& text) noexcept;
    string_view_sz(const char* text, size_t length) noexcept;
    string_view_sz(const char* text);
    string_view_sz(const string_sz& text);

    string_view_sz(const string_view_sz& text) noexcept = default;
    string_view_sz(string_view_sz&& text) noexcept = default;

    string_view_sz& operator=(const string_view_sz& text) = delete;
    string_view_sz& operator=(string_view_sz&& text) = delete;

    [[nodiscard]] size_t length() const;

    [[nodiscard]] char back() const;

    [[nodiscard]] const char* cbegin() const noexcept;
    [[nodiscard]] const char* cend() const;

    static constexpr auto npos = std::string_view::npos;

    template<typename... Args>
    [[nodiscard]] std::string_view::size_type find(Args const&... args) const;

    template<typename T = string_view_sz>
    [[nodiscard]] T substr(std::string::size_type pos = 0) const;

    operator std::string_view() const;

    [[nodiscard]] std::string string() const; // no automatic casts to std::string

private:
    const size_t m_length;
};



// --------------------------------------------------------------------------
// cs::string_sz
//
// inline implementations
// --------------------------------------------------------------------------

inline cs::string_sz::string_sz(const std::string& text) noexcept
    :   m_text(text.c_str())
{
}


inline cs::string_sz::string_sz(const char* const text) noexcept
    :   m_text(text)
{
    ASSERT(m_text != nullptr);
}


inline const char* cs::string_sz::c_str() const noexcept
{
    return m_text;
}


inline const char* cs::string_sz::data() const noexcept
{
    return m_text;
}


inline size_t cs::string_sz::length() const
{
    return strlen(m_text);
}


inline bool cs::string_sz::empty() const noexcept
{
    return ( *m_text == '\0' );
}


inline char cs::string_sz::front() const
{
    ASSERT(!empty());

    return *m_text;
}


inline char cs::string_sz::operator[](const size_t offset) const
{
    ASSERT(offset <= length());

    return m_text[offset];
}



// --------------------------------------------------------------------------
// cs::string_view_sz
//
// inline implementations
// --------------------------------------------------------------------------

inline cs::string_view_sz::string_view_sz(const std::string& text) noexcept
    :   string_sz(text),
        m_length(text.length())
{
}


inline cs::string_view_sz::string_view_sz(const char* const text, const size_t length) noexcept
    :   string_sz(text),
        m_length(length)
{
    ASSERT(m_text[m_length] == '\0');
}


inline cs::string_view_sz::string_view_sz(const char* const text)
    :   string_view_sz(text, strlen(text))
{
}


inline cs::string_view_sz::string_view_sz(const string_sz& text)
    :   string_view_sz(text.c_str(), text.length())
{
}


inline size_t cs::string_view_sz::length() const
{
    return m_length;
}


inline char cs::string_view_sz::back() const
{
    ASSERT(!empty());

    return operator[](m_length - 1);
}


inline const char* cs::string_view_sz::cbegin() const noexcept
{
    return m_text;
}


inline const char* cs::string_view_sz::cend() const
{
    return m_text + length();
}


template<typename... Args>
std::string_view::size_type cs::string_view_sz::find(Args const&... args) const
{
    return std::string_view(m_text, m_length).find(args...);
}


template<typename T/* = cs::string_view_sz*/>
T cs::string_view_sz::substr(const std::string::size_type pos/* = 0*/) const
{
    ASSERT(pos <= m_length);

    return T(m_text + pos, m_length - pos);
}


inline cs::string_view_sz::operator std::string_view() const
{
    return std::string_view(m_text, m_length);
}


inline std::string cs::string_view_sz::string() const
{
    return std::string(m_text, m_length);
}
