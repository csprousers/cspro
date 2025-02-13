#include "stdafx.h"
#include "CoordinateConverter.h"


std::wstring CoordinateConverter::ToDecimalString(double latitude, double longitude)
{
    return FormatTextCS2WS(_T("%0.6f, %0.6f"), latitude, longitude);
}


std::wstring CoordinateConverter::ToDMSString(double latitude, double longitude)
{
    auto convert = [](double value, TCHAR pcc, TCHAR ncc)
    {
        double degrees;
        double minutes;
        TCHAR cardinal_char;

        if( value >= 0 )
        {
            minutes = 60 * std::modf(value, &degrees);
            cardinal_char = pcc;
        }

        else
        {
            minutes = 60 * std::modf(-1 * value, &degrees);
            cardinal_char = ncc;
        }

        double seconds = 60 * std::modf(minutes, &value);

        return FormatTextCS2WS(_T("%d° %d' %0.0f\" %c"), (int)degrees, (int)minutes, seconds, cardinal_char);
    };

    return SO::Concatenate(convert(latitude, 'N', 'S'), _T(", "), convert(longitude, 'E', 'W'));
}
