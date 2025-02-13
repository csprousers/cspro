#include "stdafx.h"
#include "TPKReader.h"
#include <external/pugixml/pugixml.hpp>


TPKReader::TPKReader(NullTerminatedString filename)
    :   m_zipReader(filename)
{
    ReadConfXml();
    ReadBounds();
}


void TPKReader::ReadConfXml()
{
    try
    {
        std::optional<std::wstring> conf_xml_filename = m_zipReader.FindPathInZip(_T("conf.xml"));

        if( !conf_xml_filename.has_value() )
            throw std::exception();

        std::unique_ptr<std::vector<std::byte>> conf_xml_contents = m_zipReader.Read(*conf_xml_filename);
        pugi::xml_document conf_xml_doc;

        if( !conf_xml_doc.load_string(reinterpret_cast<const char*>(conf_xml_contents->data())) )
            throw std::exception();

        // parse the file with minimal error checking
        pugi::xpath_node tile_cache_info_node = conf_xml_doc.select_node("/CacheInfo/TileCacheInfo");
        m_tileWidth = atoi(tile_cache_info_node.node().child_value("TileCols"));
        m_tileHeight = atoi(tile_cache_info_node.node().child_value("TileRows"));

        if( m_tileWidth != m_tileHeight )
            throw CSProException("Tile packages without equal widths and heights are not supported.");


        pugi::xpath_node cache_tile_format_node = conf_xml_doc.select_node("/CacheInfo/TileImageInfo/CacheTileFormat");
        std::wstring tile_format = UTF8Convert::UTF8ToWide(cache_tile_format_node.node().child_value());

        // modify formats like PNG8 to simply be PNG
        if( SO::StartsWithNoCase(tile_format, _T("png")) )
            tile_format = _T("png");

        m_tileMimeType = ValueOrDefault(MimeType::GetTypeFromFileExtension(tile_format));

        if( !MimeType::IsImageType(m_tileMimeType) )
            throw CSProException(_T("Tile packages with tile type \"%s\" are not supported."), tile_format.c_str());


        pugi::xpath_node cache_storage_info_node = conf_xml_doc.select_node("/CacheInfo/CacheStorageInfo");
        m_packetSize = atoi(cache_storage_info_node.node().child_value("PacketSize"));
        std::string storage_format = cache_storage_info_node.node().child_value("StorageFormat");

        if( storage_format == "esriMapCacheStorageModeCompact" )
        {
            m_v1BundleType = true;
        }

        else if( storage_format == "esriMapCacheStorageModeCompactV2" )
        {
            m_v1BundleType = false;
        }

        else
        {
            throw CSProException(_T("Tile packages with storage format \"%s\" are not supported."), UTF8Convert::UTF8ToWide(storage_format).c_str());
        }


        // load the layer details
        std::map<int, std::wstring> all_level_bundle_paths = GetLevelBundlePaths(*conf_xml_filename);

        for( const auto& lod_info_node : conf_xml_doc.select_nodes("/CacheInfo/TileCacheInfo/LODInfos/*") )
        {
            int level_id = atoi(lod_info_node.node().child_value("LevelID"));

            const auto& all_level_bundle_paths_lookup = all_level_bundle_paths.find(level_id);

            // ignore layers without any tiles
            if( all_level_bundle_paths_lookup  == all_level_bundle_paths.cend() )
                continue;

            double scale = atof(lod_info_node.node().child_value("Scale"));

            std::optional<int> zoom_level = ScaleToZoomLevel(scale);

            if( !zoom_level.has_value() )
            {
                throw CSProException(_T("Invalid scale %f in tiling scheme for LevelID %d. ")
                                     _T("Only scales from ArcGIS Online/Bing Maps/Google Maps scheme are supported."), scale, level_id);
            }

            m_zoomLevelBundlePathMap.try_emplace(*zoom_level, all_level_bundle_paths_lookup->second);
        }

        if( m_zoomLevelBundlePathMap.empty() )
            throw CSProException("Tile package does not contain any valid levels of detail.");
    }

    catch( const CSProException& )
    {
        throw;
    }

    catch( ... )
    {
        throw CSProException("Tile package conf.xml could not be processed.");
    }
}


std::map<int, std::wstring> TPKReader::GetLevelBundlePaths(const std::wstring& conf_xml_filename)
{
    std::map<int, std::wstring> level_bundle_paths;

    std::wstring all_layers_path = PortableFunctions::PathAppendForwardSlashToPath(PortableFunctions::PathGetDirectory(conf_xml_filename), _T("_alllayers/"));
    std::wstring previous_directory;

    m_zipReader.ForeachFile(
        [&](const std::wstring& path_in_zip)
        {
            constexpr bool continue_processing = true;

            if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(path_in_zip), _T("bundle")) )
                return continue_processing;

            std::wstring directory = PortableFunctions::PathGetDirectory(path_in_zip);

            if( directory == previous_directory )
                return continue_processing;

            previous_directory = directory;

            if( !SO::StartsWithNoCase(directory, all_layers_path) )
                return continue_processing;

            directory = PortableFunctions::PathRemoveTrailingSlash(directory);

            std::wstring level_directory_name = PortableFunctions::PathGetFilename(directory);

            if( level_directory_name.length() == 3 && level_directory_name.front() == 'L' )
                level_bundle_paths.try_emplace(_ttoi(level_directory_name.c_str() + 1), directory);

            return continue_processing;
        });

    return level_bundle_paths;
}


std::optional<int> TPKReader::ScaleToZoomLevel(double scale)
{
    struct ScaleZoomLevel { double scale; int zoom_level; };

    static const ScaleZoomLevel ScaleZoomLevels[] =
    {
        { 591657527.591555, 0 },
        { 295828763.795777, 1 },
        { 147914381.897889, 2 },
        { 73957190.948944,  3 },
        { 36978595.474472,  4 },
        { 18489297.737236,  5 },
        { 9244648.868618,   6 },
        { 4622324.434309,   7 },
        { 2311162.217155,   8 },
        { 1155581.108577,   9 },
        { 577790.554289,   10 },
        { 288895.277144,   11 },
        { 144447.638572,   12 },
        { 72223.819286,   13 },
        { 36111.909643,   14 },
        { 18055.954822,   15 },
        { 9027.977411,    16 },
        { 4513.988705,    17 },
        { 2256.994353,    18 },
        { 1128.497176,    19 },
        { 564.248588,     20 },
        { 282.124294,     21 },
        { 141.062147,     22 },
        { 70.531074,      23 },
    };

    // "Since the scale levels are floating point it seems that they get rounded
    //  differently in different tpk files so need to do a fuzzy compare."
    static double EPSILON = 1e-5;

    for( size_t i = 0; i < _countof(ScaleZoomLevels); ++i )
    {
        if( abs(ScaleZoomLevels[i].scale - scale) < EPSILON )
            return ScaleZoomLevels[i].zoom_level;
    }

    return std::nullopt;
}


void TPKReader::ReadBounds()
{
    try
    {
        std::unique_ptr<std::vector<std::byte>> mapserver_json_contents = m_zipReader.Read(_T("servicedescriptions/mapserver/mapserver.json"));
        std::string_view mapserver_json_text((const char*)mapserver_json_contents->data(), mapserver_json_contents->size());

        auto mapserver_json = Json::Parse(mapserver_json_text);
        auto extent_json = mapserver_json["resourceInfo"]["geoInitialExtent"];

        auto get_value = [&](const char* name) { return extent_json[name].Get<double>(); };

        m_bounds.emplace(Bounds { { get_value("ymin"), get_value("xmin") },
                                  { get_value("ymax"), get_value("xmax") } });
    }

    catch( ... ) { } // ignore errors
}


std::unique_ptr<std::vector<std::byte>> TPKReader::GetTile(int z, int x, int y)
{
    // only allow one query to the zip file at any time
    std::lock_guard lock_guard(m_tileQueryMutex);

    int bundle_row = ( y / m_packetSize ) * m_packetSize;
    int bundle_column = ( x / m_packetSize ) * m_packetSize;

    const auto& bundle_paths_lookup = m_zoomLevelBundlePathMap.find(z);

    // only process zoom levels that are defined
    if( bundle_paths_lookup != m_zoomLevelBundlePathMap.cend() )
    {
        std::wstring bundle_path = FormatTextCS2WS(_T("%s/R%04xC%04x.bundle"), bundle_paths_lookup->second.c_str(), bundle_row, bundle_column);

        try
        {
            return m_v1BundleType ? GetTileV1(bundle_path, x, y) :
                                    GetTileV2(bundle_path, x, y);
        }

        catch( ... ) { } // ignore errors
    }

    return nullptr;
}


std::shared_ptr<const std::vector<std::byte>> TPKReader::ReadZipFileAndCache(const std::wstring& path_in_zip)
{
    // keep the most frequently read files in memory rather than constantly rereading them from the zip file
    constexpr size_t FilesToKeepInMemory = 25;

    const auto& file_search = std::find_if(m_cachedZipFiles.cbegin(), m_cachedZipFiles.cend(),
                                           [&](const auto& read_file_zile) { return ( path_in_zip == std::get<0>(read_file_zile) ); });

    if( file_search != m_cachedZipFiles.cend() )
        return std::get<1>(*file_search);

    if( m_cachedZipFiles.size() == FilesToKeepInMemory )
        m_cachedZipFiles.erase(m_cachedZipFiles.begin());

    return std::get<1>(m_cachedZipFiles.emplace_back(path_in_zip, m_zipReader.Read(path_in_zip)));
}


namespace
{
    void ReadData(void* out_data, const std::shared_ptr<const std::vector<std::byte>>& in_data, size_t offset, size_t size)
    {
        if( ( offset + size ) > in_data->size() )
            throw std::exception();

        memcpy(out_data, in_data->data() + offset, size);
    }
}


std::unique_ptr<std::vector<std::byte>> TPKReader::GetTileV1(const std::wstring& bundle_path, int x, int y)
{
    // read the bundle index and data
    ASSERT(!bundle_path.empty());
    std::wstring bundle_index_path = bundle_path;
    bundle_index_path.back() = 'x';

    auto bundle_index_data = ReadZipFileAndCache(bundle_index_path);
    auto bundle_data = ReadZipFileAndCache(bundle_path);

    // find the tile location from the bundle index
    constexpr size_t HeaderSize = 16;
    constexpr size_t TileIndexSize = 5;

    size_t tile_index_offset = HeaderSize + TileIndexSize * ( m_packetSize * ( x % m_packetSize ) + ( y % m_packetSize ) );

    int64_t tile_index = 0;
    static_assert(sizeof(tile_index) == ( TileIndexSize + 3 ));
    ReadData(&tile_index, bundle_index_data, tile_index_offset, TileIndexSize);


    // load the tile image
    constexpr size_t TileSizeSize = 4;

    int tile_size;
    static_assert(sizeof(tile_size) == TileSizeSize);
    ReadData(&tile_size, bundle_data, (size_t)tile_index, TileSizeSize);

    auto tile_data = std::make_unique<std::vector<std::byte>>(tile_size);
    ReadData(tile_data->data(), bundle_data, (size_t)tile_index + TileSizeSize, tile_size);

    return tile_data;
}


std::unique_ptr<std::vector<std::byte>> TPKReader::GetTileV2(const std::wstring& bundle_path, int x, int y)
{
    auto bundle_data = ReadZipFileAndCache(bundle_path);

    // find the tile location
    constexpr size_t HeaderSize = 64;
    constexpr size_t TileIndexSize = 8;

    // V2 appears to reverse the order of row/column from the V1 version
    std::swap(x, y);

    size_t tile_index_offset = HeaderSize + TileIndexSize * ( m_packetSize * ( x % m_packetSize ) + ( y % m_packetSize ) );

    uint64_t tile_index;
    static_assert(sizeof(tile_index) == TileIndexSize);
    ReadData(&tile_index, bundle_data, tile_index_offset, TileIndexSize);

    const uint64_t M = 0x10000000000; // 2 to the power of 40
    uint64_t tile_offset = tile_index % M;
    uint64_t tile_size = tile_index / M;

    if( tile_size == 0 )
        return nullptr;

    // load the tile image
    auto tile_data = std::make_unique<std::vector<std::byte>>((size_t)tile_size);
    ReadData(tile_data->data(), bundle_data, (size_t)tile_offset, (size_t)tile_size);

    return tile_data;
}
