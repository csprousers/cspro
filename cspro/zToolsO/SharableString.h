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
    SharableString();
    SharableString(std::monostate value);

private:
    SharableString(const std::string* value);

public:
    // the following two methods are static so that uses of it are very explicit
    static SharableString FromStaticStringPointer(const std::string* value);
    static SharableString CreateBlankString();

    template<typename T, class = typename std::enable_if<!std::is_same<std::remove_cvref_t<T>, SharableString>::value>::type>
    SharableString(T&& value);

    SharableString(std::shared_ptr<const std::string> value);
    SharableString(std::shared_ptr<std::string> value);
    SharableString(std::unique_ptr<std::string> value);
    SharableString(std::optional<std::string> value);
    SharableString(const SharableString& rhs);
    SharableString(SharableString&& rhs) noexcept;

    SharableString& operator=(const SharableString& rhs);
    SharableString& operator=(SharableString&& rhs) noexcept;

    [[nodiscard]] const std::string* operator->() const noexcept;
    [[nodiscard]] const std::string& operator*() const noexcept;

    [[nodiscard]] bool operator==(const SharableString& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const SharableString& rhs) const noexcept;
    [[nodiscard]] bool operator==(const std::string& rhs) const noexcept;
    [[nodiscard]] bool operator!=(const std::string& rhs) const noexcept;
    [[nodiscard]] bool operator<(const SharableString& rhs) const noexcept;
    [[nodiscard]] bool operator>(const SharableString& rhs) const noexcept;

    bool IsSet() const noexcept;

    void Reset();
    void ResetToBlank();

    const std::string& GetString() const;

    CLASS_DECL_ZTOOLSO std::string& MakeModifiable();

    template<typename T = std::string> // can also return SharableString
    CLASS_DECL_ZTOOLSO T Release();

    // --------------------------------------------------------------------------
    // the following functionality wraps some SO methods
    // --------------------------------------------------------------------------
public:
    size_t WideLength() const;

    CLASS_DECL_ZTOOLSO SharableString& WideMakeExactLength(size_t wide_length);

    CLASS_DECL_ZTOOLSO SharableString& MakeUpper();
    CLASS_DECL_ZTOOLSO SharableString& MakeLower();

    template<typename... Args>
    SharableString& MakeTrim(Args const&... args);

    template<typename... Args>
    SharableString& MakeTrimRight(Args const&... args);

    SharableString& MakeTrimRightSpace() { return MakeTrimRight(' '); }

private:
    bool HasModifiedString() const noexcept;
    bool HasModifiableStringCandidate() const noexcept;
    bool HasModifiedOrModifiableString() const noexcept;
    CLASS_DECL_ZTOOLSO std::string& GetModifiedOrModifiableString();

    template<bool to_upper>
    CLASS_DECL_ZTOOLSO SharableString& MakeCaseWorker();

private:
    SharableStringT m_string;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SharableString::SharableString()
    :   m_string(std::monostate())
{
    ASSERT81(m_string.index() == IndexMonostate);
}


inline SharableString::SharableString(std::monostate /*value*/)
    :   m_string(std::monostate())
{
    ASSERT81(m_string.index() == IndexMonostate);
}


inline SharableString::SharableString(const std::string* const value)
    :   m_string(value)
{
    ASSERT81(m_string.index() == IndexStaticPointer);
    ASSERT(std::get<IndexStaticPointer>(m_string) != nullptr);
}


inline SharableString SharableString::FromStaticStringPointer(const std::string* const value)
{
    return SharableString(value);
}


inline SharableString SharableString::CreateBlankString()
{
    return SharableString(&SO::Empty_string);
}


template<typename T, class/* = typename std::enable_if<!std::is_same<std::remove_cvref_t<T>, SharableString>::value>::type*/>
SharableString::SharableString(T&& value)
    :   m_string(std::make_shared<std::string>(std::forward<T>(value)))
{
    ASSERT81(m_string.index() == IndexSharedUnmodified);
    ASSERT(std::get<IndexSharedUnmodified>(m_string) != nullptr);
}


inline SharableString::SharableString(std::shared_ptr<const std::string> value)
    :   m_string(std::move(value))
{
    ASSERT81(m_string.index() == IndexSharedConst);
    ASSERT(std::get<IndexSharedConst>(m_string) != nullptr);
}


inline SharableString::SharableString(std::shared_ptr<std::string> value)
    :   m_string(std::move(value))
{
    ASSERT81(m_string.index() == IndexSharedUnmodified);
    ASSERT(std::get<IndexSharedUnmodified>(m_string) != nullptr);
}


inline SharableString::SharableString(std::unique_ptr<std::string> value)
    :   m_string(std::shared_ptr<std::string>(std::move(value)))
{
    ASSERT81(m_string.index() == IndexSharedUnmodified);
    ASSERT(std::get<IndexSharedUnmodified>(m_string) != nullptr);
}


inline SharableString::SharableString(std::optional<std::string> value)
    :   m_string(value.has_value() ? SharableStringT(std::make_shared<std::string>(std::move(*value))) :
                                     SharableStringT(std::monostate()))
{
    ASSERT81(m_string.index() == IndexSharedUnmodified || m_string.index() == IndexMonostate);
}


inline SharableString::SharableString(const SharableString& rhs)
    :   m_string(rhs.HasModifiedString() ? SharableStringT(std::make_shared<std::string>(rhs.GetString())) :
                                           SharableStringT(rhs.m_string))
{
}


inline SharableString::SharableString(SharableString&& rhs) noexcept
    :   m_string(std::move(rhs.m_string))
{
}


inline SharableString& SharableString::operator=(const SharableString& rhs)
{
    m_string = rhs.HasModifiedString() ? SharableStringT(std::make_shared<std::string>(rhs.GetString()))  :
                                         SharableStringT(rhs.m_string);
    return *this;
}


inline SharableString& SharableString::operator=(SharableString&& rhs) noexcept
{
    m_string = std::move(rhs.m_string);
    return *this;
}


[[nodiscard]] inline const std::string* SharableString::operator->() const noexcept
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


[[nodiscard]] inline const std::string& SharableString::operator*() const noexcept
{
    return GetString();
}


[[nodiscard]] inline bool SharableString::operator==(const SharableString& rhs) const noexcept
{
    return ( GetString() == rhs.GetString() );
}


[[nodiscard]] inline bool SharableString::operator!=(const SharableString& rhs) const noexcept
{
    return !operator==(rhs);
}


[[nodiscard]] inline bool SharableString::operator==(const std::string& rhs) const noexcept
{
    return ( GetString() == rhs );
}


[[nodiscard]] inline bool SharableString::operator!=(const std::string& rhs) const noexcept
{
    return !operator==(rhs);
}


[[nodiscard]] inline bool SharableString::operator<(const SharableString& rhs) const noexcept
{
    return ( GetString() < rhs.GetString() );
}


[[nodiscard]] inline bool SharableString::operator>(const SharableString& rhs) const noexcept
{
    return ( GetString() > rhs.GetString() );
}


inline bool SharableString::IsSet() const noexcept
{
    return ( m_string.index() != IndexMonostate );
}


inline void SharableString::Reset()
{
    m_string = std::monostate();
}


inline void SharableString::ResetToBlank()
{
    m_string = &SO::Empty_string;
    ASSERT81(m_string.index() == IndexStaticPointer);
}


inline const std::string& SharableString::GetString() const
{
    return *operator->();
}


inline bool SharableString::HasModifiedString() const noexcept
{
    return ( m_string.index() == IndexSharedModifiable );
}


inline bool SharableString::HasModifiableStringCandidate() const noexcept
{
    return ( m_string.index() == IndexSharedUnmodified &&
             std::get<IndexSharedUnmodified>(m_string).use_count() == 1 );
}


inline bool SharableString::HasModifiedOrModifiableString() const noexcept
{
    return ( HasModifiedString() ||
             HasModifiableStringCandidate() );
}


inline size_t SharableString::WideLength() const
{
    return SO::WideLength(GetString());
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
