#include "stdafx.h"
#include "QRCode.h"
#include "Image.h"
#include <external/qrcodegen/qrcodegen.hpp>


// --------------------------------------------------------------------------
// QRCode::Data
// --------------------------------------------------------------------------

struct Multimedia::QRCode::Data
{
    qrcodegen::QrCode::Ecc error_correction_level = qrcodegen::QrCode::Ecc::MEDIUM;
    int scale = QRCode::ScaleDefault;
    int quiet_zone = QRCode::QuietZoneDefault;
    PortableColor dark_color = PortableColor::Black;
    PortableColor light_color = PortableColor::White;

    std::unique_ptr<qrcodegen::QrCode> qr_code;
};



// --------------------------------------------------------------------------
// QRCode
// --------------------------------------------------------------------------

Multimedia::QRCode::QRCode()
    :   m_data(std::make_unique<Data>())
{
}


Multimedia::QRCode::~QRCode()
{
}


std::optional<int> Multimedia::QRCode::GetErrorCorrectionLevelFromText(wstring_view ecc_text_sv)
{
    const std::tuple<qrcodegen::QrCode::Ecc, const TCHAR*> error_correction_mapping[] =
    {
        { qrcodegen::QrCode::Ecc::LOW,      _T("low") },
        { qrcodegen::QrCode::Ecc::MEDIUM,   _T("medium") },
        { qrcodegen::QrCode::Ecc::QUARTILE, _T("quartile") },
        { qrcodegen::QrCode::Ecc::HIGH,     _T("high") }
    };

    const TCHAR single_letter_version = ( ecc_text_sv.length() == 1 ) ? std::towlower(ecc_text_sv.front()) : 0;

    for( const auto& [this_ecc, this_ecc_text] : error_correction_mapping )
    {
        if( ( single_letter_version != 0 ) ? ( single_letter_version == *this_ecc_text ) :
                                             ( SO::EqualsNoCase(ecc_text_sv, this_ecc_text) ) )
        {
            return static_cast<int>(this_ecc);
        }
    }

    return std::nullopt;
}


void Multimedia::QRCode::SetErrorCorrectionLevel(int error_correction_level)
{
    ASSERT(error_correction_level >= static_cast<int>(qrcodegen::QrCode::Ecc::LOW) &&
           error_correction_level <= static_cast<int>(qrcodegen::QrCode::Ecc::HIGH));

    m_data->error_correction_level = static_cast<qrcodegen::QrCode::Ecc>(error_correction_level);
}


void Multimedia::QRCode::SetErrorCorrectionLevel(wstring_view ecc_text_sv)
{
    std::optional<int> error_correction_level = GetErrorCorrectionLevelFromText(ecc_text_sv);

    if( !error_correction_level.has_value() )
        throw CSProException(_T("The error correction level '%s' is not valid."), std::wstring(ecc_text_sv).c_str());

    SetErrorCorrectionLevel(*error_correction_level);
}


void Multimedia::QRCode::SetScale(int scale)
{
    if( scale < ScaleMin )
        throw CSProException(_T("The scale cannot be less than %d."), ScaleMin);

    m_data->scale = scale;
}


void Multimedia::QRCode::SetQuietZone(int quiet_zone)
{
    if( quiet_zone < QuietZoneMin )
        throw CSProException(_T("The quiet zone cannot be less than %d."), QuietZoneMin);

    m_data->quiet_zone = quiet_zone;
}


void Multimedia::QRCode::SetDarkColor(PortableColor dark_color)
{
    m_data->dark_color = dark_color;
}


void Multimedia::QRCode::SetLightColor(PortableColor light_color)
{
    m_data->light_color = light_color;
}


void Multimedia::QRCode::Create(const std::string& text)
{
    try
    {
        m_data->qr_code = std::make_unique<qrcodegen::QrCode>(qrcodegen::QrCode::encodeText(text.c_str(), m_data->error_correction_level));
    }

    catch( const std::exception& exception )
    {
        throw CSProException(exception.what());
    }
}


Multimedia::BmpFile Multimedia::QRCode::GetBmpFile() const
{
    if( m_data->qr_code == nullptr )
        throw CSProException("No QR code was created");

    constexpr int BytesPerPixel = 3;

    int length_pixels = ( m_data->qr_code->getSize() + m_data->quiet_zone * 2 ) * m_data->scale;
    int length_bytes = length_pixels * BytesPerPixel;
    ASSERT(length_pixels > 0);

    // create a 24-bit image
    size_t image_size = length_pixels * length_pixels * BytesPerPixel;
    auto image_content = std::make_unique_for_overwrite<std::byte[]>(image_size);


    // get full width representations of each color
    auto create_line = [&](COLORREF colorref, std::byte* line)
    {
        std::byte* line_itr = line;

        // create the first pixel
        *(line_itr++) = std::byte(GetRValue(colorref));
        *(line_itr++) = std::byte(GetGValue(colorref));
        *(line_itr++) = std::byte(GetBValue(colorref));

        ASSERT(( line_itr - BytesPerPixel ) == line);

        // copy the pixel multiple times
        for( int i = 1; i < length_pixels; ++i )
        {
            memcpy(line_itr, line, BytesPerPixel);
            line_itr += BytesPerPixel;
        }

        return line;
    };

    std::unique_ptr<std::byte[]> dark_line = std::make_unique_for_overwrite<std::byte[]>(length_bytes);
    create_line(m_data->dark_color.ToCOLORREF(), dark_line.get());

    // the light line can be created as a border in the image
    std::byte* light_line = image_content.get();
    create_line(m_data->light_color.ToCOLORREF(), light_line);


    // a routine to draw pixels (with proper handling for rows)
    auto draw_pixels = [&](int row, int column, int pixels, std::byte* line)
    {
        ASSERT(row >= 0 && row < length_pixels);
        ASSERT(column >= 0 && ( column + pixels ) <= length_pixels);
        ASSERT(pixels > 0 && pixels <= length_pixels);
        ASSERT(line != nullptr);

        int row_with_0_equal_to_bottom = length_pixels - row - 1;
        int offset = ( row_with_0_equal_to_bottom * length_bytes ) + ( column * BytesPerPixel );

        memcpy(image_content.get() + offset, line, pixels * BytesPerPixel);
    };


    // draw the quiet zone
    int scaled_quiet_zone = m_data->quiet_zone * m_data->scale;

    {
        int start_of_second_quiet_zone = length_pixels - scaled_quiet_zone;
        int row = 0;

        // the top border
        for( ; row < scaled_quiet_zone; ++row )
            draw_pixels(row, 0, length_pixels, light_line);

        // the left and right borders
        for( ; row < start_of_second_quiet_zone; ++row )
        {
            draw_pixels(row, 0, scaled_quiet_zone, light_line);
            draw_pixels(row, start_of_second_quiet_zone, scaled_quiet_zone, light_line);
        }

        // the bottom border (- 1 because a row was drawn by create_line while creating the light line)
        int row_end = length_pixels - 1;

        for( ; row < row_end; ++row )
            draw_pixels(row, 0, length_pixels, light_line);
    }


    // draw the QR code modules
    {
        for( int x = m_data->qr_code->getSize() - 1; x >= 0; --x )
        {
            int column = scaled_quiet_zone + ( x * m_data->scale );

            for( int y = m_data->qr_code->getSize() - 1; y >= 0; --y )
            {
                int row = scaled_quiet_zone + ( y * m_data->scale );
                int row_end = row + m_data->scale;

                std::byte* line = m_data->qr_code->getModule(x, y) ? dark_line.get() :
                                                                     light_line;

                for( ; row < row_end; ++row )
                    draw_pixels(row, column, m_data->scale, line);
            }
        }
    }

    return BmpFile(image_content.get(), image_size, length_pixels, length_pixels);
}


std::unique_ptr<Multimedia::Image> Multimedia::QRCode::GetImage() const
{
    BmpFile bmp_file = GetBmpFile();

    return bmp_file.CreateImage();
}
