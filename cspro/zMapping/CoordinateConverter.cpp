#include "stdafx.h"
#include "CoordinateConverter.h"


std::string CoordinateConverter::ToDecimalString(const double latitude, const double longitude)
{
    return FormatText("%0.6f, %0.6f", latitude, longitude);
}


std::string CoordinateConverter::ToDMSString(const double latitude, const double longitude)
{
    auto convert = [](double value, const char pcc, const char ncc)
    {
        double degrees;
        double minutes;
        char cardinal_char;

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

        const double seconds = 60 * std::modf(minutes, &value);

        return FormatText(u8"%d° %d′ %0.0f″ %c", static_cast<int>(degrees), static_cast<int>(minutes), seconds, cardinal_char);
    };

    return SO::Concatenate(convert(latitude, 'N', 'S'), ", ", convert(longitude, 'E', 'W'));
}
