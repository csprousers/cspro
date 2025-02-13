#pragma once

#include <zMapping/zMapping.h>


namespace CurrentLocation
{
    // returns an approximation of the current location (based on the IP address) if available
    ZMAPPING_API const std::optional<std::tuple<double, double>>& GetCurrentLocation();

    // returns an approximation of the current location (based on the IP address);
    // if unavailable, returns the location of the Census Bureau
    ZMAPPING_API std::tuple<double, double> GetCurrentLocationOrCensusBureau();
}
