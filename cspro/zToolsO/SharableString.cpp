#include "StdAfx.h"
#include "SharableString.h"


std::string& SharableString::MakeModifiable()
{
    if( !HasModifiedString() )
    {
        // if this is the only use of an unmodified shared string, it can be promoted to modified
        if( HasModifiableStringCandidate() )
        {
            m_string = ModifiableString { std::move(std::get<IndexSharedUnmodified>(m_string)) };
        }

        // otherwise a new copy must be created
        else
        {
            m_string = ModifiableString { std::make_shared<std::string>(GetString()) };
        }
    }

    return *std::get<IndexSharedModifiable>(m_string).str;
}


template<typename T/* = std::string*/>
T SharableString::Release()
{
    std::shared_ptr<std::string> text;

    if( HasModifiedString() )
    {
        text = std::move(std::get<IndexSharedModifiable>(m_string).str);
    }

    else if( HasModifiableStringCandidate() )
    {
        text = std::move(std::get<IndexSharedUnmodified>(m_string));
    }

    else
    {
        return GetString();
    }

    ASSERT(text != nullptr);

    Reset();

    if constexpr(std::is_same_v<T, std::string>)
    {
        return std::move(*text);
    }

    else
    {
        return text;
    }
}

template CLASS_DECL_ZTOOLSO std::string SharableString::Release();
template CLASS_DECL_ZTOOLSO SharableString SharableString::Release();


std::string& SharableString::GetModifiedOrModifiableString()
{
    ASSERT(HasModifiedOrModifiableString());

    if( HasModifiableStringCandidate() )
        m_string = ModifiableString { std::move(std::get<IndexSharedUnmodified>(m_string)) };

    return *std::get<IndexSharedModifiable>(m_string).str;
}


SharableString& SharableString::WideMakeExactLength(const size_t wide_length)
{
    const ptrdiff_t length_difference = wide_length - WideLength();

    if( length_difference != 0 )
        SO::WideMakeExactLengthAdjuster(MakeModifiable(), length_difference);

    return *this;
}


SharableString& SharableString::MakeUpper()
{
    return MakeCaseWorker<true>();
}


SharableString& SharableString::MakeLower()
{
    return MakeCaseWorker<false>();
}


template<bool ToUpper>
SharableString& SharableString::MakeCaseWorker()
{
    if( HasModifiedOrModifiableString() )
    {
        std::string& text = GetModifiedOrModifiableString();
        std::string* modified_case_string = &text;
        SO::MakeWideCaseWorker<ToUpper>(text, modified_case_string);
    }

    else
    {
        // only modify the string if the case actually changes
        std::string* modified_case_string = nullptr;

        SO::MakeWideCaseWorker<ToUpper>(GetString(), modified_case_string);

        if( modified_case_string != nullptr )
            m_string = std::shared_ptr<std::string>(modified_case_string);
    }

    return *this;
}
