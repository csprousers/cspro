#pragma once

#include <zMapping/zMapping.h>
#include <zAppO/Properties/MappingProperties.h>


namespace CoordinateConverter
{
    ZMAPPING_API std::string ToDecimalString(double latitude, double longitude);

    inline std::string ToDecimalString(const std::tuple<double, double>& coordinates)
    {
        return ToDecimalString(std::get<0>(coordinates), std::get<1>(coordinates));
    }


    ZMAPPING_API std::string ToDMSString(double latitude, double longitude);

    inline std::string ToDMSString(const std::tuple<double, double>& coordinates)
    {
        return ToDMSString(std::get<0>(coordinates), std::get<1>(coordinates));
    }


    inline std::string ToString(CoordinateDisplay coordinate_display, double latitude, double longitude)
    {
        return ( coordinate_display == CoordinateDisplay::Decimal ) ? ToDecimalString(latitude, longitude) :
                                                                      ToDMSString(latitude, longitude);
    }

    inline std::string ToString(CoordinateDisplay coordinate_display, const std::tuple<double, double>& coordinates)
    {
        return ToString(coordinate_display, std::get<0>(coordinates), std::get<1>(coordinates));
    }
}
