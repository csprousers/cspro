#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/BinarySymbol.h>
#include <zMapping/Geometry.h>

class LogicDocument;


class ZENGINEO_API LogicGeometry : public BinarySymbol
{
private:
    LogicGeometry(const LogicGeometry& logic_geometry);

public:
    LogicGeometry(std::wstring geometry_name);
    LogicGeometry(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor);

    LogicGeometry& operator=(const LogicGeometry& logic_geometry);
    LogicGeometry& operator=(const LogicDocument& logic_document);

    bool HasValidContent(bool parse_data_if_necessary) const noexcept;

    std::shared_ptr<const Geometry::FeatureCollection> GetSharedFeatures() const { return m_features; }
    const Geometry::FeatureCollection& GetFeatures() const                       { return *m_features; }

    const Geometry::Polygon* GetFirstPolygon() const;

    std::shared_ptr<const Geometry::BoundingBox> GetSharedBoundingBox() const { return m_bounds; }
    const Geometry::BoundingBox& GetBoundingBox() const                       { return *m_bounds; }

    double Area() const;
    double Perimeter() const;

    void Load(std::wstring filename);
    void Save(std::wstring filename);

    void SetGeometry(std::shared_ptr<Geometry::FeatureCollection> geometry);
    void SetGeometry(Geometry::Polygon polygon);

    std::wstring GetProperty(wstring_view key_sv) const;
    void SetProperty(wstring_view key_sv, const std::variant<double, std::wstring>& value);

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void WriteValueToJson(JsonWriter& json_writer) const override;
    void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

    // BinarySymbol overrides
    bool HasValidContent() const override;

private:
    static void Save(std::ostream& os, const Geometry::FeatureCollection& features);
    static std::string SaveToString(const Geometry::FeatureCollection& features);

    BinaryData::ContentCallbackType CreateBinaryDataContentFromGeometryCallback() const;

private:
    std::shared_ptr<Geometry::FeatureCollection> m_features;
    std::shared_ptr<Geometry::BoundingBox> m_bounds;
};
