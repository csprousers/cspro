#include "stdafx.h"
#include "HtmlMapUI.h"
#include "GeoJson.h"
#include "MBTilesReader.h"
#include "OfflineTileProvider.h"
#include "TPKReader.h"
#include <zToolsO/Encoders.h>
#include <zUtilO/PortableColor.h>
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


CREATE_JSON_KEY(backgroundColor)
CREATE_JSON_KEY(callbackIndex)
CREATE_JSON_KEY(camera)
CREATE_JSON_KEY(draggable)
CREATE_JSON_KEY(geojsonUrl)
CREATE_JSON_KEY(imageUrl)
CREATE_JSON_KEY(leafletId)
CREATE_JSON_KEY(maxLatitude)
CREATE_JSON_KEY(maxLongitude)
CREATE_JSON_KEY(minLatitude)
CREATE_JSON_KEY(minLongitude)
CREATE_JSON_KEY(options)
CREATE_JSON_KEY(padding)
CREATE_JSON_KEY(zoom)


// --------------------------------------------------------------------------
// HtmlMapUI objects
//
// variable names with the suffix:
//     _html = passed through HtmlishSanitizer
//     _text = passed through HtmlTextConverter
// --------------------------------------------------------------------------

struct HtmlMapUI::Button
{
    int id;
    int on_click_callback;
    std::variant<std::string, SharableString> image_url_or_label_html;
};


struct HtmlMapUI::MapGeometry
{
    int id;
    std::unique_ptr<VirtualFileMappingHandler> virtual_file_mapping;
    int leaflet_id;
};


struct HtmlMapUI::Marker
{
    int id;
    double latitude = 0;
    double longitude = 0;
    int on_click_callback = -1;
    int on_drag_callback = -1;
    int on_info_window_click_callback = -1;
    int leaflet_id = -1;
    std::string image_url;
    SharableString description_html;
    SharableString text_html;
    PortableColor background_color = PortableColor::White;
    PortableColor text_color = PortableColor::Black;
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
    std::vector<std::tuple<int, std::unique_ptr<JsonStringWriter>>> pending_action_messages; // id -> message
    std::mutex pending_action_messages_mutex;

    SharableString title_html;
    std::string title_text;

    std::optional<BaseMapSelection> base_map_selection;
    std::shared_ptr<OfflineTileReader> tile_reader;
    std::unique_ptr<OfflineTileProvider> tile_provider;

    bool show_current_location = true;

    std::variant<std::monostate, Zoom1, Zoom2> zoom;

    int next_map_id = 1;

    std::map<int, Button> buttons;
    std::map<int, Marker> markers;
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


std::unique_ptr<JsonStringWriter> HtmlMapUI::InitializePostActionMessage(const cs::string_sz action,
                                                                         const std::function<void(JsonWriter&)>& callback_function)
{
    if( !IsMapShowing() )
        return nullptr;

    std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::action, action);

    callback_function(*json_writer);

    return json_writer;
}


void HtmlMapUI::FinalizePostActionMessage(JsonStringWriter& json_writer)
{
    ASSERT(IsMapShowing());

    json_writer.EndObject();

    OnPostActionMessage(json_writer.ReleaseSharableString());
}


void HtmlMapUI::FinalizePostActionMessage(const int leaflet_id, JsonStringWriter& json_writer)
{
    ASSERT(leaflet_id != -1);

    json_writer.Write(JK::leafletId, leaflet_id);

    FinalizePostActionMessage(json_writer);
}


void HtmlMapUI::PostActionMessage(const cs::string_sz action,
                                  const std::function<void(JsonWriter&)>& callback_function)
{
    const std::unique_ptr<JsonStringWriter> json_writer = InitializePostActionMessage(action, callback_function);

    if( json_writer == nullptr )
        return;

    FinalizePostActionMessage(*json_writer);
}


void HtmlMapUI::PostActionMessage(const cs::string_sz action, const int id, const int leaflet_id,
                                  const std::function<void(JsonWriter&)>& callback_function)
{
    std::unique_ptr<JsonStringWriter> json_writer = InitializePostActionMessage(action, callback_function);

    if( json_writer == nullptr )
        return;

    // if the Leaflet ID has not been set yet (via one of the ...Placed web messages),
    // hold this message until it is set
    if( leaflet_id == -1 )
    {
        const std::lock_guard<std::mutex> lock(m_data->pending_action_messages_mutex);
        m_data->pending_action_messages.emplace_back(id, std::move(json_writer));
    }

    else
    {
        FinalizePostActionMessage(leaflet_id, *json_writer);
    }
}


void HtmlMapUI::ProcessPendingActionMessages(const int id, const int leaflet_id)
{
    ASSERT(IsMapShowing() && leaflet_id != -1);

    const std::lock_guard<std::mutex> lock(m_data->pending_action_messages_mutex);

    if( m_data->pending_action_messages.empty() )
        return;

    // finalize any messages are that connected to this ID
    for( auto itr = m_data->pending_action_messages.begin(); itr != m_data->pending_action_messages.end(); )
    {
        if( std::get<0>(*itr) == id )
        {
            ASSERT(std::get<1>(*itr) != nullptr);
            FinalizePostActionMessage(leaflet_id, *std::get<1>(*itr));
            itr = m_data->pending_action_messages.erase(itr);
        }

        else
        {
            ++itr;
        }
    }
}


void HtmlMapUI::OnWebMessageReceived(const std::string_view message_sv)
{
    try
    {
        const JsonNode json_node = Json::Parse(message_sv);
        const std::string_view action_sv = json_node.Get<std::string_view>(JK::action);

        const JsonNode camera_json_node = json_node.GetOrEmpty(JK::camera);
        const MapCamera camera = camera_json_node.IsEmpty() ? MapCamera { 0, 0, 0, 0 } :
                                                              MapCamera { camera_json_node.Get<double>(JK::latitude),
                                                                          camera_json_node.Get<double>(JK::longitude),
                                                                          camera_json_node.Get<float>(JK::zoom),
                                                                          0 };

        if( action_sv == "documentLoaded" )
        {
            SetUpInitialMapIMIS();
        }

        else if( action_sv == "mapClick" )
        {
            NotifyEvent(EventCode::MapClicked,
                        -1, -1,
                        json_node.Get<double>(JK::latitude), json_node.Get<double>(JK::longitude),
                        camera);
        }

        else if( action_sv == "markerPlaced" )
        {
            const int marker_id = json_node.Get<int>(JK::id);
            const int leaflet_id = json_node.Get<int>(JK::leafletId);
            Marker* const marker = GetMarker(marker_id);

            if( marker != nullptr )
            {
                marker->leaflet_id = leaflet_id;
                ProcessPendingActionMessages(marker->id, leaflet_id);
            }
        }

        else if( action_sv == "markerClick" )
        {
            const int marker_id = json_node.Get<int>(JK::id);
            Marker* const marker = GetMarker(marker_id);

            if( marker != nullptr )
            {
                NotifyEvent(EventCode::MarkerClicked,
                            marker_id, marker->on_click_callback,
                            marker->latitude, marker->longitude,
                            camera);
            }
        }

        else if( action_sv == "markerPopup" )
        {
            const int marker_id = json_node.Get<int>(JK::id);
            Marker* const marker = GetMarker(marker_id);

            if( marker != nullptr )
            {
                NotifyEvent(EventCode::MarkerInfoWindowClicked,
                            marker_id, marker->on_info_window_click_callback,
                            marker->latitude, marker->longitude,
                            camera);
            }
        }

        else if( action_sv == "markerDrag" )
        {
            const int marker_id = json_node.Get<int>(JK::id);
            Marker* const marker = GetMarker(marker_id);

            if( marker != nullptr )
            {
                marker->latitude = json_node.Get<double>(JK::latitude);
                marker->longitude = json_node.Get<double>(JK::longitude);

                NotifyEvent(EventCode::MarkerDragged,
                            marker_id, marker->on_drag_callback,
                            marker->latitude, marker->longitude,
                            camera);
            }
        }

        else if( action_sv == "buttonClick" )
        {
            const int button_id = json_node.Get<int>(JK::id);
            Button* const button = GetButton(button_id);

            if( button != nullptr )
            {
                NotifyEvent(EventCode::ButtonClicked,
                            button_id, button->on_click_callback,
                            0.0, 0.0,
                            camera);
            }
        }

        else if( action_sv == "geometryPlaced" )
        {
            const int geometry_id = json_node.Get<int>(JK::id);
            const int leaflet_id = json_node.Get<int>(JK::leafletId);
            MapGeometry* const geometry = GetGeometry(geometry_id);

            if( geometry != nullptr )
            {
                geometry->leaflet_id = leaflet_id;
                ProcessPendingActionMessages(geometry->id, leaflet_id);
            }
        }
    }
    catch(...) { ASSERT(false); };
}


void HtmlMapUI::Clear()
{
    HtmlMapUI::SetTitle(SharableString());

    HtmlMapUI::SetBaseMapWorker(std::nullopt);

    HtmlMapUI::SetShowCurrentLocation(true);

    HtmlMapUI::ClearMarkers();
    HtmlMapUI::ClearButtons();
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

    // add markers
    for( const auto& [id, marker] : m_data->markers )
        AddMarkerIMIS(marker);

    // add buttons
    for( const auto& [id, button] : m_data->buttons )
        AddButtonIMIS(button);

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
                    const MappingTileProviderProperties& mapping_tile_provider_properties = m_mappingProperties->GetMappingTileProviderProperties();

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

    return SetShowCurrentLocationIMIS();
}


bool HtmlMapUI::SetShowCurrentLocationIMIS()
{
    if( m_data->show_current_location )
    {
        return OnShowCurrentLocation();
    }

    else
    {
        OnHideCurrentLocation();
        return true;
    }
}


void HtmlMapUI::OnHideCurrentLocation()
{
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
        PostActionMessage("fitMarkers");
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


HtmlMapUI::Marker* HtmlMapUI::GetMarker(const int marker_id)
{
    const auto& lookup = m_data->markers.find(marker_id);
    return ( lookup != m_data->markers.cend() ) ? &lookup->second :
                                                  nullptr;
}


int HtmlMapUI::AddMarker(const double latitude, const double longitude)
{
    const int marker_id = m_data->next_map_id++;

    AddMarkerIMIS(m_data->markers.try_emplace(marker_id, Marker { marker_id,
                                                                  latitude,
                                                                  longitude }).first->second);

    return marker_id;
}


void HtmlMapUI::AddMarkerIMIS(const Marker& marker)
{
    PostActionMessage("addMarker",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, marker.id)
                       .Write(JK::latitude, marker.latitude)
                       .Write(JK::longitude, marker.longitude)
                       .Write(JK::draggable, ( marker.on_drag_callback >= 0 ))
                       .Write(JK::callbackIndex, marker.on_info_window_click_callback)
                       .Write(JK::description, marker.description_html)
                       .Write(JK::text, marker.text_html)
                       .Write(JK::backgroundColor, marker.background_color)
                       .Write(JK::textColor, marker.text_color)
                       .Write(JK::imageUrl, marker.image_url);
        });
}



bool HtmlMapUI::RemoveMarker(const int marker_id)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    const int leaflet_id = marker->leaflet_id;

    m_data->markers.erase(marker_id);

    PostActionMessage("removeMarker", marker_id, leaflet_id,
        [](JsonWriter& /*json_writer*/)
        {
        });

    return true;
}


void HtmlMapUI::ClearMarkers()
{
    m_data->markers.clear();

    PostActionMessage("clearMarkers");
}


bool HtmlMapUI::SetMarkerImage(const int marker_id, const std::string& image_url_or_file_path)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->image_url = GetUrlForUrlOrFile(image_url_or_file_path);

    PostActionMessage("setMarkerImage", marker_id, marker->leaflet_id,
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::imageUrl, marker->image_url);
        });

    return true;
}


bool HtmlMapUI::SetMarkerText(const int marker_id, SharableString text, const int background_color, const int text_color)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->text_html = HtmlishSanitizer::Sanitize(std::move(text));
    marker->background_color = PortableColor::FromColorInt(background_color);
    marker->text_color = PortableColor::FromColorInt(text_color);

    PostActionMessage("setMarkerText", marker_id, marker->leaflet_id,
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::text, marker->text_html)
                       .Write(JK::backgroundColor, marker->background_color)
                       .Write(JK::textColor, marker->text_color);
        });

    return true;
}


bool HtmlMapUI::SetMarkerDescription(const int marker_id, SharableString description)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->description_html = HtmlishSanitizer::Sanitize(std::move(description));

    SetMarkerDescriptionIMIS(*marker);

    return true;
}


void HtmlMapUI::SetMarkerDescriptionIMIS(const Marker& marker)
{
    PostActionMessage("setMarkerDescription", marker.id, marker.leaflet_id,
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, marker.id)
                       .Write(JK::description, marker.description_html)
                       .Write(JK::callbackIndex, marker.on_info_window_click_callback);
        });
}


bool HtmlMapUI::SetMarkerOnClick(const int marker_id, const int on_click_callback)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->on_click_callback = on_click_callback;

    return true;
}


bool HtmlMapUI::SetMarkerOnClickInfoWindow(const int marker_id, const int on_click_callback)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->on_info_window_click_callback = on_click_callback;

    SetMarkerDescriptionIMIS(*marker);

    return true;
}


bool HtmlMapUI::SetMarkerOnDrag(const int marker_id, const int on_drag_callback)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->on_drag_callback = on_drag_callback;

    PostActionMessage("setMarkerOnDrag", marker_id, marker->leaflet_id,
        [](JsonWriter& /*json_writer*/)
        {
        });

    return true;
}


bool HtmlMapUI::SetMarkerLocation(const int marker_id, const double latitude, const double longitude)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return false;

    marker->latitude = latitude;
    marker->longitude = longitude;

    PostActionMessage("setMarkerLocation", marker_id, marker->leaflet_id,
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::latitude, marker->latitude)
                       .Write(JK::longitude, marker->longitude);
        });

    return true;
}


std::optional<std::tuple<double, double>> HtmlMapUI::GetMarkerLocation(const int marker_id)
{
    Marker* const marker = GetMarker(marker_id);

    if( marker == nullptr )
        return std::nullopt;

    return std::make_tuple(marker->latitude, marker->longitude);
}


HtmlMapUI::Button* HtmlMapUI::GetButton(const int button_id)
{
    const auto& lookup = m_data->buttons.find(button_id);
    return ( lookup != m_data->buttons.cend() ) ? &lookup->second :
                                                  nullptr;
}


int HtmlMapUI::AddImageButton(const std::string& image_url_or_file_path, const int on_click_callback)
{
    const int button_id = m_data->next_map_id++;

    AddButtonIMIS(m_data->buttons.try_emplace(button_id, Button { button_id,
                                                                  on_click_callback,
                                                                  GetUrlForUrlOrFile(image_url_or_file_path) }).first->second);

    return button_id;
}


int HtmlMapUI::AddTextButton(SharableString label, const int on_click_callback)
{
    const int button_id = m_data->next_map_id++;

    AddButtonIMIS(m_data->buttons.try_emplace(button_id, Button { button_id,
                                                                  on_click_callback,
                                                                  HtmlishSanitizer::Sanitize(std::move(label)) }).first->second);

    return button_id;
}


void HtmlMapUI::AddButtonIMIS(const Button& button)
{
    const bool is_image_button = std::holds_alternative<std::string>(button.image_url_or_label_html);

    PostActionMessage(is_image_button ? "addImageButton" : "addTextButton",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, button.id);

            if( is_image_button )
            {
                json_writer.Write(JK::imageUrl, std::get<std::string>(button.image_url_or_label_html));
            }

            else
            {
                json_writer.Write(JK::text, std::get<SharableString>(button.image_url_or_label_html));
            }
        });
}


bool HtmlMapUI::RemoveButton(const int button_id)
{
    Button* const button = GetButton(button_id);

    if( button == nullptr )
        return false;

    m_data->buttons.erase(button_id);

    PostActionMessage("removeButton",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, button_id);
        });

    return true;
}


void HtmlMapUI::ClearButtons()
{
    m_data->buttons.clear();

    PostActionMessage("clearButtons");
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

    PostActionMessage("removeGeometry", geometry_id, leaflet_id,
        [](JsonWriter& /*json_writer*/)
        {
        });

    return true;
}


void HtmlMapUI::ClearGeometry()
{
    m_data->geometries.clear();

    PostActionMessage("clearGeometry");
}
