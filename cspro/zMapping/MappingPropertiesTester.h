#pragma once

#include <zMapping/zMapping.h>
#include <zAppO/Properties/MappingProperties.h>


class MappingPropertiesTester
{
public:
    ZMAPPING_API static void Test(MappingProperties mapping_properties, std::optional<MappingTileProvider> mapping_tile_provider = std::nullopt);

private:
    class TestMapUI;

    static void SetupMapForTileProvider(TestMapUI& map_ui, const MappingProperties& mapping_properties);
    static void SetupMapForOfflineBaseMap(TestMapUI& map_ui, const std::string& file_path);
};
