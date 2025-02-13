#pragma once

namespace Multimedia { class BmpFile; }


namespace CSPro
{
    namespace Util
    {
        public ref class QRCode sealed
        {
        public:
            QRCode(System::String^ text, System::String^ ecc_text, int scale, int quiet_zone);

            ~QRCode() { this->!QRCode(); }
            !QRCode();

            // the bitmap is valid until QRCode is destroyed
            System::Drawing::Bitmap^ GetBitmap();

        private:
            Multimedia::BmpFile* m_bmpFile;
        };
    }
}
