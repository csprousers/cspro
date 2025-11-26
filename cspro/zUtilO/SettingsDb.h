#pragma once

#include <zUtilO/zUtilO.h>
#include <zJson/Json.h>

namespace CSProExecutables { enum class Program; }


// --------------------------------------------------------------------------
// SettingsDb
//
// A simple way to store settings that persist across application runs:
//
// - The settings are stored in a SQLite database in: %AppData%/CSPro
//
// - Settings are queried on demand, and potentially cached to minimize
//   hits to the database.
//
// - Settings can be set to expire at some point.
//
// - Modifications are only written to the database on application close
//   unless otherwise specified.
//
// - The keys used to store settings can be obfuscated.
//
// - No exceptions are thrown at any point.
//
// - Look at SimpleDbMap and WinRegistry for similar functionality.
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO SettingsDb
{
public:
    enum class KeyObfuscator { Hash };

    SettingsDb(std::string_view filename_only_sv, std::string settings_name = "settings",
               std::optional<int64_t> expiration_seconds = std::nullopt,
               std::optional<KeyObfuscator> key_obfuscator = std::nullopt) noexcept;

    SettingsDb(CSProExecutables::Program program, std::string settings_name = "settings",
               std::optional<int64_t> expiration_seconds = std::nullopt,
               std::optional<KeyObfuscator> key_obfuscator = std::nullopt) noexcept;

    // The Read and Write methods are defined for most basic types.
    // If not a basic type and a JSON serializer exists, the object is serialized to JSON.
    // If neither of the above are true, the object is interpreted as a std::string.
    template<typename T>
    constexpr static bool IsBasicType() noexcept;

    // If T is a pointer, the return type will be const T* and the type must be cachable.
    // Otherwise the return type will be std::optional<T>.
    template<typename T>
    [[nodiscard]] auto Read(std::string_view key_sv, bool cache_value = true) noexcept;

    template<typename T, class = typename std::enable_if<!std::is_lvalue_reference<T>::value>::type>
    [[nodiscard]] T ReadOrDefault(std::string_view key_sv, T&& default_value = T(), bool cache_value = true) noexcept;

    template<typename T>
    [[nodiscard]] T ReadOrDefault(std::string_view key_sv, const T& default_value, bool cache_value = true) noexcept;

    template<typename T>
    void Write(std::string_view key_sv, const T& value, bool cache_value = true) noexcept;

private:
    template<typename T>
    [[nodiscard]] const T* ReadPointer(std::string_view key_sv) noexcept;

    template<typename T>
    std::optional<T> ReadWorker(std::string_view key_sv, bool cache_value) noexcept;

    template<typename T>
    void WriteWorker(std::string_view key_sv, const T& value, bool cache_value) noexcept;

private:
    class ImplDb;
    struct ImplTable;
    class ImplCache;

    ImplDb* m_implDb;
    ImplTable* m_implTable;
    std::optional<int64_t> m_expirationSeconds;
    std::shared_ptr<KeyObfuscator> m_keyObfuscator;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
constexpr bool SettingsDb::IsBasicType() noexcept
{
    return ( std::is_same_v<T, bool> ||
             std::is_same_v<T, int> ||
             std::is_same_v<T, unsigned int> ||
             std::is_same_v<T, int64_t> ||
             std::is_same_v<T, size_t> ||
             std::is_same_v<T, float> ||
             std::is_same_v<T, double> ||
             std::is_same_v<T, std::string> );
}


template<typename T>
const T* SettingsDb::ReadPointer(const std::string_view key_sv) noexcept
{
    const std::optional<const T*> value_ptr = ReadWorker<const T*>(key_sv, true);
    return value_ptr.value_or(nullptr);
}


template<typename T>
auto SettingsDb::Read(const std::string_view key_sv, const bool cache_value/* = true*/) noexcept
{
    if constexpr(std::is_pointer_v<T>)
    {
        using RealType = std::remove_const_t<std::remove_pointer_t<T>>;
        static_assert(IsBasicType<RealType>());
        ASSERT(cache_value);

        return ReadPointer<RealType>(key_sv);
    }

    else
    {
        if constexpr(JsonSerializerTester<T>::HasCreateFromJson())
        {
            std::optional<T> value;

            auto parse_json_text = [&](const std::string& json_text)
            {
                try
                {
                    const JsonNode json_node = Json::Parse(json_text);
                    value = json_node.Get<T>();
                }
                catch(...) { ASSERT(false); }
            };

            if( cache_value )
            {
                const std::string* const json_text = ReadPointer<std::string>(key_sv);

                if( json_text != nullptr )
                    parse_json_text(*json_text);
            }

            else
            {
                const std::optional<std::string> json_text = ReadWorker<std::string>(key_sv, cache_value);

                if( json_text.has_value() )
                    parse_json_text(*json_text);
            }

            return value;
        }

        else
        {
            return ReadWorker<T>(key_sv, cache_value);
        }
    }
}


template<typename T, class/* = typename std::enable_if<!std::is_lvalue_reference<T>::value>::type*/>
T SettingsDb::ReadOrDefault(const std::string_view key_sv, T&& default_value/* = T()*/, const bool cache_value/* = true*/) noexcept
{
    static_assert(!std::is_pointer_v<T>);

    std::optional<T> value = Read<T>(key_sv, cache_value);

    if( value.has_value() )
        return std::move(*value);

    return std::forward<T>(default_value);
}


template<typename T>
T SettingsDb::ReadOrDefault(const std::string_view key_sv, const T& default_value, const bool cache_value/* = true*/) noexcept
{
    static_assert(!std::is_pointer_v<T>);

    std::optional<T> value = Read<T>(key_sv, cache_value);

    if( value.has_value() )
        return std::move(*value);

    return default_value;
}


template<typename T>
void SettingsDb::Write(const std::string_view key_sv, const T& value, const bool cache_value/* = true*/) noexcept
{
    if constexpr(IsBasicType<T>())
    {
        WriteWorker(key_sv, value, cache_value);
    }

    else if constexpr(JsonSerializerTester<T>::HasWriteJson())
    {
        try
        {
            WriteWorker(key_sv, Json::ToJson(value), cache_value);
        }
        catch(...) { ASSERT(false); }
    }

    else
    {
        WriteWorker(key_sv, std::string(static_cast<std::string_view>(value)), cache_value);
    }
}
