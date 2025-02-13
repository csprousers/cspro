#pragma once

#include <zAppO/zAppO.h>

template<typename CharType> class JsonNode;
class JsonWriter;
class Serializer;


class ZAPPO_API JsonProperties
{
public:
    enum class JsonFormat : int { Compact, Pretty };
    static constexpr JsonFormat DefaultJsonFormat = JsonFormat::Compact;
    
    enum class ArrayFormat : int { Full, Sparse };
    static constexpr ArrayFormat DefaultArrayFormat = ArrayFormat::Full;

    enum class HashMapFormat : int { Array, Object };
    static constexpr HashMapFormat DefaultHashMapFormat = HashMapFormat::Object;

    enum class BinaryDataFormat : int { DataUrl, LocalhostUrl };
    static constexpr BinaryDataFormat DefaultBinaryDataFormat = BinaryDataFormat::LocalhostUrl;

    bool operator==(const JsonProperties& rhs) const;
    bool operator!=(const JsonProperties& rhs) const { return !( *this == rhs ); }

    JsonFormat GetJsonFormat() const      { return m_jsonFormat; }
    void SetJsonFormat(JsonFormat format) { m_jsonFormat = format; }

    ArrayFormat GetArrayFormat() const      { return m_arrayFormat; }
    void SetArrayFormat(ArrayFormat format) { m_arrayFormat = format; }

    HashMapFormat GetHashMapFormat() const      { return m_hashMapFormat; }
    void SetHashMapFormat(HashMapFormat format) { m_hashMapFormat = format; }

    BinaryDataFormat GetBinaryDataFormat() const      { return m_binaryDataFormat; }
    void SetBinaryDataFormat(BinaryDataFormat format) { m_binaryDataFormat = format; }


    // serialization
    // --------------------------------------------------------------------------
    static JsonProperties CreateFromJson(const JsonNode<wchar_t>& json_node);
    void UpdateFromJson(const JsonNode<wchar_t>& json_node);
    void WriteJson(JsonWriter& json_writer) const;

    void serialize(Serializer& ar);


private:
    JsonFormat m_jsonFormat = DefaultJsonFormat;
    ArrayFormat m_arrayFormat = DefaultArrayFormat;
    HashMapFormat m_hashMapFormat = DefaultHashMapFormat;
    BinaryDataFormat m_binaryDataFormat = DefaultBinaryDataFormat;
};
