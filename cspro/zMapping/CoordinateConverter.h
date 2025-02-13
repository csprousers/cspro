#pragma once

#include <zMapping/zMapping.h>
#include <zAppO/Properties/MappingProperties.h>


namespace CoordinateConverter
{
    ZMAPPING_API std::wstring ToDecimalString(double latitude, double longitude);

    inline std::wstring ToDecimalString(const std::tuple<double, double>& coordinates)
    {
        return ToDecimalString(std::get<0>(coordinates), std::get<1>(coordinates));
    }


    ZMAPPING_API std::wstring ToDMSString(double latitude, double longitude);

    inline std::wstring ToDMSString(const std::tuple<double, double>& coordinates)
    {
        return ToDMSString(std::get<0>(coordinates), std::get<1>(coordinates));
    }


    inline std::wstring ToString(CoordinateDisplay coordinate_display, double latitude, double longitude)
    {
        return ( coordinate_display == CoordinateDisplay::Decimal ) ? ToDecimalString(latitude, longitude) :
                                                                      ToDMSString(latitude, longitude);
    }

    inline std::wstring ToString(CoordinateDisplay coordinate_display, const std::tuple<double, double>& coordinates)
    {
        return ToString(coordinate_display, std::get<0>(coordinates), std::get<1>(coordinates));
    }
}
