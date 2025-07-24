#include "stdafx.h"
#include "CurrentLocation.h"
#include <zUtilO/CredentialStore.h>
#include <zNetwork/CurlHttpConnection.h>


const std::optional<std::tuple<double, double>>& CurrentLocation::GetCurrentLocation()
{
    static std::optional<std::tuple<double, double>> current_location;

    // only query the location once per session and only once every hour
    constexpr double RefreshSeconds = 3600;

    if( !current_location.has_value() )
    {
        constexpr std::string_view AttributeName_sv = "CSPro_location";
        CredentialStore credential_store;
        const std::string location_cache = credential_store.Retrieve(AttributeName_sv);
        double cached_timestamp = 0;

        // first check the cached location (to avoid using the API too often)
        if( !location_cache.empty() )
        {
            const JsonNode cache_json = Json::Parse(location_cache);

            if( cache_json.Contains(JK::timestamp) )
            {
                cached_timestamp = cache_json.Get<double>(JK::timestamp);

                current_location.emplace(cache_json.Get<double>(JK::latitude),
                                         cache_json.Get<double>(JK::longitude));
            }
        }

        // if the location was never cached or is old, query for the location
        if( ( GetTimestamp() - cached_timestamp ) >= RefreshSeconds )
        {
            try
            {
                CurlHttpConnection connection;
                const HttpRequest request = HttpRequestBuilder("https://ipinfo.io/json").build();
                HttpResponse response = connection.Request(request);

                const JsonNode response_json = Json::Parse(response.body.ToString());
                const std::string loc = response_json.Get<std::string>("loc");
                const size_t comma_pos = loc.find(',');

                current_location.emplace(std::stod(loc.substr(0, comma_pos)), std::stod(loc.substr(comma_pos + 1)));

                const std::string new_cache = Json::CreateObjectString(
                    {
                        { JK::timestamp, GetTimestamp() },
                        { JK::latitude,  std::get<0>(*current_location) },
                        { JK::longitude, std::get<1>(*current_location) }
                    });

                credential_store.Store(AttributeName_sv, new_cache);
            }

            // ignore connection errors
            catch(...) { }
        }
    }

    return current_location;
}


std::tuple<double, double> CurrentLocation::GetCurrentLocationOrCensusBureau()
{
    return GetCurrentLocation().value_or(std::make_tuple(38.846261, -76.929445));
}
