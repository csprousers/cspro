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


    class MappingPropertiesTesterMapUI : public WindowsMapUISingleThread
    {
    public:
        MappingPropertiesTesterMapUI(const MappingProperties& mapping_properties)
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

        void AddButton(std::wstring button_text, std::function<void(int)> callback_function)
        {
            AddTextButton(std::move(button_text), m_callbackFunctions.size());
            m_callbackFunctions.emplace_back(std::move(callback_function));
        }

        void NotifyEvent(EventCode code, int marker_id = -1, int callback_id = -1,
            double /*latitude*/ = 0, double /*longitude*/ = 0, const MapCamera& /*camera*/ = MapCamera { 0, 0, 0, 0 }) override
        {
            if( code == EventCode::ButtonClicked )
            {
                ASSERT(callback_id < (int)m_callbackFunctions.size());
                m_callbackFunctions[callback_id](marker_id);
            }
        }

    private:
        std::vector<std::function<void(int)>> m_callbackFunctions;
    };



    void SetupMapForTileProvider(MappingPropertiesTesterMapUI& map_ui, const MappingProperties& mapping_properties)
    {
        const MappingTileProviderProperties& mapping_tile_provider_properties = mapping_properties.GetWindowsMappingTileProviderProperties();

        if( mapping_tile_provider_properties.GetMappingTileProvider() == MappingTileProvider::Mapbox &&
            mapping_tile_provider_properties.GetAccessToken().empty() )
        {
            throw CSProException("Mapbox will not work without an access token.");
        }

        auto set_base_map = [&map_ui, &mapping_tile_provider_properties](BaseMap base_map)
        {
            std::wstring title = SO::Concatenate(ToString(mapping_tile_provider_properties.GetMappingTileProvider()), 
                                                 _T(" Map: "),
                                                 ToString(base_map));

            if( base_map != BaseMap::None )
                SO::AppendFormat(title, _T(" (%s)"), mapping_tile_provider_properties.GetTileLayer(base_map).c_str());

            map_ui.SetTitle(std::move(title));

            map_ui.SetBaseMap(base_map);
        };

        std::optional<BaseMap> default_base_map;

        if( std::holds_alternative<BaseMap>(mapping_properties.GetDefaultBaseMap()) )
            default_base_map = std::get<BaseMap>(mapping_properties.GetDefaultBaseMap());

        set_base_map(default_base_map.value_or(BaseMap::Normal));

        // add buttons for each base map
        for( const std::wstring& base_map_string : GetBaseMapStrings() )
        {
            BaseMap base_map = *FromString<BaseMap>(base_map_string);

            map_ui.AddButton(SO::Concatenate(base_map_string, ( base_map == default_base_map ) ? _T(" (default)") : _T("")),
                [set_base_map, base_map](int /*button_id*/)
                {
                    set_base_map(base_map);
                });
        }

        // display the current location with a marker, also showing how coordinates are formatted
        std::tuple<double, double> current_location = CurrentLocation::GetCurrentLocationOrCensusBureau();

        map_ui.SetShowCurrentLocation(false);

        map_ui.ZoomTo(std::get<0>(current_location), std::get<1>(current_location), ZoomLevelWhenDisplayingCurrentLocation);

        std::wstring coordinates_text;

        auto add_coordinates = [&](CoordinateDisplay coordinate_display)
        {
            SO::AppendWithSeparator(coordinates_text,
                                    SO::Concatenate(CoordinateConverter::ToString(coordinate_display, current_location),
                                                    ( coordinate_display == mapping_properties.GetCoordinateDisplay() ) ? _T(" (default)") : _T("")),
                                    _T(" — "));
        };

        add_coordinates(CoordinateDisplay::Decimal);
        add_coordinates(CoordinateDisplay::DMS);

        int marker_id = map_ui.AddMarker(std::get<0>(current_location), std::get<1>(current_location));
        map_ui.SetMarkerText(marker_id, std::move(coordinates_text), PortableColor::White.ToColorInt(), PortableColor::Black.ToColorInt());
    }


    void SetupMapForOfflineBaseMap(MappingPropertiesTesterMapUI& map_ui, const std::wstring& filename)
    {
        // when showing an offline map, simply display the map, zooming to the bounds of the map
        map_ui.SetTitle(SO::Concatenate(_T("Offline Map: "), PortableFunctions::PathGetFilename(filename)));

        map_ui.SetShowCurrentLocation(false);

        map_ui.SetBaseMap(filename);

        map_ui.AddButton(_T("Show Current (Approximate) Location"),
            [&map_ui](int button_id)
            {
                map_ui.SetShowCurrentLocation(true);
                map_ui.RemoveButton(button_id);

                const std::optional<std::tuple<double, double>>& current_location = CurrentLocation::GetCurrentLocation();

                if( current_location.has_value() )
                    map_ui.ZoomTo(std::get<0>(*current_location), std::get<1>(*current_location), ZoomLevelWhenDisplayingCurrentLocation);
            });

        // zoom to the bounds of the offline map
        std::optional<OfflineTileReader::Bounds> bounds = map_ui.GetOfflineTileReaderBounds();

        if( bounds.has_value() )
        {
            map_ui.ZoomTo(std::get<0>(bounds->min), std::get<1>(bounds->min),
                          std::get<0>(bounds->max), std::get<1>(bounds->max));

            map_ui.AddButton(_T("Copy Offline Map Bounds to Clipboard"),
                [&map_ui, bounds](int /*button_id*/)
                {
                    WinClipboard::PutText(map_ui.GetMapDlg(), FormatText(_T("%s, %s, %s, %s"),
                        DoubleToString(std::get<0>(bounds->min)).c_str(), DoubleToString(std::get<1>(bounds->min)).c_str(),
                        DoubleToString(std::get<0>(bounds->max)).c_str(), DoubleToString(std::get<1>(bounds->max)).c_str()));
                });
        }
    }
}


void TestMappingProperties(MappingProperties mapping_properties,
    std::optional<MappingTileProvider> mapping_tile_provider/* = std::nullopt*/)
{
    // mapping properties is passed as a copy in case we need to override the tile provider
    if( mapping_tile_provider.has_value() )
        mapping_properties.SetWindowsMappingTileProvider(*mapping_tile_provider);

    try
    {
        MappingPropertiesTesterMapUI map_ui(mapping_properties);

        if( mapping_tile_provider.has_value() || std::holds_alternative<BaseMap>(mapping_properties.GetDefaultBaseMap()) )
        {
            SetupMapForTileProvider(map_ui, mapping_properties);
        }

        else
        {
            SetupMapForOfflineBaseMap(map_ui, std::get<std::wstring>(mapping_properties.GetDefaultBaseMap()));
        }

        map_ui.Show();
    }

    catch( const CSProException& exception )
    {
        ErrorMessage::Display(exception);
    }
}
