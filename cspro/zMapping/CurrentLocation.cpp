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
        class CurrentLocationCredentialStore : public CredentialStore
        {
        public:
            std::wstring AttributeName;

        protected:
            std::wstring PrefixAttribute(const std::wstring& attribute) override
            {
                ASSERT(attribute == AttributeName);
                return _T("CSPro_location");
            }
        };

        CurrentLocationCredentialStore credential_store;
        std::wstring location_cache = credential_store.Retrieve(credential_store.AttributeName);
        double cached_timestamp = 0;

        // first check the cached location (to avoid using the API too often)
        if( !location_cache.empty() )
        {
            auto cache_json = Json::Parse(location_cache);

            if( cache_json.Contains(_T("timestamp")) )
            {
                cached_timestamp = cache_json.Get<double>(_T("timestamp"));

                current_location.emplace(cache_json.Get<double>(_T("latitude")),
                                         cache_json.Get<double>(_T("longitude")));
            }
        }

        // if the location was never cached or is old, query for the location
        if( ( GetTimestamp() - cached_timestamp ) >= RefreshSeconds )
        {
            try
            {
                CurlHttpConnection connection;
                HttpRequest request = HttpRequestBuilder(_T("https://ipinfo.io/json")).build();
                HttpResponse response = connection.Request(request);

                auto response_json = Json::Parse(response.body.ToString());
                std::string loc = response_json["loc"].Get<std::string>();
                int comma = loc.find(',');

                current_location.emplace(std::stod(loc.substr(0, comma)), std::stod(loc.substr(comma + 1)));

                std::wstring new_cache = Json::CreateObjectString(
                    {
                        { _T("timestamp"), GetTimestamp() },
                        { _T("latitude"),  std::get<0>(*current_location) },
                        { _T("longitude"), std::get<1>(*current_location) }
                    });

                credential_store.Store(credential_store.AttributeName, new_cache);
            }

            // ignore connection errors
            catch( ... ) { }
        }
    }

    return current_location;
}


std::tuple<double, double> CurrentLocation::GetCurrentLocationOrCensusBureau()
{
    return GetCurrentLocation().value_or(std::make_tuple(38.846261, -76.929445));
}
