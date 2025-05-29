#include "stdafx.h"
#include "HtmlMapUI.h"
#include "GeoJson.h"
#include "MBTilesReader.h"
#include "OfflineTileProvider.h"
#include "TPKReader.h"
#include <zToolsO/Encoders.h>
#include <zHtml/HtmlishSanitizer.h>
#include <zHtml/HtmlTextConverter.h>
#include <zHtml/PortableLocalhost.h>
#include <zAppO/Properties/MappingProperties.h>
#include <sstream>

#pragma warning(push)
#pragma warning(disable: 4068 4239)
#include <mapbox/feature.hpp>
#include <mapbox/geometry.hpp>
#pragma warning(pop)


CREATE_JSON_KEY(camera)
CREATE_JSON_KEY(geojsonUrl)
CREATE_JSON_KEY(leafletId)
CREATE_JSON_KEY(maxLatitude)
CREATE_JSON_KEY(maxLongitude)
CREATE_JSON_KEY(minLatitude)
CREATE_JSON_KEY(minLongitude)
CREATE_JSON_KEY(options)
CREATE_JSON_KEY(padding)
CREATE_JSON_KEY(tileProvider)
CREATE_JSON_KEY(zoom)


// --------------------------------------------------------------------------
// HtmlMapUI objects
//
// variable names with the suffix:
//     _html = passed through HtmlishSanitizer
//     _text = passed through HtmlTextConverter
// --------------------------------------------------------------------------

struct HtmlMapUI::MapGeometry
{
    int id;
    std::unique_ptr<VirtualFileMappingHandler> virtual_file_mapping;
    int leaflet_id;
};


struct HtmlMapUI::Zoom1
{
    double latitude;
    double longitude;
    double zoom;
};


struct HtmlMapUI::Zoom2
{
    double min_latitude;
    double min_longitude;
    double max_latitude;
    double max_longitude;
    double padding_percent;
};


struct HtmlMapUI::Data
{
    SharableString title_html;
    std::string title_text;

    std::optional<BaseMapSelection> base_map_selection;
    std::shared_ptr<OfflineTileReader> tile_reader;
    std::unique_ptr<OfflineTileProvider> tile_provider;

    bool show_current_location = true;

    std::variant<std::monostate, Zoom1, Zoom2> zoom;

    int next_map_id = 1;

    std::map<int, MapGeometry> geometries;
};



// --------------------------------------------------------------------------
// HtmlMapUI
// --------------------------------------------------------------------------

HtmlMapUI::HtmlMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties)
    :   m_mappingProperties(std::move(mapping_properties)),
        m_data(std::make_unique<Data>())
{
}


HtmlMapUI::~HtmlMapUI()
{
}


std::string HtmlMapUI::GetUrlOfMapHtml() const
{
    const std::string& file_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Mapping), "logic-map.html");
    return PortableLocalhost::CreateFileUrl(file_path);
}


std::string HtmlMapUI::GetUrlForUrlOrFile(const std::string& url_or_file_path)
{
    if( Encoders::IsDataOrHttpUrl(url_or_file_path) )
        return url_or_file_path;

    return PortableLocalhost::CreateFileUrl(url_or_file_path);
}


void HtmlMapUI::PostActionMessage(const cs::string_sz action)
{
    if( !IsMapShowing() )
        return;

    OnPostActionMessage("{\"action\":" + Encoders::ToJsonString(action.c_str()) + "}");
}


void HtmlMapUI::PostActionMessage(cs::string_sz action, const std::function<void(JsonWriter&)>& callback_function)
{
    if( !IsMapShowing() )
        return;

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::action, action);

    callback_function(*json_writer);

    json_writer->EndObject();

    OnPostActionMessage(json_writer->ReleaseSharableString());
}


void HtmlMapUI::OnWebMessageReceived(const std::string_view message_sv)
{
    try
    {
        const JsonNode json_node = Json::Parse(message_sv);
        const std::string_view action_sv = json_node.Get<std::string_view>(JK::action);

        const JsonNode camera_json_node = json_node.GetOrEmpty(JK::camera);
        const IMapUI::MapCamera camera = camera_json_node.IsEmpty() ? IMapUI::MapCamera { 0, 0, 0, 0 } :
                                                                      IMapUI::MapCamera { camera_json_node.Get<double>(JK::latitude),
                                                                                          camera_json_node.Get<double>(JK::longitude),
                                                                                          camera_json_node.Get<float>(JK::zoom),
                                                                                          0 };

        if( action_sv == "documentLoaded" )
        {
            SetUpInitialMapIMIS();
        }

        else if( action_sv == "geometryPlaced" )
        {
            const int geometry_id = json_node.Get<int>(JK::id);
            const int leaflet_id = json_node.Get<int>(JK::leafletId);
            MapGeometry* const geometry = GetGeometry(geometry_id);

            if( geometry != nullptr )
                geometry->leaflet_id = leaflet_id;
        }
    }
    catch(...) { ASSERT(false); };
}


void HtmlMapUI::Clear()
{
    HtmlMapUI::SetTitle(SharableString());

    HtmlMapUI::SetBaseMapWorker(std::nullopt);

    HtmlMapUI::SetShowCurrentLocation(true);

    HtmlMapUI::ClearGeometry();

    HtmlMapUI::ZoomToWorker(std::monostate());
}


void HtmlMapUI::SetUpInitialMapIMIS()
{
    // set the title
    SetTitleIMIS();

    // set the base map
    SetBaseMapIMIS();

    // show or hide the current location
    SetShowCurrentLocationIMIS();

    // add geometries
    for( const auto& [id, geometry] : m_data->geometries )
        AddGeometryIMIS(geometry);

    // set the zoom
    ZoomToIMIS();
}


bool HtmlMapUI::SetTitle(SharableString title)
{
    m_data->title_html = HtmlishSanitizer::Sanitize(std::move(title));
    m_data->title_text = HtmlTextConverter::HtmlToText(*m_data->title_html);

    SetTitleIMIS();

    return true;
}


void HtmlMapUI::SetTitleIMIS()
{
    PostActionMessage("setTitle",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::title, m_data->title_html);
        });

    OnSetWindowTitle(m_data->title_text);
}


bool HtmlMapUI::IsBaseMapDefined() const
{
    return m_data->base_map_selection.has_value();
}


bool HtmlMapUI::SetBaseMap(BaseMapSelection base_map_selection)
{
    SetBaseMapWorker(std::move(base_map_selection));
    return true;
}


void HtmlMapUI::SetBaseMapWorker(std::optional<BaseMapSelection> base_map_selection)
{
    if( !base_map_selection.has_value() || std::holds_alternative<BaseMap>(*base_map_selection) )
    {
        m_data->tile_reader.reset();
        m_data->tile_provider.reset();
    }

    else
    {
        // open the MBTiles or TPK file
        const std::string& file_path = std::get<std::string>(*base_map_selection);
        const std::string extension = Path::GetExtension(file_path);

        if( SO::EqualsNoCase(extension, "mbtiles") )
        {
            m_data->tile_reader = std::make_unique<MBTilesReader>(file_path);
        }

        else if( SO::EqualsOneOfNoCase(extension, "tpk", "tpkx") )
        {
            m_data->tile_reader = std::make_unique<TPKReader>(file_path);
        }

        else
        {
            throw CSProException("unknown base map file with extension '%s'", extension.c_str());
        }

        m_data->tile_provider = std::make_unique<OfflineTileProvider>(m_data->tile_reader);
    }

    m_data->base_map_selection = std::move(base_map_selection);

    SetBaseMapIMIS();
}


void HtmlMapUI::SetBaseMapIMIS()
{
    ASSERT(!m_data->base_map_selection.has_value() ||
           std::holds_alternative<BaseMap>(*m_data->base_map_selection) == ( m_data->tile_provider == nullptr ));

    PostActionMessage("setBaseMap",
        [&](JsonWriter& json_writer)
        {
            if( !m_data->base_map_selection.has_value() ||
                std::holds_alternative<BaseMap>(*m_data->base_map_selection) )
            {
                // if the base map has not been manually set, use Normal
                const BaseMap base_map = m_data->base_map_selection.has_value() ? std::get<BaseMap>(*m_data->base_map_selection) :
                                                                                  BaseMap::Normal;
                json_writer.Write(JK::type, base_map);

                if( base_map != BaseMap::None )
                {
                    const MappingTileProviderProperties& mapping_tile_provider_properties = m_mappingProperties->GetWindowsMappingTileProviderProperties();

                    json_writer.Write(JK::tileProvider, mapping_tile_provider_properties.GetMappingTileProvider())
                               .Write(JK::tileLayer, mapping_tile_provider_properties.GetTileLayer(base_map))
                               .Write(JK::accessToken, mapping_tile_provider_properties.GetAccessToken());
                }
            }

            else
            {
                json_writer.Write(JK::url, m_data->tile_provider->GetTileLayerUrl());

                json_writer.Key(JK::options);
                m_data->tile_provider->WriteJsonLeafletTileLayerOptions(json_writer);
            }
        });
}


OfflineTileReader* HtmlMapUI::GetOfflineTileReader()
{
    return m_data->tile_reader.get();
}


bool HtmlMapUI::SetShowCurrentLocation(const bool show)
{
    m_data->show_current_location = show;

    SetShowCurrentLocationIMIS();

    return true;
}


void HtmlMapUI::SetShowCurrentLocationIMIS()
{
    if( !m_data->show_current_location|| !OnShowCurrentLocation() )
        PostActionMessage("hideCurrentLocation");
}


bool HtmlMapUI::SetCamera(const MapCamera& camera)
{
    return ZoomTo(camera.latitude, camera.longitude, camera.zoom);
}


constexpr bool HtmlMapUI::AreCoordinatesValid(const double latitude, const double longitude)
{
    return ( latitude >= -90 && latitude <= 90 &&
             longitude >= -180 && longitude <= 180 );
}


bool HtmlMapUI::ZoomTo(const double latitude, const double longitude, const double zoom/* = -1*/)
{
    if( !AreCoordinatesValid(latitude, longitude) )
        return false;

    ZoomToWorker(Zoom1 { latitude, longitude, zoom });

    return true;
}


bool HtmlMapUI::ZoomTo(const double min_latitude, const double min_longitude,
                       const double max_latitude, const double max_longitude,
                       const double padding_percent/* = 0*/)
{
    if( !AreCoordinatesValid(min_latitude, min_longitude) ||
        !AreCoordinatesValid(max_latitude, max_longitude) )
    {
        return false;
    }

    ZoomToWorker(Zoom2 { min_latitude, min_longitude, max_latitude, max_longitude, padding_percent });

    return true;
}


void HtmlMapUI::ZoomToWorker(std::variant<std::monostate, Zoom1, Zoom2> zoom)
{
    m_data->zoom = std::move(zoom);

    ZoomToIMIS();
}


void HtmlMapUI::ZoomToIMIS()
{
    if( std::holds_alternative<std::monostate>(m_data->zoom) )
    {
        FitMarkersIMIS();
    }

    else if( std::holds_alternative<Zoom1>(m_data->zoom) )
    {
        const Zoom1& zoom1 = std::get<Zoom1>(m_data->zoom);

        PostActionMessage("zoomTo",
            [&](JsonWriter& json_writer)
            {
                // need to set initial zoom, 7 seems like a nice number
                constexpr double DefaultZoomLevel = 7;

                json_writer.Write(JK::latitude, zoom1.latitude)
                           .Write(JK::longitude, zoom1.longitude)
                           .Write(JK::zoom, ( zoom1.zoom > 0 ) ? zoom1.zoom : DefaultZoomLevel);
            });
    }

    else
    {
        ASSERT(std::holds_alternative<Zoom2>(m_data->zoom));
        const Zoom2& zoom2 = std::get<Zoom2>(m_data->zoom);

        PostActionMessage("zoomTo",
            [&](JsonWriter& json_writer)
            {
                json_writer.Write(JK::minLatitude, zoom2.min_latitude)
                           .Write(JK::minLongitude, zoom2.min_longitude)
                           .Write(JK::maxLatitude, zoom2.max_latitude)
                           .Write(JK::maxLongitude, zoom2.max_longitude)
                           .Write(JK::padding, zoom2.padding_percent);
            });
    }
}


void HtmlMapUI::FitMarkersIMIS()
{
    PostActionMessage("fitMarkers");
}


HtmlMapUI::MapGeometry* HtmlMapUI::GetGeometry(const int geometry_id)
{
    const auto& lookup = m_data->geometries.find(geometry_id);
    return ( lookup != m_data->geometries.cend() ) ? &lookup->second :
                                                     nullptr;
}


int HtmlMapUI::AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds)
{
    ASSERT(geometry != nullptr && bounds != nullptr);

    std::ostringstream stream;
    GeoJson::toGeoJson(stream, *geometry);

    // serve the GeoJSON as a virtual file
    auto virtual_file_mapping = std::make_unique<TextVirtualFileMappingHandler>(stream.str(), MimeType::Type::GeoJson);
    PortableLocalhost::CreateVirtualFile(*virtual_file_mapping);

    const int geometry_id = m_data->next_map_id++;

    AddGeometryIMIS(m_data->geometries.try_emplace(geometry_id, MapGeometry { geometry_id,
                                                                              std::move(virtual_file_mapping),
                                                                              -1 }).first->second);
    return geometry_id;
}


void HtmlMapUI::AddGeometryIMIS(const MapGeometry& geometry)
{
    PostActionMessage("addGeometry",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, geometry.id)
                       .Write(JK::geojsonUrl, geometry.virtual_file_mapping->GetUrl());
        });
}


bool HtmlMapUI::RemoveGeometry(const int geometry_id)
{
    MapGeometry* const geometry = GetGeometry(geometry_id);

    if( geometry == nullptr )
        return false;

    const int leaflet_id = geometry->leaflet_id;

    m_data->geometries.erase(geometry_id);

    PostActionMessage("removeGeometry",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::leafletId, leaflet_id);
        });

    return true;
}


void HtmlMapUI::ClearGeometry()
{
    m_data->geometries.clear();

    PostActionMessage("clearGeometry");
}
