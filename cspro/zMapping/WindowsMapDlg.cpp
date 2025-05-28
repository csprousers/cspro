#include "stdafx.h"
#include "WindowsMapDlg.h"
#include "GeoJson.h"
#include <zToolsO/Encoders.h>
#include <zToolsO/Utf8.h>
#include <zUtilO/MimeType.h>
#include <zHtml/PortableLocalhost.h>
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
CREATE_JSON_KEY(padding)
CREATE_JSON_KEY(zoom)


BEGIN_MESSAGE_MAP(WindowsMapDlg, HtmlViewDlg)
    ON_MESSAGE(UWM::Mapping::PostActionMessage, OnPostActionMessage)
    ON_MESSAGE(UWM::Mapping::SaveSnapshot, OnSaveSnapshot)
END_MESSAGE_MAP()


WindowsMapDlg::WindowsMapDlg(WindowsMapUI& map_ui, CWnd* const pParent/*= nullptr*/)
    :   HtmlViewDlg(pParent),
        m_mapUI(map_ui),
        m_loaded(false)
{
    m_htmlViewCtrl.AddWebEventObserver([&](const std::wstring_view message_sv) { OnWebMessageReceived(TC::ToUtf8(message_sv)); });

    SetInitialUrl(m_mapUI.GetUrlOfMapHtml());

    // set the default dialog title (if not overridden already)
    if( !m_viewerOptions.title.IsSet() )
        m_viewerOptions.title = "CSPro Map";
}


WindowsMapDlg::~WindowsMapDlg()
{
    m_mapUI.NotifyEvent(IMapUI::EventCode::MapClosed);
}


LRESULT WindowsMapDlg::OnPostActionMessage(const WPARAM wParam, LPARAM /*lParam*/)
{
    const SharableString message = WindowsDesktopMessage::GetPostedObject<SharableString>(wParam);
    ASSERT(message.IsSet());

    m_htmlViewCtrl.PostWebMessageAsString(*message);

    return 1;
}


void WindowsMapDlg::PostActionMessage(const std::string_view action_sv)
{
    WindowsDesktopMessage::PostObject(this, UWM::Mapping::PostActionMessage,
                                      "{\"action\":" + Encoders::ToJsonString(action_sv) + "}");
}


template<typename CF>
void WindowsMapDlg::PostActionMessage(const cs::string_sz action, const CF& callback_function)
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write(JK::action, action);

    callback_function(*json_writer);

    json_writer->EndObject();

    WindowsDesktopMessage::PostObject(this, UWM::Mapping::PostActionMessage,
                                      json_writer->ReleaseSharableString());
}


void WindowsMapDlg::AddMarker(const WindowsMapUI::Marker& marker, const int id)
{
    PostActionMessage("addMarker",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, id)
                       .Write(JK::latitude, marker.latitude)
                       .Write(JK::longitude, marker.longitude)
                       .Write(JK::draggable, ( marker.on_drag_callback >= 0 ))
                       .Write(JK::callbackIndex, marker.on_info_window_click_callback)
                       .Write(JK::description, m_htmlishSanitizer.Sanitize(*marker.description))
                       .Write(JK::text, m_htmlishSanitizer.Sanitize(*marker.text))
                       .Write(JK::backgroundColor, marker.background_color)
                       .Write(JK::textColor, marker.text_color)
                       .Write(JK::imageUrl, marker.image_url);
        });
}


void WindowsMapDlg::RemoveMarker(const int leaflet_id)
{
    PostActionMessage("removeMarker",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::leafletId, leaflet_id);
        });
}


void WindowsMapDlg::ClearMarkers()
{
    PostActionMessage("clearMarkers");
}


void WindowsMapDlg::SetMarkerImage(const WindowsMapUI::Marker& marker)
{
    PostActionMessage("setMarkerImage",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::leafletId, marker.leaflet_id)
                       .Write(JK::imageUrl, marker.image_url);
        });
}


void WindowsMapDlg::SetMarkerText(const WindowsMapUI::Marker& marker)
{
    PostActionMessage("setMarkerText",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::leafletId, marker.leaflet_id)
                       .Write(JK::text, m_htmlishSanitizer.Sanitize(*marker.text))
                       .Write(JK::backgroundColor, marker.background_color)
                       .Write(JK::textColor, marker.text_color);
        });
}


void WindowsMapDlg::SetMarkerOnDrag(const int leaflet_id)
{
    PostActionMessage("setMarkerOnDrag",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::leafletId, leaflet_id);
        });
}


void WindowsMapDlg::SetMarkerDescription(const WindowsMapUI::Marker& marker, const int id)
{
    PostActionMessage("setMarkerDescription",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, id)
                       .Write(JK::leafletId, marker.leaflet_id)
                       .Write(JK::description, m_htmlishSanitizer.Sanitize(*marker.description))
                       .Write(JK::callbackIndex, marker.on_info_window_click_callback);
        });
}


void WindowsMapDlg::SetMarkerLocation(const WindowsMapUI::Marker& marker)
{
    PostActionMessage("setMarkerLocation",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::leafletId, marker.leaflet_id)
                       .Write(JK::latitude, marker.latitude)
                       .Write(JK::longitude, marker.longitude);
        });
}


void WindowsMapDlg::FitMarkers()
{
    PostActionMessage("fitMarkers");
}


void WindowsMapDlg::AddImageButton(const WindowsMapUI::Button& button, const int id)
{
    PostActionMessage("addImageButton",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, id)
                       .Write(JK::imageUrl, button.content);
        });
}


void WindowsMapDlg::AddTextButton(const WindowsMapUI::Button& button, const int id)
{
    PostActionMessage("addTextButton",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, id)
                       .Write(JK::text, m_htmlishSanitizer.Sanitize(*button.content));
        });
}


void WindowsMapDlg::RemoveButton(const int id)
{
    PostActionMessage("removeButton",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, id);
        });
}


void WindowsMapDlg::ClearButtons()
{
    PostActionMessage("clearButtons");
}


void WindowsMapDlg::ZoomTo(const double latitude, const double longitude, const double zoom)
{
    PostActionMessage("zoomTo",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::latitude, latitude)
                       .Write(JK::longitude, longitude)
                       .Write(JK::zoom, zoom);
        });
}


void WindowsMapDlg::ZoomTo(const double min_latitude, const double min_longitude,
                            const double max_latitude, const double max_longitude,
                            const double padding_percent)
{
    PostActionMessage("zoomTo",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::minLatitude, min_latitude)
                       .Write(JK::minLongitude, min_longitude)
                       .Write(JK::maxLatitude, max_latitude)
                       .Write(JK::maxLongitude, max_longitude)
                       .Write(JK::padding, padding_percent);
        });
}


void WindowsMapDlg::AddGeometry(const WindowsMapUI::MapGeometry& geometry, const int id)
{
    std::ostringstream stream;
    GeoJson::toGeoJson(stream, *geometry.geometry);

    // serve the GeoJSON as a virtual file
    const auto& virtual_file_mapping = m_geometryVirtualFileMappings.emplace_back(std::make_unique<TextVirtualFileMappingHandler>(stream.str(), MimeType::Type::GeoJson));

    PortableLocalhost::CreateVirtualFile(*virtual_file_mapping);

    PostActionMessage("addGeometry",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::id, id)
                       .Write(JK::geojsonUrl, virtual_file_mapping->GetUrl());
        });
}


void WindowsMapDlg::RemoveGeometry(const int leaflet_id)
{
    PostActionMessage("removeGeometry",
        [&](JsonWriter& json_writer)
        {
            json_writer.Write(JK::leafletId, leaflet_id);
        });
}


void WindowsMapDlg::ClearGeometry()
{
    PostActionMessage("clearGeometry");
}


void WindowsMapDlg::OnWebMessageReceived(const std::string_view message_sv)
{
    try
    {
        m_mapUI.OnWebMessageReceived(message_sv);
        OnWebMessageReceived(Json::Parse(message_sv));
    }
    catch(...) { ASSERT(false); }
}


void WindowsMapDlg::OnWebMessageReceived(const JsonNode json_node)
{
    const std::string_view action_sv = json_node.Get<std::string_view>(JK::action);

    const JsonNode camera_json_node = json_node.GetOrEmpty(JK::camera);
    const IMapUI::MapCamera camera = camera_json_node.IsEmpty() ? IMapUI::MapCamera { 0, 0, 0, 0 } :
                                                                  IMapUI::MapCamera { camera_json_node.Get<double>(JK::latitude),
                                                                                      camera_json_node.Get<double>(JK::longitude),
                                                                                      camera_json_node.Get<float>(JK::zoom),
                                                                                      0 };

    if( action_sv == "documentLoaded" )
    {
        m_loaded = true;
        SetUpInitialMap();
    }

    else if( action_sv == "mapClick" )
    {
        m_mapUI.NotifyEvent(IMapUI::EventCode::MapClicked, -1, -1,
                            json_node.Get<double>(JK::latitude), json_node.Get<double>(JK::longitude), camera);
    }

    else if( action_sv == "markerPlaced" )
    {
        const int marker_id = json_node.Get<int>(JK::id);
        const int leaflet_id = json_node.Get<int>(JK::leafletId);
        WindowsMapUI::Marker* const marker = m_mapUI.GetMarker(marker_id);

        if( marker != nullptr )
            marker->leaflet_id = leaflet_id;
    }

    else if( action_sv == "markerClick" )
    {
        const int marker_id = json_node.Get<int>(JK::id);
        WindowsMapUI::Marker* const marker = m_mapUI.GetMarker(marker_id);

        if( marker != nullptr )
        {
            m_mapUI.NotifyEvent(IMapUI::EventCode::MarkerClicked, marker_id,
                                marker->on_click_callback, marker->latitude, marker->longitude, camera);
        }
    }

    else if( action_sv == "markerPopup" )
    {
        const int marker_id = json_node.Get<int>(JK::id);
        WindowsMapUI::Marker* const marker = m_mapUI.GetMarker(marker_id);

        if( marker != nullptr )
        {
            m_mapUI.NotifyEvent(IMapUI::EventCode::MarkerInfoWindowClicked, marker_id,
                                marker->on_info_window_click_callback, marker->latitude, marker->longitude, camera);
        }
    }

    else if( action_sv == "markerDrag" )
    {
        const int marker_id = json_node.Get<int>(JK::id);
        WindowsMapUI::Marker* const marker = m_mapUI.GetMarker(marker_id);

        if( marker != nullptr )
        {
            marker->latitude = json_node.Get<double>(JK::latitude);
            marker->longitude = json_node.Get<double>(JK::longitude);

            m_mapUI.NotifyEvent(IMapUI::EventCode::MarkerDragged, marker_id,
                                marker->on_drag_callback, marker->latitude, marker->longitude, camera);
        }
    }

    else if( action_sv == "buttonClick" )
    {
        const int button_id = json_node.Get<int>(JK::id);
        WindowsMapUI::Button* const button = m_mapUI.GetButton(button_id);

        if( button != nullptr )
        {
            m_mapUI.NotifyEvent(IMapUI::EventCode::ButtonClicked, button_id,
                                button->on_click_callback, 0, 0, camera);
        }
    }

    else if( action_sv == "geometryPlaced" )
    {
        const int geometry_id = json_node.Get<int>(JK::id);
        const int leaflet_id = json_node.Get<int>(JK::leafletId);
        WindowsMapUI::MapGeometry* const geometry = m_mapUI.GetGeometry(geometry_id);

        if( geometry != nullptr )
            geometry->leaflet_id = leaflet_id;
    }
}


void WindowsMapDlg::SetUpInitialMap()
{
    ASSERT(m_loaded);

    // add markers
    for( const auto& [id, marker] : m_mapUI.m_markers )
    {
        AddMarker(marker, id);
    }

    // add buttons
    for( const auto& [id, button] : m_mapUI.m_buttons )
    {
        if( button.type == WindowsMapUI::Button::Type::Text )
        {
            AddTextButton(button, id);
        }

        else
        {
            AddImageButton(button, id);
        }
    }

    // set the zoom
    if( m_mapUI.m_zoom != nullptr )
    {
        const WindowsMapUI::Zoom& zoom = *m_mapUI.m_zoom;

        if( zoom.latitude2 > -91 )
        {
            ZoomTo(zoom.latitude, zoom.longitude, zoom.latitude2, zoom.longitude2, zoom.level);
        }

        else
        {
            // need to set initial zoom, 7 seems like a nice number
            ZoomTo(zoom.latitude, zoom.longitude, ( zoom.level > 0 ) ? zoom.level : 7);
        }
    }

    else
    {
        FitMarkers();
    }

    // add geometries
    for( const auto& [id, geometry] : m_mapUI.m_geometries )
    {
        AddGeometry(geometry, id);
    }
}


struct WindowsMapDlg::SnapshotData
{
    const std::string& file_path;
    std::optional<std::string> exception_message;
};


void WindowsMapDlg::SaveSnapshot(const std::string& file_path)
{
    // send a message to save the snapshot on the UI thread
    SnapshotData snapshot_data { file_path };
    SendMessage(UWM::Mapping::SaveSnapshot, reinterpret_cast<WPARAM>(&snapshot_data));

    if( snapshot_data.exception_message.has_value() )
        throw CSProException(*snapshot_data.exception_message);
}


LRESULT WindowsMapDlg::OnSaveSnapshot(const WPARAM wParam, LPARAM /*lParam*/)
{
    SnapshotData& snapshot_data = *reinterpret_cast<SnapshotData*>(wParam);
    ASSERT(!snapshot_data.exception_message.has_value());

    try
    {
        m_htmlViewCtrl.SaveScreenshot(snapshot_data.file_path);
    }

    catch( const CSProException& exception )
    {
        snapshot_data.exception_message.emplace(exception.what());
    }

    return 0;
}


void WindowsMapDlg::SetWindowTitle(const std::string& title)
{
    ASSERT(m_viewerOptions.title.IsSet());
    SetWindowText(TC::ToWide(title.empty() ? *m_viewerOptions.title : title).c_str());
}
