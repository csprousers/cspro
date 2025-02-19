#pragma once

#include <zToolsO/zToolsO.h>
#include <zToolsO/StringOperations.h>


// --------------------------------------------------------------------------
// SharableString holds either:
//
// - [0] std::monostate: a blank string, which can also be considered an
//       unset state
//
// - [1] const std::string*: a pointer to a string that is guaranteed not
//       to change for the lifetime of the SharableString
//
// - [2] std::shared_ptr<const std::string>: a non-null string that cannot
//       be modified
//
// - [3] std::shared_ptr<std::string>: a non-null string that can be modified
//       but has not been marked for modification yet
//
// - [4] std::shared_ptr<std::string>: a non-null string that has been
//       marked for modification and should only be held by this object
//
// - If constructed from a std::optional<std::string>, objects without a
//   value will be set to std::monostate.
//
// - The Reset method resets the string to its unset (std::monostate) state,
//   whereas the ResetToBlank method resets the string to a set, blank,
//   state.
//
// - The MakeModifiable method ensures that the string is modifiable.
//
// - The Release method moves a modifiable string or creates a copy of a
//   non-modifiable string.
// --------------------------------------------------------------------------

class SharableString
{
    static constexpr size_t IndexMonostate        = 0;
    static constexpr size_t IndexStaticPointer    = 1;
    static constexpr size_t IndexSharedConst      = 2;
    static constexpr size_t IndexSharedUnmodified = 3;
    static constexpr size_t IndexSharedModifiable = 4;

    struct ModifiableString { std::shared_ptr<std::string> str; };

    using SharableStringT = std::variant<std::monostate,
                                         const std::string*,
                                         std::shared_ptr<const std::string>,
                                         std::shared_ptr<std::string>,
                                         ModifiableString>;

public:
    SharableString()
        :   m_string(std::monostate())
    {
        ASSERT81(m_string.index() == IndexMonostate);
    }

    SharableString(std::monostate /*value*/)
        :   m_string(std::monostate())
    {
        ASSERT81(m_string.index() == IndexMonostate);
    }

private:
    SharableString(const std::string* const value)
        :   m_string(value)
    {
        ASSERT81(m_string.index() == IndexStaticPointer);
        ASSERT(std::get<IndexStaticPointer>(m_string) != nullptr);
    }

public:
    // the following two methods are static so that uses of it are very explicit
    static SharableString FromStaticStringPointer(const std::string* const value)
    {
        return SharableString(value);
    }

    static SharableString CreateBlankString()
    {
        return SharableString(&SO::Empty_string);
    }

    template<typename T, class = typename std::enable_if<!std::is_same<std::remove_cvref_t<T>, SharableString>::value>::type>
    SharableString(T&& value)
        :   m_string(std::make_shared<std::string>(std::forward<T>(value)))
    {
        ASSERT81(m_string.index() == IndexSharedUnmodified);
        ASSERT(std::get<IndexSharedUnmodified>(m_string) != nullptr);
    }

    SharableString(std::shared_ptr<const std::string> value)
        :   m_string(std::move(value))
    {
        ASSERT81(m_string.index() == IndexSharedConst);
        ASSERT(std::get<IndexSharedConst>(m_string) != nullptr);
    }

    SharableString(std::shared_ptr<std::string> value)
        :   m_string(std::move(value))
    {
        ASSERT81(m_string.index() == IndexSharedUnmodified);
        ASSERT(std::get<IndexSharedUnmodified>(m_string) != nullptr);
    }

    SharableString(std::unique_ptr<std::string> value)
        :   m_string(std::shared_ptr<std::string>(std::move(value)))
    {
        ASSERT81(m_string.index() == IndexSharedUnmodified);
        ASSERT(std::get<IndexSharedUnmodified>(m_string) != nullptr);
    }

    SharableString(std::optional<std::string> value)
        :   m_string(value.has_value() ? SharableStringT(std::make_shared<std::string>(std::move(*value))) :
                                         SharableStringT(std::monostate()))
    {
        ASSERT81(m_string.index() == IndexSharedUnmodified || m_string.index() == IndexMonostate);
    }

    SharableString(const SharableString& rhs)
        :   m_string(rhs.HasModifiedString() ? SharableStringT(std::make_shared<std::string>(rhs.GetString())) :
                                               SharableStringT(rhs.m_string))
    {
    }

    SharableString(SharableString&& rhs) noexcept
        :   m_string(std::move(rhs.m_string))
    {
    }

    SharableString& operator=(const SharableString& rhs)
    {
        m_string = rhs.HasModifiedString() ? SharableStringT(std::make_shared<std::string>(rhs.GetString()))  :
                                             SharableStringT(rhs.m_string);
        return *this;
    }

    SharableString& operator=(SharableString&& rhs) noexcept
    {
        m_string = std::move(rhs.m_string);
        return *this;
    }

    [[nodiscard]] const std::string* operator->() const noexcept
    {
        switch( m_string.index() )
        {
            case IndexStaticPointer:    return std::get<IndexStaticPointer>(m_string);
            case IndexSharedConst:      return std::get<IndexSharedConst>(m_string).get();
            case IndexSharedUnmodified: return std::get<IndexSharedUnmodified>(m_string).get();
            case IndexSharedModifiable: return std::get<IndexSharedModifiable>(m_string).str.get();
            default:                    return &SO::Empty_string;
        }
    }

    [[nodiscard]] const std::string& operator*() const noexcept
    {
        return GetString();
    }

    [[nodiscard]] bool operator==(const SharableString& rhs) const noexcept
    {
        return ( GetString() == rhs.GetString() );
    }

    [[nodiscard]] bool operator==(const std::string& rhs) const noexcept
    {
        return ( GetString() == rhs );
    }

    [[nodiscard]] bool operator<(const SharableString& rhs) const noexcept
    {
        return ( GetString() < rhs.GetString() );
    }

    [[nodiscard]] bool operator>(const SharableString& rhs) const noexcept
    {
        return ( GetString() > rhs.GetString() );
    }

    bool IsSet() const noexcept
    {
        return ( m_string.index() != IndexMonostate );
    }

    void Reset()
    {
        m_string = std::monostate();
    }

    void ResetToBlank()
    {
        m_string = &SO::Empty_string;
        ASSERT81(m_string.index() == IndexStaticPointer);
    }

    const std::string& GetString() const
    {
        return *operator->();
    }

    std::string& MakeModifiable()
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

    std::string Release()
    {
        if( HasModifiedString() )
        {
            return std::move(*std::get<IndexSharedModifiable>(m_string).str);
        }

        else if( HasModifiableStringCandidate() )
        {
            return std::move(*std::get<IndexSharedUnmodified>(m_string));
        }

        else
        {
            return GetString();
        }
    }

private:
    bool HasModifiedString() const noexcept
    {
        return ( m_string.index() == IndexSharedModifiable );
    }

    bool HasModifiableStringCandidate() const noexcept
    {
        return ( m_string.index() == IndexSharedUnmodified &&
                 std::get<IndexSharedUnmodified>(m_string).use_count() == 1 );
    }

    bool HasModifiedOrModifiableString() const noexcept
    {
        return ( HasModifiedString() ||
                 HasModifiableStringCandidate() );
    }

    std::string& GetModifiedOrModifiableString()
    {
        ASSERT(HasModifiedOrModifiableString());

        if( HasModifiableStringCandidate() )
            m_string = ModifiableString { std::move(std::get<IndexSharedUnmodified>(m_string)) };

        return *std::get<IndexSharedModifiable>(m_string).str;
    }

private:
    SharableStringT m_string;


    // --------------------------------------------------------------------------
    // the following functionality wraps some SO methods
    // --------------------------------------------------------------------------
public:
    size_t WideLength() const;

    SharableString& WideMakeExactLength(size_t wide_length);

    SharableString& MakeUpper();
    SharableString& MakeLower();

    template<typename... Args>
    SharableString& MakeTrim(Args const&... args);

    template<typename... Args>
    SharableString& MakeTrimRight(Args const&... args);

    SharableString& MakeTrimRightSpace() { return MakeTrimRight(' '); }

private:
    template<bool ToUpper>
    CLASS_DECL_ZTOOLSO SharableString& MakeCaseWorker();
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline size_t SharableString::WideLength() const
{
    return SO::WideLength(GetString());
}


inline SharableString& SharableString::WideMakeExactLength(const size_t wide_length)
{
    const ptrdiff_t length_difference = wide_length - WideLength();

    if( length_difference != 0 )
        SO::WideMakeExactLengthAdjuster(MakeModifiable(), length_difference);

    return *this;
}


inline SharableString& SharableString::MakeUpper()
{
    return MakeCaseWorker<true>();
}


inline SharableString& SharableString::MakeLower()
{
    return MakeCaseWorker<false>();
}


template<typename... Args>
SharableString& SharableString::MakeTrim(Args const&... args)
{
    const std::string& text = GetString();
    const std::string_view trimmed_text_sv = SO::Trim(text, args...);

    if( trimmed_text_sv.length() != text.length() )
        m_string = std::make_shared<std::string>(trimmed_text_sv);

    return *this;
}


template<typename... Args>
SharableString& SharableString::MakeTrimRight(Args const&... args)
{
    const std::string& text = GetString();
    const std::string_view trimmed_text_sv = SO::TrimRight(text, args...);

    if( trimmed_text_sv.length() != text.length() )
    {
        if( HasModifiedOrModifiableString() )
        {
            GetModifiedOrModifiableString().resize(trimmed_text_sv.length());
        }

        else
        {
            m_string = std::make_shared<std::string>(trimmed_text_sv);
        }
    }

    return *this;
}
