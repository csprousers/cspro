#pragma once

#include <zToolsO/zToolsO.h>

class NullTerminatedString;

#ifdef WIN32
#define INTERFACE_USES_WIDE_CHARS
#endif


// --------------------------------------------------------------------------
// InterfaceString
//
// this class holds:
//  - a string object itself (as a unique pointer),
//  - a pointer to a string object, or
//  - a pointer to a null-terminated string
//
// the string is stored as a std::string or std::wstring, in the format
// necessary for interface calls specific to the platform
//
// this object should only be used in function parameters as a way to wrap
// the string as the class does not always store the memory for the string
//
// - the GetString method returns the string in the platform-specific format,
//   but can also return a string in a converted format
//
// - the GetStringView method returns a std::string_view or std::wstring_view
//
// - the Release method moves a modifiable string or creates a copy of a
//   non-modifiable string in the platform-specific format, but can also
//   return a string in a converted format
//
// - the c_str_utf8 method returns a UTF-8 null-terminated string that can
//   be used as an input to FormatText
//
// look at cs::string_sz for a similar class intended for function parameters
// --------------------------------------------------------------------------

class InterfaceString
{
#ifdef INTERFACE_USES_WIDE_CHARS
    using PlatformCharT = wchar_t;
    using OtherPlatformCharT = char;
#else
    using PlatformCharT = char;
    using OtherPlatformCharT = wchar_t;
#endif

    using PlatformStringT = std::basic_string<PlatformCharT>;
    using OtherStringT = std::basic_string<OtherPlatformCharT>;

public:
    InterfaceString();
    InterfaceString(const InterfaceString& rhs) noexcept;
    InterfaceString(InterfaceString&& rhs) noexcept = default;

    InterfaceString(const PlatformStringT& text) noexcept;
    InterfaceString(PlatformStringT&& text);
    InterfaceString(const PlatformCharT* text) noexcept;

    CLASS_DECL_ZTOOLSO InterfaceString(const OtherStringT& text);
    CLASS_DECL_ZTOOLSO InterfaceString(const OtherPlatformCharT* text);

#ifdef USING_CSTRING
    InterfaceString(const CString& text);
#endif

    CLASS_DECL_ZTOOLSO InterfaceString(const NullTerminatedString& text);

    template<typename RT = PlatformStringT>
    const RT& GetString() const;

    std::basic_string_view<PlatformCharT> GetStringView() const;

    template<typename RT = PlatformStringT>
    RT Release();

    const char* c_str_utf8() const;

    // std::string / std::wstring methods
    [[nodiscard]] const PlatformCharT* c_str() const noexcept;

    [[nodiscard]] const PlatformCharT* data() const noexcept { return c_str(); }

    [[nodiscard]] size_t length() const;

    [[nodiscard]] bool empty() const noexcept;

    [[nodiscard]] PlatformCharT front() const;
    [[nodiscard]] PlatformCharT back() const;

private:
    template<typename RT, typename T>
    static std::unique_ptr<RT> CreateString(const T& text);

    CLASS_DECL_ZTOOLSO void EnsurePlatformStringExists() const;
    CLASS_DECL_ZTOOLSO void EnsureOtherStringExists() const;

private:
    using InterfaceStringT = std::variant<std::unique_ptr<PlatformStringT>, const PlatformStringT*, const PlatformCharT*>;
    InterfaceStringT m_text;

    mutable std::unique_ptr<std::tuple<std::unique_ptr<PlatformStringT>, std::unique_ptr<OtherStringT>>> m_calculatedStrings;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline InterfaceString::InterfaceString()
#ifdef INTERFACE_USES_WIDE_CHARS
    :   InterfaceString(L"")
#else
    :   InterfaceString("")
#endif
{
}


inline InterfaceString::InterfaceString(const InterfaceString& rhs) noexcept
    :   m_text(( rhs.m_text.index() == 0 ) ? InterfaceStringT(std::get<0>(rhs.m_text).get()) :
               ( rhs.m_text.index() == 1 ) ? InterfaceStringT(std::get<1>(rhs.m_text)) :
                                             InterfaceStringT(std::get<2>(rhs.m_text)))
{
}


inline InterfaceString::InterfaceString(const PlatformStringT& text) noexcept
    :   m_text(&text)
{
}


inline InterfaceString::InterfaceString(PlatformStringT&& text)
    :   m_text(std::make_unique<PlatformStringT>(std::move(text)))
{
}


inline InterfaceString::InterfaceString(const PlatformCharT* const text) noexcept
    :   m_text(text)
{
    ASSERT(std::get<2>(m_text) != nullptr);
}


#ifdef USING_CSTRING
inline InterfaceString::InterfaceString(const CString& text)
    :   InterfaceString(text.GetString())
{
}
#endif


template<typename RT/* = PlatformStringT*/>
const RT& InterfaceString::GetString() const
{
    if constexpr(std::is_same_v<RT, PlatformStringT>)
    {
        if( m_text.index() == 0 )
        {
            return *std::get<0>(m_text);
        }

        else if( m_text.index() == 1 )
        {
            return *std::get<1>(m_text);
        }

        else
        {
            EnsurePlatformStringExists();
            return *std::get<0>(*m_calculatedStrings);
        }
    }

    else
    {
        EnsureOtherStringExists();
        return *std::get<1>(*m_calculatedStrings);
    }
}


inline std::basic_string_view<InterfaceString::PlatformCharT> InterfaceString::GetStringView() const
{
    if( m_text.index() == 0 )
    {
        return *std::get<0>(m_text);
    }

    else if( m_text.index() == 1 )
    {
        return *std::get<1>(m_text);
    }

    else if( m_calculatedStrings != nullptr && std::get<0>(*m_calculatedStrings) != nullptr )
    {
        return *std::get<0>(*m_calculatedStrings);
    }

    else
    {
        return std::get<2>(m_text);
    }
}


template<typename RT/* = PlatformStringT*/>
RT InterfaceString::Release()
{
    if constexpr(std::is_same_v<RT, PlatformStringT>)
    {
        if( m_text.index() == 0 )
        {
            return std::move(*std::get<0>(m_text));
        }

        else if( m_text.index() == 1 )
        {
            return *std::get<1>(m_text);
        }

        else if( m_calculatedStrings != nullptr && std::get<0>(*m_calculatedStrings) != nullptr )
        {
            return std::move(*std::get<0>(*m_calculatedStrings));
        }

        else
        {
            return std::get<2>(m_text);
        }
    }

    else
    {
        EnsureOtherStringExists();
        return std::move(*std::get<1>(*m_calculatedStrings));
    }
}


inline const char* InterfaceString::c_str_utf8() const
{
#ifdef INTERFACE_USES_WIDE_CHARS
    EnsureOtherStringExists();
    return std::get<1>(*m_calculatedStrings)->c_str();
#else
    return c_str();
#endif
}


inline const InterfaceString::PlatformCharT* InterfaceString::c_str() const noexcept
{
    switch( m_text.index() )
    {
        case 0:  return std::get<0>(m_text)->c_str();
        case 1:  return std::get<1>(m_text)->c_str();
        case 2:
        default: return std::get<2>(m_text);
    }
}


inline size_t InterfaceString::length() const
{
    if( m_text.index() == 0 )
    {
        return std::get<0>(m_text)->length();
    }

    else if( m_text.index() == 1 )
    {
        return std::get<1>(m_text)->length();
    }

    else
    {
#ifdef INTERFACE_USES_WIDE_CHARS
        return wcslen(std::get<2>(m_text));
#else
        return strlen(std::get<2>(m_text));
#endif
    }
}


inline bool InterfaceString::empty() const noexcept
{
    return ( *c_str() == '\0' );
}


inline InterfaceString::PlatformCharT InterfaceString::front() const
{
    ASSERT(!empty());

    return *c_str();
}


inline InterfaceString::PlatformCharT InterfaceString::back() const
{
    ASSERT(!empty());

    switch( m_text.index() )
    {
        case 0:  return std::get<0>(m_text)->back();
        case 1:  return std::get<1>(m_text)->back();
        case 2:
        default: return c_str()[length() - 1];
    }
}

#undef INTERFACE_USES_WIDE_CHARS
