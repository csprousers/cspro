#include "Stdafx.h"
#include "QRCode.h"
#include <zMultimediaO/QRCode.h>


CSPro::Util::QRCode::QRCode(System::String^ text, System::String^ ecc_text, const int scale, const int quiet_zone)
    :   m_bmpFile(nullptr)
{
    try
    {
        Multimedia::QRCode qr_code;

        const std::optional<int> error_correction_level = Multimedia::QRCode::GetErrorCorrectionLevelFromText(clr_helpers::to_string(ecc_text));

        if( !error_correction_level.has_value() )
            throw CSProException("Invalid error correction level");

        qr_code.SetErrorCorrectionLevel(*error_correction_level);
        qr_code.SetScale(scale);
        qr_code.SetScale(quiet_zone);

        qr_code.Create(clr_helpers::to_string(text));

        m_bmpFile = new Multimedia::BmpFile(qr_code.GetBmpFile());        
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(clr_helpers::to_SystemString(exception.what()));
    }
}


CSPro::Util::QRCode::!QRCode()
{
    delete m_bmpFile;
}


System::Drawing::Bitmap^ CSPro::Util::QRCode::GetBitmap()
{
    ASSERT(m_bmpFile != nullptr);

    System::IntPtr pixel_data(reinterpret_cast<INT_PTR>(m_bmpFile->GetPixelData()));

    return gcnew System::Drawing::Bitmap(m_bmpFile->GetWidth(), m_bmpFile->GetHeight(), m_bmpFile->GetStride(),
                                         System::Drawing::Imaging::PixelFormat::Format24bppRgb, pixel_data);
}
