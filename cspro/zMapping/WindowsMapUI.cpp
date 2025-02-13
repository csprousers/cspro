#include "stdafx.h"
#include "WindowsMapUI.h"
#include "MBTilesReader.h"
#include "OfflineTileProvider.h"
#include "OfflineTileReader.h"
#include "TPKReader.h"
#include "WindowsMapDlg.h"
#include "WindowsMapUIThreadRunner.h"
#include <zHtml/SharedHtmlLocalFileServer.h>


WindowsMapUI::WindowsMapUI(const MappingProperties& mapping_properties)
    :   m_mappingProperties(mapping_properties),
        m_showCurrentLocation(true),
        m_nextMapId(1)
{
}


WindowsMapUI::~WindowsMapUI()
{
    WaitForShowThreadToTerminate();
}


void WindowsMapUI::WaitForShowThreadToTerminate()
{
    auto old_ui_thread_runner = m_uiThreadRunner;
    m_uiThreadRunner.reset();

    auto old_show_thread = std::move(m_showThread);

    if( old_show_thread != nullptr && old_show_thread->joinable() )
        old_show_thread->join();
}


int WindowsMapUI::Show()
{
    EnsureFileServerIsSetup();

    return WindowsShow();
}


int WindowsMapUI::WindowsShow()
{
    // return if the map is already showing
    if( m_uiThreadRunner != nullptr )
        return 0;

    ASSERT(m_showThread == nullptr);

    if( m_mapEvent != nullptr )
    {
        // if the map was hidden using map.hide(), there will be an
        // unprocessed map closing event posted by WindowsMapDlg's destructor
        std::lock_guard<std::mutex> lock(m_mapEventMutex);
        ASSERT(m_mapEvent->code_ == IMapUI::EventCode::MapClosed);
        m_mapEvent.reset();
    }

    // after showing the map, we must return to the engine,
    // so the map dialog will be launched in a new thread
    m_uiThreadRunner = std::make_shared<WindowsMapUIThreadRunner>(*this);
    auto ui_thread_runner = m_uiThreadRunner;

    m_showThread = std::make_unique<std::thread>([ui_thread_runner]()
    {
        ui_thread_runner->RunOnUIThread();
    });

    return 1;
}


WindowsMapDlg* WindowsMapUI::GetMapDlgForAction()
{
    return ( m_uiThreadRunner != nullptr ) ? m_uiThreadRunner->GetMapDlg() : nullptr;
}


template<typename Action>
void WindowsMapUI::PerformMapDlgAction(Action action)
{
    WindowsMapDlg* map_dlg = GetMapDlgForAction();

    if( map_dlg != nullptr )
        action(*map_dlg);
}


int WindowsMapUI::Hide()
{
    if( m_uiThreadRunner == nullptr )
        return 0;

    // send a message to close the dialog
    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SendMessage(WM_CLOSE);
    });

    // wait for the dialog to fully close
    WaitForShowThreadToTerminate();

    return 1;
}


int WindowsMapUI::SaveSnapshot(const std::wstring& filename)
{
    int result = 0;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SaveSnapshot(filename);
        result = 1;
    });

    return result;
}


int WindowsMapUI::AddMarker(double latitude, double longitude)
{
    const Marker& marker = m_markers.try_emplace(m_nextMapId,
        Marker { latitude, longitude }).first->second;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.AddMarker(marker, m_nextMapId);
        map_dlg.FitPoints();
    });

    return m_nextMapId++;
}


int WindowsMapUI::RemoveMarker(int marker_id)
{
    Marker* marker = GetMarker(marker_id);
    if( marker == nullptr )
        return 0;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.RemoveMarker(marker->leaflet_id);
    });

    m_markers.erase(marker_id);

    return 1;
}


void WindowsMapUI::ClearMarkers()
{
    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.ClearMarkers();
    });

    m_markers.clear();
}


int WindowsMapUI::SetMarkerImage(int marker_id, std::wstring image_file_path)
{
    Marker* marker = GetMarker(marker_id);
    if( marker == nullptr )
        return 0;

    marker->image_url = GetUrlForFilename(image_file_path);

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetMarkerImage(*marker);
    });

    return 1;
}


int WindowsMapUI::SetMarkerText(int marker_id, std::wstring text, int background_color, int text_color)
{
    Marker* marker = GetMarker(marker_id);
    if( marker == nullptr )
        return 0;

    marker->text = std::move(text);
    marker->background_color = PortableColor::FromColorInt(background_color);
    marker->text_color = PortableColor::FromColorInt(text_color);

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetText(*marker);
    });

    return 1;
}


int WindowsMapUI::SetMarkerOnClick(int marker_id, int on_click_callback)
{
    Marker* marker = GetMarker(marker_id);
    if( marker == nullptr )
        return 0;

    marker->on_click_callback = on_click_callback;

    return 1;
}


int WindowsMapUI::SetMarkerOnClickInfoWindow(int marker_id, int on_click_callback)
{
    Marker* marker = GetMarker(marker_id);
    if( marker == nullptr )
        return 0;

    marker->on_info_window_click_callback = on_click_callback;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetDescription(*marker, marker_id);
    });

    return 1;
}


int WindowsMapUI::SetMarkerOnDrag(int marker_id, int on_drag_callback)
{
    Marker* marker = GetMarker(marker_id);
    if( marker == nullptr )
        return 0;

    marker->on_drag_callback = on_drag_callback;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetDraggable(marker->leaflet_id);
    });

    return 1;
}


int WindowsMapUI::SetMarkerDescription(int marker_id, std::wstring description)
{
    Marker* marker = GetMarker(marker_id);
    if( marker == nullptr )
        return 0;

    marker->description = std::move(description);

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetDescription(*marker, marker_id);
    });

    return 1;
}


int WindowsMapUI::SetMarkerLocation(int marker_id, double latitude, double longitude)
{
    Marker* marker = GetMarker(marker_id);
    if( marker == nullptr )
        return 0;

    marker->latitude = latitude;
    marker->longitude = longitude;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.MoveMarker(*marker);
        map_dlg.FitPoints();
    });

    return 1;
}


int WindowsMapUI::GetMarkerLocation(int marker_id, double& latitude, double& longitude)
{
    Marker* marker = GetMarker(marker_id);
    if( marker == nullptr )
        return 0;

    latitude = marker->latitude;
    longitude = marker->longitude;

    return 1;
}


int WindowsMapUI::AddImageButton(std::wstring image_path, int on_click_callback)
{
    const Button& button = m_buttons.try_emplace(m_nextMapId,
        Button { ButtonType::Image, on_click_callback, GetUrlForFilename(image_path) }).first->second;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.AddImageButton(button, m_nextMapId);
    });

    return m_nextMapId++;
}


int WindowsMapUI::AddTextButton(std::wstring label, int on_click_callback)
{
    const Button& button = m_buttons.try_emplace(m_nextMapId,
        Button { ButtonType::Text, on_click_callback, std::move(label) }).first->second;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.AddTextButton(button, m_nextMapId);
    });

    return m_nextMapId++;
}


int WindowsMapUI::RemoveButton(int button_id)
{
    Button* button = GetButton(button_id);
    if( button == nullptr )
        return 0;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.RemoveButton(button_id);
    });

    m_buttons.erase(button_id);

    return 1;
}


void WindowsMapUI::ClearButtons()
{
    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.ClearButtons();
    });

    m_buttons.clear();
}


void WindowsMapUI::Clear()
{
    m_title.clear();
    m_baseMapSelection.reset();
    m_zoom.reset();
    m_showCurrentLocation = true;

    ClearButtons();
    ClearMarkers();
    ClearGeometry();

    m_tileReader.reset();
    m_tileProvider.reset();
}


bool WindowsMapUI::IsBaseMapDefined() const
{
    return m_baseMapSelection.has_value();
}


int WindowsMapUI::SetBaseMap(const BaseMapSelection& base_map_selection)
{
    if( std::holds_alternative<BaseMap>(base_map_selection) )
    {
        m_tileReader.reset();
        m_tileProvider.reset();
    }

    else
    {
        // open the mbtiles or tpk file
        const std::wstring& filename = std::get<std::wstring>(base_map_selection);
        std::wstring extension = PortableFunctions::PathGetFileExtension(filename);

        if( SO::EqualsNoCase(extension, _T("mbtiles")) )
        {
            m_tileReader = std::make_shared<MBTilesReader>(filename);
        }

        else if( SO::EqualsNoCase(extension, _T("tpk")) )
        {
            m_tileReader = std::make_shared<TPKReader>(filename);
        }

        else
        {
            throw CSProException(_T("unknown base map file with extension \"%s\""), extension.c_str());
        }

        m_tileProvider = std::make_unique<OfflineTileProvider>(m_tileReader);
    }

    m_baseMapSelection = base_map_selection;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetupBaseMap();
    });

    return 1;
}


int WindowsMapUI::SetShowCurrentLocation(bool show)
{
    m_showCurrentLocation = show;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetupCurrentLocation();
    });

    return 1;
}


int WindowsMapUI::SetTitle(std::wstring title)
{
    m_title = std::move(title);

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetTitle(m_title);
    });

    return 1;
}


namespace
{
    constexpr bool AreCoordinatesValid(double latitude, double longitude)
    {
        return ( latitude >= -90 && latitude <= 90 &&
                 longitude >= -180 && longitude <= 180 );
    }
}


int WindowsMapUI::ZoomTo(double latitude, double longitude, double zoom/* = -1*/)
{
    if( !AreCoordinatesValid(latitude, longitude) )
        return 0;

    m_zoom = Zoom { latitude, longitude, -91, -181, zoom };

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetZoom(latitude, longitude, zoom);
    });

    return 1;
}


int WindowsMapUI::ZoomTo(double minLat, double minLong, double maxLat, double maxLong, double paddingPercent/* = 0*/)
{
    if( !AreCoordinatesValid(minLat, minLong) || !AreCoordinatesValid(maxLat, maxLong) )
        return 0;

    m_zoom = Zoom { minLat, minLong, maxLat, maxLong, paddingPercent };

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.SetZoom(minLat, maxLat, minLong, maxLong, paddingPercent);
    });

    return 1;
}


int WindowsMapUI::SetCamera(const MapCamera& camera)
{
    return ZoomTo(camera.latitude_, camera.longitude_, camera.zoom_);
}


int WindowsMapUI::AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds)
{
    ASSERT(geometry != nullptr && bounds != nullptr);

    const MapGeometry& map_geometry = m_geometries.try_emplace(m_nextMapId,
        MapGeometry { std::move(geometry), -1 }).first->second;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.AddGeometry(map_geometry, m_nextMapId);
    });

    return m_nextMapId++;
}


int WindowsMapUI::RemoveGeometry(int geometry_id)
{
    MapGeometry* geometry = GetGeometry(geometry_id);
    if( geometry == nullptr )
        return 0;

    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.RemoveGeometry(geometry->leaflet_id);
    });

    m_geometries.erase(geometry_id);

    return 1;
}


void WindowsMapUI::ClearGeometry()
{
    PerformMapDlgAction([&](WindowsMapDlg& map_dlg)
    {
        map_dlg.ClearGeometry();
    });

    m_geometries.clear();
}


IMapUI::MapEvent WindowsMapUI::WaitForEvent()
{
    std::unique_ptr<MapEvent> received_map_event;

    // wait for an event
    while( m_mapEvent == nullptr )
        Sleep(5);

    // lock guard
    {
        std::lock_guard<std::mutex> lock(m_mapEventMutex);
        received_map_event = std::move(m_mapEvent);
    }

    // if the dialog is closing, wait for the show thread to terminate
    // before returning the event to the engine
    if( received_map_event->code_ == IMapUI::EventCode::MapClosed )
        WaitForShowThreadToTerminate();

    return *received_map_event;
}


void WindowsMapUI::NotifyEvent(EventCode code, int marker_id/* = -1*/, int callback_id/* = -1*/,
        double latitude/* = 0*/, double longitude/* = 0*/, const MapCamera& camera/* = MapCamera { 0, 0, 0, 0 }*/)
{
    // wait until any existing events have been processed by the engine
    while( m_mapEvent != nullptr )
        Sleep(5);

    std::lock_guard<std::mutex> lock(m_mapEventMutex);

    m_mapEvent = std::make_unique<MapEvent>(MapEvent
    {
        code,
        marker_id,
        callback_id,
        latitude,
        longitude,
        camera
    });
}


void WindowsMapUI::EnsureFileServerIsSetup()
{
    if( m_fileServer == nullptr )
        m_fileServer = std::make_unique<SharedHtmlLocalFileServer>(_T("mapping"));
}


std::wstring WindowsMapUI::GetUrlOfMapHtml() const
{
    ASSERT(m_fileServer != nullptr);
    return m_fileServer->GetProjectUrl(_T("logic-map.html"));
}


std::wstring WindowsMapUI::GetUrlForFilename(NullTerminatedStringView filename)
{
    EnsureFileServerIsSetup();
    return m_fileServer->GetFilenameUrl(filename);
}
