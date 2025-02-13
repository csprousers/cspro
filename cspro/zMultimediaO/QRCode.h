#pragma once

#include <zMultimediaO/zMultimediaO.h>
#include <zMultimediaO/BmpFile.h>
#include <zUtilO/PortableColor.h>

namespace Multimedia { class Image; }


namespace Multimedia
{
    // a simple QR code creator that wraps the qrcodegen library

    class ZMULTIMEDIAO_API QRCode
    {
    public:
        static constexpr int ScaleMin         = 1;
        static constexpr int ScaleDefault     = 4;

        static constexpr int QuietZoneMin     = 4;
        static constexpr int QuietZoneDefault = 4;

        QRCode();
        ~QRCode();

        static std::optional<int> GetErrorCorrectionLevelFromText(wstring_view ecc_text_sv);

        // the Set... methods can throw CSProException exceptions
        void SetErrorCorrectionLevel(int error_correction_level);
        void SetErrorCorrectionLevel(wstring_view ecc_text_sv);
        void SetScale(int scale);
        void SetQuietZone(int quiet_zone);
        void SetDarkColor(PortableColor dark_color);
        void SetLightColor(PortableColor light_color);

        // can throw CSProException exceptions
        void Create(const std::string& text);

        // can throw CSProException exceptions
        BmpFile GetBmpFile() const;

        // can throw CSProException and Multimedia::ImageException exceptions
        std::unique_ptr<Multimedia::Image> GetImage() const;

    private:
        struct Data;
        std::unique_ptr<Data> m_data;
    };
}
