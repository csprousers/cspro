#pragma once

#include <zMapping/zMapping.h>
#include <zMapping/IMapUI.h>
#include <zToolsO/PointerClasses.h>

class MappingProperties;
class OfflineTileReader;


// --------------------------------------------------------------------------
// HTML-based implementation of mapping.
// --------------------------------------------------------------------------

class ZMAPPING_API HtmlMapUI : public IMapUI
{
    struct Data;
    struct MapGeometry;
    struct Zoom1;
    struct Zoom2;

public:
    HtmlMapUI(cs::non_null_shared_or_raw_ptr<const MappingProperties> mapping_properties);
    ~HtmlMapUI();

    // IMapUI overrides
    void Clear() override;

    bool SetTitle(SharableString title) override;

    bool IsBaseMapDefined() const override;
    bool SetBaseMap(BaseMapSelection base_map_selection) override;

    bool SetShowCurrentLocation(bool show) override;

    bool SetCamera(const MapCamera& camera) override;

    bool ZoomTo(double latitude, double longitude, double zoom = -1) override;
    bool ZoomTo(double min_latitude, double min_longitude, double max_latitude, double max_longitude, double padding_percent = 0) override;

    int AddGeometry(std::shared_ptr<const Geometry::FeatureCollection> geometry, std::shared_ptr<const Geometry::BoundingBox> bounds) override;
    bool RemoveGeometry(int geometry_id) override;
    void ClearGeometry() override;

protected:
    std::string GetUrlOfMapHtml() const;
    std::string GetUrlForUrlOrFile(const std::string& url_or_file_path);

    void PostActionMessage(cs::string_sz action);
    void PostActionMessage(cs::string_sz action, const std::function<void(JsonWriter&)>& callback_function);

    void OnWebMessageReceived(std::string_view message_sv);

    OfflineTileReader* GetOfflineTileReader();

    // the following methods must be overridden by subclasses:
    virtual bool IsMapShowing() = 0;

    // OnPostActionMessage will only be called when the map is showing.
    virtual void OnPostActionMessage(SharableString action_message_json) = 0;

    virtual void OnSetWindowTitle(const std::string& title) = 0;

    // OnShowCurrentLocation should return false if the current location is unknown.
    virtual bool OnShowCurrentLocation() = 0;

private:
    // The following methods, with the suffix IMIS ("if map is showing"), are mostly companion
    // functions to the IMapUI overrides that will only be called when the map is showing.
    void SetUpInitialMapIMIS();

    void SetTitleIMIS();

    void SetBaseMapWorker(std::optional<BaseMapSelection> base_map_selection);
    void SetBaseMapIMIS();

    void SetShowCurrentLocationIMIS();

    constexpr bool AreCoordinatesValid(double latitude, double longitude);

    void ZoomToWorker(std::variant<std::monostate, Zoom1, Zoom2> zoom);
    void ZoomToIMIS();

protected:
    void FitMarkersIMIS();

private:
    MapGeometry* GetGeometry(int geometry_id);
    void AddGeometryIMIS(const MapGeometry& map_geometry);

protected:
    cs::non_null_shared_or_raw_ptr<const MappingProperties> m_mappingProperties;

private:
    std::unique_ptr<Data> m_data;
};
