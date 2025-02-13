#include "stdafx.h"
#include "WindowsMapDlg.h"
#include "CurrentLocation.h"
#include "GeoJson.h"
#include "OfflineTileProvider.h"
#include <zToolsO/Encoders.h>
#include <zUtilO/MimeType.h>
#include <zHtml/PortableLocalhost.h>

#pragma warning(push)
#pragma warning(disable: 4068 4239)
#include <mapbox/feature.hpp>
#include <mapbox/geometry.hpp>
#pragma warning(pop)


BEGIN_MESSAGE_MAP(WindowsMapDlg, HtmlViewDlg)
    ON_MESSAGE(UWM::Mapping::ExecuteJavaScript, OnExecuteJavaScript)
    ON_MESSAGE(UWM::Mapping::SaveSnapshot, OnSaveSnapshot)
END_MESSAGE_MAP()


WindowsMapDlg::WindowsMapDlg(WindowsMapUI& map_ui, CWnd* pParent/*= nullptr*/)
    :   HtmlViewDlg(pParent),
        m_mapUI(map_ui),
        m_loaded(false)
{
    m_htmlViewCtrl.AddWebEventObserver([&](const std::wstring& message) { OnWebMessageReceived(message); });

    SetInitialUrl(m_mapUI.GetUrlOfMapHtml());

    // set the default dialog title (if not overridden already)
    if( !m_viewerOptions.title.has_value() )
        m_viewerOptions.title = _T("CSPro Map");
}


WindowsMapDlg::~WindowsMapDlg()
{
    m_mapUI.NotifyEvent(IMapUI::EventCode::MapClosed);
}


void WindowsMapDlg::RemoveMarker(int leaflet_id)
{
    ExecuteJavaScript(FormatText(_T("removeMarker(%d);"), leaflet_id));
}


void WindowsMapDlg::AddMarker(const WindowsMapUI::Marker& marker, int id)
{
    ExecuteJavaScript(FormatText(_T("addMarker(%g, %g, %d, %d, %d, \"%s\", \"%s\", \"%s\", \"%s\", \"%s\");"),
        marker.latitude,
        marker.longitude,
        marker.on_drag_callback,
        marker.on_info_window_click_callback,
        id,
        Encoders::ToEscapedString(marker.description).c_str(),
        Encoders::ToEscapedString(marker.text).c_str(),
        marker.background_color.ToStringRGB().c_str(),
        marker.text_color.ToStringRGB().c_str(),
        marker.image_url.c_str()));
}


void WindowsMapDlg::ClearMarkers()
{
    ExecuteJavaScript(_T("clearMarkers();"));
}


void WindowsMapDlg::MoveMarker(const WindowsMapUI::Marker& marker)
{
    ExecuteJavaScript(FormatText(_T("moveMarker(%g, %g, %d);"), marker.latitude, marker.longitude, marker.leaflet_id));
}


void WindowsMapDlg::FitPoints()
{
    ExecuteJavaScript(_T("fitPoints();"));
}


void WindowsMapDlg::SetDraggable(int leaflet_id)
{
    ExecuteJavaScript(FormatText(_T("setDraggable(%d);"), leaflet_id));
}


void WindowsMapDlg::SetDescription(const WindowsMapUI::Marker& marker, int id)
{
    ExecuteJavaScript(FormatText(_T("setDescription(%d, %d, %d, \"%s\");"), id, marker.leaflet_id,
                                 marker.on_info_window_click_callback,
                                 Encoders::ToEscapedString(marker.description).c_str()));
}


void WindowsMapDlg::SetText(const WindowsMapUI::Marker& marker)
{
    ExecuteJavaScript(FormatText(_T("setText(%d, \"%s\", \"%s\", \"%s\");"), marker.leaflet_id,
                                 Encoders::ToEscapedString(marker.text).c_str(),
                                 marker.background_color.ToStringRGB().c_str(),
                                 marker.text_color.ToStringRGB().c_str()));
}


void WindowsMapDlg::SetTitle(std::wstring title)
{
    ASSERT(m_viewerOptions.title.has_value());
    SetWindowText(title.empty() ? m_viewerOptions.title->c_str() : title.c_str());

    ExecuteJavaScript(FormatText(_T("setTitle(\"%s\");"), Encoders::ToEscapedString(std::move(title)).c_str()));
}


void WindowsMapDlg::AddTextButton(const WindowsMapUI::Button& button, int id)
{
    ExecuteJavaScript(FormatText(_T("addTextButton(\"%s\", %d);"), Encoders::ToEscapedString(button.content).c_str(), id));
}


void WindowsMapDlg::AddImageButton(const WindowsMapUI::Button& button, int id)
{
    ExecuteJavaScript(FormatText(_T("addImageButton(\"%s\", %d);"), button.content.c_str(), id));
}


void WindowsMapDlg::RemoveButton(int id)
{
    ExecuteJavaScript(FormatText(_T("removeButton(%d);"), id));
}


void WindowsMapDlg::ClearButtons()
{
    ExecuteJavaScript(_T("clearButtons();"));
}


void WindowsMapDlg::SetupCurrentLocation()
{
    if( m_mapUI.m_showCurrentLocation )
    {
        const std::optional<std::tuple<double, double>>& current_location = CurrentLocation::GetCurrentLocation();

        // only show the current location when it can be retrieved
        if( current_location.has_value() )
        {
            ExecuteJavaScript(FormatText(_T("showCurrentLocation(%g, %g);"), std::get<0>(*current_location),
                                                                             std::get<1>(*current_location)));
            return;
        }
    }

    ExecuteJavaScript(_T("hideCurrentLocation();"));
}


void WindowsMapDlg::SetZoom(double latitude, double longitude, double zoom)
{
    ExecuteJavaScript(FormatText(_T("setView(%g, %g, %g);"), latitude, longitude, zoom));
}


void WindowsMapDlg::SetZoom(double minLat, double maxLat, double minLong, double maxLong, double paddingPercent)
{
    ExecuteJavaScript(FormatText(_T("fitBounds(%g, %g, %g, %g, %g);"), minLat, minLong, maxLat, maxLong, paddingPercent));
}


void WindowsMapDlg::SetMarkerImage(const WindowsMapUI::Marker& marker)
{
    ExecuteJavaScript(FormatText(_T("setMarkerImage(\"%s\", %d);"), marker.image_url.c_str(), marker.leaflet_id));
}


void WindowsMapDlg::SetupBaseMap()
{
    // if the base map has not been manually set, use Normal
    if( !m_mapUI.m_baseMapSelection.has_value() )
        m_mapUI.m_baseMapSelection = BaseMap::Normal;

    ASSERT(std::holds_alternative<BaseMap>(*m_mapUI.m_baseMapSelection) == ( m_mapUI.m_tileProvider == nullptr ));

    if( std::holds_alternative<BaseMap>(*m_mapUI.m_baseMapSelection) )
    {
        BaseMap base_map = std::get<BaseMap>(*m_mapUI.m_baseMapSelection);

        if( base_map == BaseMap::None )
        {
            ExecuteJavaScript(_T("resetBaseMap();"));
        }

        else
        {
            const MappingTileProviderProperties& mapping_tile_provider_properties = m_mapUI.m_mappingProperties.GetWindowsMappingTileProviderProperties();

            ExecuteJavaScript(FormatText(_T("set%sBaseMap(\"%s\", \"%s\");"),
                                         ToString(mapping_tile_provider_properties.GetMappingTileProvider()),
                                         Encoders::ToEscapedString(mapping_tile_provider_properties.GetTileLayer(base_map)).c_str(),
                                         Encoders::ToEscapedString(mapping_tile_provider_properties.GetAccessToken()).c_str()));
        }
    }

    else
    {
        ExecuteJavaScript(FormatText(_T("setOfflineBaseMap(\"%s\", `%s`);"),
                                     m_mapUI.m_tileProvider->GetTileLayerUrl().c_str(),
                                     Encoders::ToEscapedString(m_mapUI.m_tileProvider->GetLeafletTileLayerOptions()).c_str()));
    }
}


void WindowsMapDlg::AddGeometry(const WindowsMapUI::MapGeometry& geometry, int id)
{
    std::ostringstream stream;
    GeoJson::toGeoJson(stream, *geometry.geometry);

    // serve the GeoJSON as a virtual file
    const auto& virtual_file_mapping = m_geometryVirtualFileMappings.emplace_back(std::make_unique<TextVirtualFileMappingHandler>(stream.str(), MimeType::Type::GeoJson));

    PortableLocalhost::CreateVirtualFile(*virtual_file_mapping);

    ExecuteJavaScript(FormatText(_T("addGeometry(%d, '%s');"), id, Encoders::ToEscapedString(virtual_file_mapping->GetUrl()).c_str()));
}


void WindowsMapDlg::RemoveGeometry(int leaflet_id)
{
    ExecuteJavaScript(FormatText(_T("removeGeometry(%d);"), leaflet_id));
}


void WindowsMapDlg::ClearGeometry()
{
    ExecuteJavaScript(_T("clearGeometry();"));
}


void WindowsMapDlg::OnWebMessageReceived(wstring_view message)
{
    auto json = Json::Parse(message);
    wstring_view action = json.Get<wstring_view>(_T("action"));
    IMapUI::MapCamera camera = IMapUI::MapCamera{ 0, 0, 0, 0 };

    if (json.Contains(_T("camera_lat")))
    {
        camera = IMapUI::MapCamera
        {
            json.Get<double>(_T("camera_lat")),
            json.Get<double>(_T("camera_lng")),
            json.Get<float>(_T("camera_zoom")),
            0
        };
    }


    if (action == _T("document_loaded"))
    {
        m_loaded = true;
        SetupInitialMap();
    }

    else if (action == _T("marker_click"))
    {
        int marker_id = json.Get<int>(_T("id"));
        WindowsMapUI::Marker* marker = m_mapUI.GetMarker(marker_id);

        if (marker != nullptr)
        {
            m_mapUI.NotifyEvent(IMapUI::EventCode::MarkerClicked,
                marker_id, marker->on_click_callback, marker->latitude, marker->longitude, camera);
        }
    }

    else if (action == _T("marker_placed"))
    {
        int marker_id = json.Get<int>(_T("id"));
        int leaflet_id = json.Get<int>(_T("leaflet_id"));
        WindowsMapUI::Marker* marker = m_mapUI.GetMarker(marker_id);
        marker->leaflet_id = leaflet_id;
    }

    else if (action == _T("marker_drag"))
    {
        int marker_id = json.Get<int>(_T("id"));
        WindowsMapUI::Marker* marker = m_mapUI.GetMarker(marker_id);
        if (marker != nullptr)
        {
            marker->latitude = json.Get<double>(_T("latitude"));
            marker->longitude = json.Get<double>(_T("longitude"));
            m_mapUI.NotifyEvent(IMapUI::EventCode::MarkerDragged,
                marker_id, marker->on_drag_callback, marker->latitude, marker->longitude, camera);
        }
    }

    else if (action == _T("map_click"))
    {
        m_mapUI.NotifyEvent(IMapUI::EventCode::MapClicked, -1, -1,
            json.Get<double>(_T("latitude")), json.Get<double>(_T("longitude")), camera);
    }

    else if (action == _T("popup_click"))
    {
        int marker_id = json.Get<int>(_T("id"));
        WindowsMapUI::Marker* marker = m_mapUI.GetMarker(marker_id);
        m_mapUI.NotifyEvent(IMapUI::EventCode::MarkerInfoWindowClicked,
            marker_id, marker->on_info_window_click_callback, marker->latitude, marker->longitude, camera);
    }

    else if (action == _T("button_click"))
    {
        int button_id = json.Get<int>(_T("id"));
        WindowsMapUI::Button* button = m_mapUI.GetButton(button_id);
        m_mapUI.NotifyEvent(IMapUI::EventCode::ButtonClicked, button_id, button->on_click_callback, 0, 0, camera);
    }

    else if (action == _T("geometry_placed"))
    {
        int geometry_id = json.Get<int>(_T("id"));
        int leaflet_id = json.Get<int>(_T("leaflet_id"));
        WindowsMapUI::MapGeometry* geometry = m_mapUI.GetGeometry(geometry_id);
        geometry->leaflet_id = leaflet_id;
    }
}


void WindowsMapDlg::SetupInitialMap()
{
    ASSERT(m_loaded);

    // set the title
    SetTitle(m_mapUI.m_title);

    // setup the base map
    SetupBaseMap();

    // add markers
    for( const auto& [id, marker] : m_mapUI.m_markers )
    {
        AddMarker(marker, id);
    }

    // add buttons
    for (const auto& [id, button] : m_mapUI.m_buttons)
    {
        if (button.type == WindowsMapUI::ButtonType::Text)
        {
            AddTextButton(button, id);
        }
        
        else
        {
            AddImageButton(button, id);
        }
    }

    // show or hide the current location
    SetupCurrentLocation();

    // set the zoom
    if (m_mapUI.m_zoom.has_value())
    {
        const WindowsMapUI::Zoom& zoom = *m_mapUI.m_zoom;

        if (zoom.latitude2 > -91)
        {
            SetZoom(zoom.latitude, zoom.latitude2, zoom.longitude, zoom.longitude2, zoom.level);
        }

        else
        {
            //need to set initial zoom, 7 seems like a nice number
            SetZoom(zoom.latitude, zoom.longitude, zoom.level > 0 ? zoom.level : 7);
        }
    }

    else
    {
        FitPoints();
    }

    // add geometries
    for( const auto& [id, geometry] : m_mapUI.m_geometries )
    {
        AddGeometry(geometry, id);
    }
}


void WindowsMapDlg::ExecuteJavaScript(const TCHAR* javascript)
{
    // send a message to execute the JavaScript so that code executed
    // from the engine thread runs on the UI thread
    SendMessage(UWM::Mapping::ExecuteJavaScript, reinterpret_cast<WPARAM>(javascript));
}


LRESULT WindowsMapDlg::OnExecuteJavaScript(WPARAM wParam, LPARAM /*lParam*/)
{
    if( m_loaded )
    {
        const TCHAR* javascript = reinterpret_cast<const TCHAR*>(wParam);
        m_htmlViewCtrl.ExecuteScript(javascript);
    }

    return 0;
}


void WindowsMapDlg::SaveSnapshot(const std::wstring& filename)
{
    // send a message to save the snapsnot on the UI thread
    std::optional<std::wstring> exception_message;

    SendMessage(UWM::Mapping::SaveSnapshot, reinterpret_cast<WPARAM>(&filename), reinterpret_cast<LPARAM>(&exception_message));

    if( exception_message.has_value() )
        throw CSProException(*exception_message);
}


LRESULT WindowsMapDlg::OnSaveSnapshot(WPARAM wParam, LPARAM lParam)
{
    const std::wstring& filename = *reinterpret_cast<const std::wstring*>(wParam);
    std::optional<std::wstring>& exception_message = *reinterpret_cast<std::optional<std::wstring>*>(lParam);

    try
    {
        m_htmlViewCtrl.SaveScreenshot(filename);
    }

    catch( const CSProException& exception )
    {
        exception_message = exception.GetErrorMessage();
    }

    return 0;
}
