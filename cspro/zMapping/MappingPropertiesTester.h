#pragma once

#include <zMapping/zMapping.h>
#include <zAppO/Properties/MappingProperties.h>


ZMAPPING_API void TestMappingProperties(MappingProperties mapping_properties,
    std::optional<MappingTileProvider> mapping_tile_provider = std::nullopt);
