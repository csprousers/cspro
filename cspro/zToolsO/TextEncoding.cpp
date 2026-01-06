#include "StdAfx.h"
#include "TextEncoding.h"
#include "File.h"
#include "TextConverter.h"
#include <zUtilO/Versioning.h>


// --------------------------------------------------------------------------
// TextEncoding
// --------------------------------------------------------------------------

static_assert(Versioning::Number < 9.0, "Starting with CSPro 9.0, change DefaultEncoding so that writing UTF-8 without a BOM the default");


namespace
{
    constexpr unsigned char MinBomCharValue = static_cast<unsigned char>(std::min({ TextEncoding::Utf8Bom_sv.front(),
                                                                                    TextEncoding::Utf16LEBom_sv.front(),
                                                                                    TextEncoding::Utf16BEBom_sv.front() }));
    constexpr size_t MinBomLength = 2;
    constexpr size_t MaxBomLength = 3;
}


TextEncoding::Type TextEncoding::GetType(const std::string_view text_sv, const Type default_encoding_if_no_bom)
{
    ASSERT(!UsesBom(default_encoding_if_no_bom));

    if( text_sv.length() >= MinBomLength &&
        static_cast<unsigned char>(text_sv.front()) >= MinBomCharValue )
    {
        switch( text_sv.front() )
        {
            case Utf8Bom_sv.front():
            {
                if( text_sv.length() >= Utf8Bom_sv.length() &&
                    text_sv[1] == Utf8Bom_sv[1] &&
                    text_sv[2] == Utf8Bom_sv[2] )
                {
                    return Type::Utf8Bom;
                }

                break;
            }

            case Utf16LEBom_sv.front():
            {
                if( text_sv[1] == Utf16LEBom_sv[1] )
                    return Type::Utf16LE;

                break;
            }

            case Utf16BEBom_sv.front():
            {
                if( text_sv[1] == Utf16BEBom_sv[1] )
                    return Type::Utf16BE;

                break;
            }
        }
    }

    return default_encoding_if_no_bom;
}


TextEncoding::Type TextEncoding::GetType(FILE* const file, const Type default_encoding_if_no_bom)
{
    ASSERT(!UsesBom(default_encoding_if_no_bom));

    if( file == nullptr )
        return default_encoding_if_no_bom;

    ASSERT(PortableFunctions::ftelli64(file) == 0);

    const int first_ch = fgetc(file);

    // return if there is need to check for a BOM
    if( first_ch < MinBomCharValue )
    {
        ungetc(first_ch, file);
        return default_encoding_if_no_bom;
    }

    // UTF-8
    if( first_ch == Utf8Bom_sv[0] )
    {
        if( fgetc(file) == Utf8Bom_sv[1] &&
            fgetc(file) == Utf8Bom_sv[2] )
        {
            return Type::Utf8Bom;
        }
    }

    // UTF-16LE
    else if( first_ch == Utf16LEBom_sv[0] )
    {
        if( fgetc(file) == Utf16LEBom_sv[1] )
            return Type::Utf16LE;
    }

    // UTF-16BE
    else if( first_ch == Utf16BEBom_sv[0] )
    {
        if( fgetc(file) == Utf16BEBom_sv[1] )
            return Type::Utf16BE;
    }

    // if no BOM was processed, seek back to the beginning
    PortableFunctions::fseeki64(file, 0, SEEK_SET);

    return default_encoding_if_no_bom;
}


TextEncoding TextEncoding::ReadFileBom(const std::string& file_path, Type default_encoding_if_no_bom/* = DefaultEncodingIfNoBom*/)
{
    try
    {
        FileIO::File file;
        file.OpenForReading(file_path);

        char bom_buffer[MaxBomLength];
        const size_t bytes_read = file.Read(bom_buffer, MaxBomLength);

        if( bytes_read >= MinBomLength )
            return TextEncoding(std::string_view(bom_buffer, bytes_read), default_encoding_if_no_bom);
    }
    catch(...) { }

    return default_encoding_if_no_bom;
}


const char* TextEncoding::ToString() const
{
    switch( m_type )
    {
        case Type::Ansi:
            return "ANSI";

        case Type::Utf16LE:
            return "UTF-16LE";

        case Type::Utf16BE:
            return "UTF-16BE";

        case Type::Utf8:
        case Type::Utf8Bom:
        default:
            return "UTF-8";
    }
}



// --------------------------------------------------------------------------
// AnsiConverter
// --------------------------------------------------------------------------

class AnsiConverter : public TextEncoding::Converter
{
public:
    std::string ToUtf8(const std::string_view text_sv) override
    {
        return TextConverter::AnsiToUtf8(text_sv);
    }

    std::string FromUtf8(const std::string_view text_sv) override
    {
        return TextConverter::Utf8ToAnsi(text_sv);
    }
};



#ifdef WIN32

// --------------------------------------------------------------------------
// Utf16LEConverter
// --------------------------------------------------------------------------

class Utf16LEConverter : public TextEncoding::Converter
{
public:
    std::string ToUtf8(const std::string_view text_sv) override
    {
        ASSERT(text_sv.size() % sizeof(wchar_t) == 0);

        return TC::ToUtf8(reinterpret_cast<const wchar_t*>(text_sv.data()),
                          static_cast<int>(text_sv.size() / sizeof(wchar_t)));
    }

    std::string FromUtf8(const std::string_view text_sv) override
    {
        const std::wstring wide_text = TC::ToWide(text_sv);

        return std::string(reinterpret_cast<const char*>(wide_text.data()),
                           static_cast<int>(wide_text.length() * sizeof(wchar_t)));
    }
};

#endif // WIN32



// --------------------------------------------------------------------------
// TextEncoding::CreateConverter
// --------------------------------------------------------------------------

std::unique_ptr<TextEncoding::Converter> TextEncoding::CreateConverter(const Type type)
{
    switch( type )
    {
        case Type::Ansi:
            return std::make_unique<AnsiConverter>();

        case Type::Utf8:
        case Type::Utf8Bom:
            return nullptr;

        case Type::Utf16LE: // TEXT_ENCODING_TODO implement a UTF-16LE converter in the portable environment?
#ifdef WIN32
            return std::make_unique<Utf16LEConverter>();
#else
            throw CSProException("CSPro cannot read files encoded as UTF-16LE.");
#endif

        case Type::Utf16BE: // TEXT_ENCODING_TODO implement a UTF-16BE converter ?
            throw CSProException("CSPro cannot read files encoded as UTF-16BE.");

        default:
            return ReturnProgrammingError(nullptr);
    }
}
