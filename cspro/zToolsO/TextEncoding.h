#pragma once

#include <zToolsO/zToolsO.h>


// --------------------------------------------------------------------------
// TextEncoding
// --------------------------------------------------------------------------

class TextEncoding
{
public:
    class Converter;

    enum class Type : int { Ansi, Utf8, Utf8Bom, Utf16LE, Utf16BE };

    static constexpr Type DefaultEncoding           = Type::Utf8Bom; // will change to Utf8 in CSPro 9.0
    static constexpr Type DefaultEncodingIfNoBom    = Type::Utf8;

    static constexpr std::string_view NoBom_sv      = "";
    static constexpr std::string_view Utf8Bom_sv    = "\xEF\xBB\xBF";
    static constexpr std::string_view Utf16LEBom_sv = "\xFF\xFE";
    static constexpr std::string_view Utf16BEBom_sv = "\xFE\xFF";

    TextEncoding(Type type = DefaultEncoding);

    // Processes the beginning of the text / buffer to determine the BOM.
    TextEncoding(std::string_view text_sv, Type default_encoding_if_no_bom = DefaultEncodingIfNoBom);
    TextEncoding(const void* buffer, size_t buffer_length, Type default_encoding_if_no_bom = DefaultEncodingIfNoBom);

    // Reads characters from the file, which must be at position 0, to determine the BOM.
    // If a BOM is found, the file is positioned following the BOM.
    // If no BOM is found, the file is positioned back to position 0.
    TextEncoding(FILE* file, Type default_encoding_if_no_bom = DefaultEncodingIfNoBom);

    // Opens the file, reads for a BOM, and returns one if found.
    // If no BOM is present, or the file could not be opened, then default_encoding_if_no_bom is returned.
    CLASS_DECL_ZTOOLSO static TextEncoding ReadFileBom(const std::string& file_path, Type default_encoding_if_no_bom = DefaultEncodingIfNoBom);

    // Updates the encoding based on a call to one of the constructors, with the value of
    // default_encoding_if_no_bom coming from the current value of m_type.
    template<typename... Args>
    void UpdateEncoding(Args const&... args);

    // Returns the encoding type.
    Type GetType() const { return m_type; }

    // Returns whether the encoding type uses a BOM.
    static constexpr bool UsesBom(Type type);
    bool UsesBom() const { return UsesBom(m_type); }

    // Returns the BOM for the encoding type.
    static constexpr std::string_view GetBom(Type type);
    std::string_view GetBom() const { return GetBom(m_type); }

    // Returns the BOM length for the encoding type.
    static constexpr size_t GetBomLength(Type type);
    size_t GetBomLength() const { return GetBomLength(m_type); }

    // Returns whether or not the encoding type is UTF-8 (with or without a BOM).
    static constexpr bool IsUtf8(Type type);
    bool IsUtf8() const { return IsUtf8(m_type); }

    // Returns whether or not the encoding type is ANSI or UTF-8 (with or without a BOM).
    static constexpr bool IsAnsiOrUtf8(Type type);
    bool IsAnsiOrUtf8() const { return IsAnsiOrUtf8(m_type); }

    // Returns a string describing the encoding.
    CLASS_DECL_ZTOOLSO const char* ToString() const;

    // Creates a converter to convert text to/from UTF-8.
    // The converter is null if not needed.
    CLASS_DECL_ZTOOLSO static std::unique_ptr<Converter> CreateConverter(Type type);
    std::unique_ptr<Converter> CreateConverter() const { return CreateConverter(m_type); }

private:
    CLASS_DECL_ZTOOLSO static Type GetType(std::string_view text_sv, Type default_encoding_if_no_bom);
    static Type GetType(const void* buffer, size_t buffer_length, Type default_encoding_if_no_bom);
    CLASS_DECL_ZTOOLSO static Type GetType(FILE* file, Type default_encoding_if_no_bom);

private:
    Type m_type;
};



// --------------------------------------------------------------------------
// TextEncoding::Converter
// --------------------------------------------------------------------------

class TextEncoding::Converter
{
public:
    virtual ~Converter() { }

    virtual std::string ToUtf8(std::string_view text_sv) = 0;
    virtual std::string FromUtf8(std::string_view text_sv) = 0;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline TextEncoding::TextEncoding(const Type type/* = DefaultEncoding*/)
    :   m_type(type)
{
}


inline TextEncoding::TextEncoding(const std::string_view text_sv, const Type default_encoding_if_no_bom/* = DefaultEncodingIfNoBom*/)
    :   m_type(GetType(text_sv, default_encoding_if_no_bom))
{
}


inline TextEncoding::TextEncoding(const void* const buffer, const size_t buffer_length, const Type default_encoding_if_no_bom/* = DefaultEncodingIfNoBom*/)
    :   m_type(GetType(buffer, buffer_length, default_encoding_if_no_bom))
{
}


inline TextEncoding::TextEncoding(FILE* const file, const Type default_encoding_if_no_bom/* = DefaultEncodingIfNoBom*/)
    :   m_type(GetType(file, default_encoding_if_no_bom))
{
}


inline TextEncoding::Type TextEncoding::GetType(const void* const buffer, const size_t buffer_length, const Type default_encoding_if_no_bom)
{
    return ( buffer != nullptr ) ? GetType(std::string_view(reinterpret_cast<const char*>(buffer), buffer_length), default_encoding_if_no_bom) :
                                   default_encoding_if_no_bom;
}


template<typename... Args>
void TextEncoding::UpdateEncoding(Args const&... args)
{
    static_assert(DefaultEncodingIfNoBom == Type::Utf8);

    const Type default_encoding_if_no_bom = ( m_type == Type::Ansi ) ? Type::Ansi :
                                                                       Type::Utf8;

    m_type = GetType(args..., default_encoding_if_no_bom);
}


constexpr bool TextEncoding::UsesBom(const Type type)
{
    return ( type != Type::Ansi &&
             type != Type::Utf8 );
}


constexpr std::string_view TextEncoding::GetBom(const Type type)
{
    switch( type )
    {
        case Type::Utf8Bom: return Utf8Bom_sv;
        case Type::Utf16LE: return Utf16LEBom_sv;
        case Type::Utf16BE: return Utf16BEBom_sv;
        default:            return NoBom_sv;
    }
}


constexpr size_t TextEncoding::GetBomLength(const Type type)
{
    return GetBom(type).length();
}


constexpr bool TextEncoding::IsUtf8(const Type type)
{
    return ( type == Type::Utf8 ||
             type == Type::Utf8Bom );
}


constexpr bool TextEncoding::IsAnsiOrUtf8(const Type type)
{
    return ( IsUtf8(type) ||
             type == Type::Ansi );
}
