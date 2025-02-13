#pragma once

#include <zUtilO/zUtilO.h>
#include <zUtilO/CSProExecutables.h>
#include <zJson/Json.h>


// --------------------------------------------------------------------------
// SettingsDb
// 
// a simple way to store settings that persist across application runs:
// 
// - the settings are stored in a SQLite database in %AppData%/CSPro
// 
// - settings are queried on demand, and potentially cached to minimize
//   hits to the database
//
// - settings can be set to expire at some point
// 
// - modifications are only written to the database on application close
//   unless otherwise specified
//
// - the keys used to store settings can be obfuscated
// 
// - no exceptions are thrown at any point
// 
// - look at SimpleDbMap and WinRegistry for similar functionality
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO SettingsDb
{
public:
    enum class KeyObfuscator { Hash };

    SettingsDb(const std::wstring& filename_only, std::wstring settings_name = _T("settings"),
               std::optional<int64_t> expiration_seconds = std::nullopt, std::optional<KeyObfuscator> key_obfuscator = std::nullopt);

    SettingsDb(CSProExecutables::Program program, std::wstring settings_name = _T("settings"),
               std::optional<int64_t> expiration_seconds = std::nullopt, std::optional<KeyObfuscator> key_obfuscator = std::nullopt);

    // the Read and Write methods are defined for most basic types;
    // if not a basic type and a JSON serializer exists, the object is serialized to JSON;
    // if neither of the above are true, the object interpreted as a std::wstring
    template<typename T>
    constexpr static bool IsBasicType()
    {
        return constexpr(std::is_same_v<T, bool> ||
                         std::is_same_v<T, int> ||
                         std::is_same_v<T, unsigned int> ||
                         std::is_same_v<T, int64_t> ||
                         std::is_same_v<T, size_t> ||
                         std::is_same_v<T, float> ||
                         std::is_same_v<T, double> ||
                         std::is_same_v<T, std::wstring>);
    }

    // if T is a pointer, the return type will be const T* and the type must be cachable;
    // otherwise the return type will be std::optional<T>
    template<typename T> 
    [[nodiscard]] auto Read(wstring_view key, bool cache_value = true);

    template<typename T, class = typename std::enable_if<!std::is_lvalue_reference<T>::value>::type>
    [[nodiscard]] T ReadOrDefault(wstring_view key, T&& default_value = T(), bool cache_value = true);

    template<typename T>
    [[nodiscard]] T ReadOrDefault(wstring_view key, const T& default_value, bool cache_value = true);

    template<typename T>
    void Write(wstring_view key, const T& value, bool cache_value = true);

private:
    template<typename T>
    std::optional<T> ReadWorker(wstring_view key, bool cache_value);

    template<typename T>
    void WriteWorker(wstring_view key, const T& value, bool cache_value);

public:
    class ImplDb;
    struct ImplTable;
    class ImplCache;

private:
    ImplDb* m_implDb;
    ImplTable* m_implTable;
    std::optional<int64_t> m_expirationSeconds;
    std::shared_ptr<KeyObfuscator> m_keyObfuscator;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

template<typename T>
auto SettingsDb::Read(wstring_view key, bool cache_value/* = true*/)
{
    if constexpr(std::is_pointer_v<T>)
    {
        using RealType = std::remove_const_t<std::remove_pointer_t<T>>;
        static_assert(IsBasicType<RealType>());
        ASSERT(cache_value);

        std::optional<const RealType*> value_ptr = ReadWorker<const RealType*>(key, cache_value);
        return value_ptr.value_or(nullptr);
    }

    else
    {
        if constexpr(JsonSerializerTester<T>::HasCreateFromJson())
        {
            std::optional<T> value;

            auto parse_json_text = [&](const std::wstring& json_text)
            {
                try
                {
                    auto json_node = Json::Parse(json_text);
                    value = json_node.Get<T>();
                }
                catch(...) { }
            };

            if( cache_value )
            {
                const std::wstring* json_text = Read<const std::wstring*>(key, cache_value);

                if( json_text != nullptr )
                    parse_json_text(*json_text);
            }

            else
            {
                std::optional<std::wstring> json_text = Read<std::wstring>(key, cache_value);

                if( json_text.has_value() )
                    parse_json_text(*json_text);
            }

            return value;
        }

        else
        {
            return ReadWorker<T>(key, cache_value);
        }
    }
}


template<typename T, class/* = typename std::enable_if<!std::is_lvalue_reference<T>::value>::type*/>
T SettingsDb::ReadOrDefault(wstring_view key, T&& default_value/* = T()*/, bool cache_value/* = true*/)
{
    static_assert(!std::is_pointer_v<T>);

    std::optional<T> value = Read<T>(key, cache_value);

    if( value.has_value() )
        return std::move(*value);

    return std::forward<T>(default_value);
}


template<typename T>
T SettingsDb::ReadOrDefault(wstring_view key, const T& default_value, bool cache_value/* = true*/)
{
    static_assert(!std::is_pointer_v<T>);

    std::optional<T> value = Read<T>(key, cache_value);

    if( value.has_value() )
        return std::move(*value);

    return default_value;
}


template<typename T>
void SettingsDb::Write(wstring_view key, const T& value, bool cache_value/* = true*/)
{
    if constexpr(IsBasicType<T>())
    {
        WriteWorker(key, value, cache_value);
    }

    else if constexpr(JsonSerializerTester<T>::HasWriteJson())
    {
        try
        {
            auto json_writer = Json::CreateStringWriter();
            json_writer->Write(value);
            WriteWorker(key, json_writer->GetString(), cache_value);
        }
        catch(...) { }
    }

    else
    {
        WriteWorker(key, std::wstring(static_cast<wstring_view>(value)), cache_value);
    }
}
