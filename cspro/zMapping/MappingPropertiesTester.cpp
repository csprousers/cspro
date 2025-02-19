#include "stdafx.h"
#include "MappingPropertiesTester.h"
#include "CoordinateConverter.h"
#include "CurrentLocation.h"
#include "OfflineTileReader.h"
#include "WindowsMapDlg.h"
#include "WindowsMapUISingleThread.h"
#include <zToolsO/WinClipboard.h>


namespace
{
    constexpr double ZoomLevelWhenDisplayingCurrentLocation = 12;
}


// --------------------------------------------------------------------------
// MappingPropertiesTester::TestMapUI
// --------------------------------------------------------------------------

class MappingPropertiesTester::TestMapUI : public WindowsMapUISingleThread
{
public:
    TestMapUI(const MappingProperties& mapping_properties)
        :   WindowsMapUISingleThread(mapping_properties)
    {
    }

    std::optional<OfflineTileReader::Bounds> GetOfflineTileReaderBounds()
    {
        return ( m_tileReader != nullptr ) ? m_tileReader->GetBounds() : std::nullopt;
    }

    WindowsMapDlg* GetMapDlg()
    {
        return GetMapDlgForAction();
    }

    void AddButton(SharableString button_text, std::function<void(int)> callback_function)
    {
        AddTextButton(std::move(button_text), m_callbackFunctions.size());
        m_callbackFunctions.emplace_back(std::move(callback_function));
    }

    void NotifyEvent(const EventCode code, const int marker_id/* = -1*/, const int callback_id/* = -1*/,
                     const double /*latitude*/ = 0, const double /*longitude*/ = 0,
                     const MapCamera& /*camera*/ = MapCamera { 0, 0, 0, 0 }) override
    {
        if( code == EventCode::ButtonClicked )
        {
            ASSERT(callback_id < static_cast<int>(m_callbackFunctions.size()));
            m_callbackFunctions[callback_id](marker_id);
        }
    }

private:
    std::vector<std::function<void(int)>> m_callbackFunctions;
};



// --------------------------------------------------------------------------
// MappingPropertiesTester
// --------------------------------------------------------------------------

void MappingPropertiesTester::Test(MappingProperties mapping_properties, const std::optional<MappingTileProvider> mapping_tile_provider/* = std::nullopt*/)
{
    // mapping properties is passed as a copy in case we need to override the tile provider
    if( mapping_tile_provider.has_value() )
        mapping_properties.SetWindowsMappingTileProvider(*mapping_tile_provider);

    try
    {
        TestMapUI map_ui(mapping_properties);

        if( mapping_tile_provider.has_value() || std::holds_alternative<BaseMap>(mapping_properties.GetDefaultBaseMap()) )
        {
            SetupMapForTileProvider(map_ui, mapping_properties);
        }

        else
        {
            SetupMapForOfflineBaseMap(map_ui, std::get<std::string>(mapping_properties.GetDefaultBaseMap()));
        }

        map_ui.Show();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}


void MappingPropertiesTester::SetupMapForTileProvider(TestMapUI& map_ui, const MappingProperties& mapping_properties)
{
    const MappingTileProviderProperties& mapping_tile_provider_properties = mapping_properties.GetWindowsMappingTileProviderProperties();

    if( mapping_tile_provider_properties.GetMappingTileProvider() == MappingTileProvider::Mapbox &&
        mapping_tile_provider_properties.GetAccessToken().empty() )
    {
        throw CSProException("Mapbox will not work without an access token.");
    }

    auto set_base_map = [&](const BaseMap base_map)
    {
        std::string title = SO::Concatenate(ToString(mapping_tile_provider_properties.GetMappingTileProvider()),
                                            " Map: ",
                                            ToString(base_map));

        if( base_map != BaseMap::None )
            title.append(FormatText(" (%s)", mapping_tile_provider_properties.GetTileLayer(base_map).c_str()));

        map_ui.SetTitle(std::move(title));

        map_ui.SetBaseMap(base_map);
    };

    std::optional<BaseMap> default_base_map;

    if( std::holds_alternative<BaseMap>(mapping_properties.GetDefaultBaseMap()) )
        default_base_map = std::get<BaseMap>(mapping_properties.GetDefaultBaseMap());

    set_base_map(default_base_map.value_or(BaseMap::Normal));

    // add buttons for each base map
    for( const char* const base_map_string : GetBaseMapStrings() )
    {
        const std::optional<BaseMap> base_map = FromString<BaseMap>(base_map_string);
        ASSERT(base_map.has_value());

        map_ui.AddButton(SO::Concatenate(base_map_string, ( *base_map == default_base_map ) ? " (default)" : ""),
            [set_base_map, this_base_map = *base_map](int /*button_id*/)
            {
                set_base_map(this_base_map);
            });
    }

    // display the current location with a marker, also showing how coordinates are formatted
    const std::tuple<double, double> current_location = CurrentLocation::GetCurrentLocationOrCensusBureau();

    map_ui.SetShowCurrentLocation(false);

    map_ui.ZoomTo(std::get<0>(current_location), std::get<1>(current_location), ZoomLevelWhenDisplayingCurrentLocation);

    std::string coordinates_text;

    auto add_coordinates = [&](const CoordinateDisplay coordinate_display)
    {
        SO::AppendWithSeparator(coordinates_text,
                                SO::Concatenate(CoordinateConverter::ToString(coordinate_display, current_location),
                                                ( coordinate_display == mapping_properties.GetCoordinateDisplay() ) ? " (default)" : ""),
                                u8" — ");
    };

    add_coordinates(CoordinateDisplay::Decimal);
    add_coordinates(CoordinateDisplay::DMS);

    const int marker_id = map_ui.AddMarker(std::get<0>(current_location), std::get<1>(current_location));
    map_ui.SetMarkerText(marker_id, std::move(coordinates_text), PortableColor::White.ToColorInt(), PortableColor::Black.ToColorInt());
}


void MappingPropertiesTester::SetupMapForOfflineBaseMap(TestMapUI& map_ui, const std::string& file_path)
{
    // when showing an offline map, simply display the map, zooming to the bounds of the map
    map_ui.SetTitle(SO::Concatenate("Offline Map: ", Path::GetFilename(file_path)));

    map_ui.SetShowCurrentLocation(false);

    map_ui.SetBaseMap(file_path);

    map_ui.AddButton("Show Current (Approximate) Location",
        [&map_ui](const int button_id)
        {
            map_ui.SetShowCurrentLocation(true);
            map_ui.RemoveButton(button_id);

            const std::optional<std::tuple<double, double>>& current_location = CurrentLocation::GetCurrentLocation();

            if( current_location.has_value() )
                map_ui.ZoomTo(std::get<0>(*current_location), std::get<1>(*current_location), ZoomLevelWhenDisplayingCurrentLocation);
        });

    // zoom to the bounds of the offline map
    const std::optional<OfflineTileReader::Bounds> bounds = map_ui.GetOfflineTileReaderBounds();

    if( bounds.has_value() )
    {
        map_ui.ZoomTo(std::get<0>(bounds->min), std::get<1>(bounds->min),
                      std::get<0>(bounds->max), std::get<1>(bounds->max));

        map_ui.AddButton("Copy Offline Map Bounds to Clipboard",
            [&map_ui, bounds](int /*button_id*/)
            {
                const std::string bounds_text = FormatText("%s, %s, %s, %s",
                                                           DoubleToString(std::get<0>(bounds->min)).c_str(), DoubleToString(std::get<1>(bounds->min)).c_str(),
                                                           DoubleToString(std::get<0>(bounds->max)).c_str(), DoubleToString(std::get<1>(bounds->max)).c_str());

                WinClipboard::PutText(map_ui.GetMapDlg(), bounds_text);
            });
    }
}
