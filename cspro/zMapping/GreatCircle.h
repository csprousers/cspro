#pragma once

#include <zToolsO/Special.h>

#define _USE_MATH_DEFINES
#include <math.h>


namespace GreatCircle
{
    const double EARTH_CIRCUMFERENCE_METERS = 40030218; // Radius = 6371007 (GRS80)
    const double EARTH_RADIUS_METERS = 6371007;


    // 20110224 adopted from http://edndoc.esri.com/arcobjects/9.0/Samples/Geometry/Great_Circle_Distance.htm
    inline double Distance(double lat1, double long1, double lat2, double long2)
    {
        // 20140130 a strange value was returned if any of the parameters was a special value
        if( IsSpecial(lat1) || IsSpecial(long1) || IsSpecial(lat2) || IsSpecial(long2) )
            return DEFAULT;

        double origDistance = ( 90 - lat1 ) / 180 * M_PI;
        double destDistance = ( 90 - lat2 ) / 180 * M_PI;
        double distL = abs(long1 - long2) / 180 * M_PI;

        double cossa = ( cos(origDistance) * cos(destDistance) ) + ( sin(origDistance) * sin(destDistance) * cos(distL) );

        if( cossa == 1 )
            return 0;

        else if( cossa == -1 )
            return EARTH_CIRCUMFERENCE_METERS / 2;

        else
            return EARTH_CIRCUMFERENCE_METERS * ( ( M_PI_2 - atan(cossa / sqrt(1 - pow(cossa,2))) ) / ( 2 * M_PI ) );
    }
}
