#include "StdAfx.h"
#include "InterfaceString.h"


InterfaceString::InterfaceString(const OtherStringT& text)
    :   m_text(CreateString<PlatformStringT>(text))
{
}


InterfaceString::InterfaceString(const OtherPlatformCharT* const text)
    :   m_text(CreateString<PlatformStringT>(text))
{
}


InterfaceString::InterfaceString(const NullTerminatedString& text) // UTF8_TODO remove
    :   InterfaceString(text.c_str())
{
}


template<typename RT, typename T>
std::unique_ptr<RT> InterfaceString::CreateString(const T& text)
{
    static_assert(!std::is_same_v<RT, T>);

    if constexpr(std::is_same_v<RT, std::wstring>)
    {
        return std::make_unique<RT>(TC::ToWide(text));
    }

    else
    {
        return std::make_unique<RT>(TC::ToUtf8(text));
    }
}


void InterfaceString::EnsurePlatformStringExists() const
{
    ASSERT(m_text.index() == 2);

    if( m_calculatedStrings == nullptr )
        m_calculatedStrings = std::make_unique<std::tuple<std::unique_ptr<PlatformStringT>, std::unique_ptr<OtherStringT>>>();

    if( std::get<0>(*m_calculatedStrings) == nullptr )
        std::get<0>(*m_calculatedStrings) = std::make_unique<PlatformStringT>(std::get<2>(m_text));
}


void InterfaceString::EnsureOtherStringExists() const
{
    if( m_calculatedStrings == nullptr )
        m_calculatedStrings = std::make_unique<std::tuple<std::unique_ptr<PlatformStringT>, std::unique_ptr<OtherStringT>>>();

    if( std::get<1>(*m_calculatedStrings) == nullptr )
        std::get<1>(*m_calculatedStrings) = CreateString<OtherStringT>(GetStringView());
}
