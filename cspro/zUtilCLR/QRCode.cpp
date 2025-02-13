#include "Stdafx.h"
#include "QRCode.h"
#include <zMultimediaO/QRCode.h>
#include <zToolsO/Utf8Convert.h>


CSPro::Util::QRCode::QRCode(System::String^ text, System::String^ ecc_text, int scale, int quiet_zone)
    :   m_bmpFile(nullptr)
{
    try
    {
        Multimedia::QRCode qr_code;

        std::optional<int> error_correction_level = Multimedia::QRCode::GetErrorCorrectionLevelFromText(ToWS(ecc_text));

        if( !error_correction_level.has_value() )
            throw CSProException("Invalid error correction level");

        qr_code.SetErrorCorrectionLevel(*error_correction_level);
        qr_code.SetScale(scale);
        qr_code.SetScale(quiet_zone);

        qr_code.Create(UTF8Convert::WideToUTF8(ToWS(text)));

        m_bmpFile = new Multimedia::BmpFile(qr_code.GetBmpFile());        
    }

    catch( const CSProException& exception )
    {
        throw gcnew System::Exception(gcnew System::String(exception.GetErrorMessage().c_str()));
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
