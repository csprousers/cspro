#include "stdafx.h"
#include "TPKReader.h"
#include <zXml/SimpleXml.h>
#include <zZip/ZipFile.h>


// --------------------------------------------------------------------------
// TPKReader
// --------------------------------------------------------------------------

TPKReader::TPKReader(const std::string& file_path)
    :   m_zipReader(std::make_unique<ZipReader>(file_path))
{
    ReadMetadata(file_path);
}


TPKReader::~TPKReader()
{
}


std::optional<OfflineTileReader::Tile> TPKReader::GetTile(const int z, const int x, const int y)
{
    // only allow one query to the zip file at any time
    std::lock_guard lock_guard(m_tileQueryMutex);

    const int bundle_row = ( y / m_packetSize ) * m_packetSize;
    const int bundle_column = ( x / m_packetSize ) * m_packetSize;

    const auto& bundle_paths_lookup = m_zoomLevelBundlePathMap.find(z);

    // only process zoom levels that are defined
    if( bundle_paths_lookup != m_zoomLevelBundlePathMap.cend() )
    {
        const std::string bundle_path = FormatText("%s/R%04xC%04x.bundle", bundle_paths_lookup->second.c_str(), bundle_row, bundle_column);

        try
        {
            std::optional<BinaryBlock> tile_data = m_v1BundleType ? GetTileV1(bundle_path, x, y) :
                                                                    GetTileV2(bundle_path, x, y);

            if( tile_data.has_value() )
            {
                ASSERT(!tile_data->empty());

                ASSERT(!m_tileMimeTypes.empty());
                size_t mime_type_index = 0;

                // for mixed packages, determine what kind of image this is
                if( m_tileMimeTypes.size() > 1 )
                {
                    constexpr unsigned char PngSignature[] = { 137, 80, 78, 71, 13, 10, 26, 10 };
                    constexpr size_t PngIndex = 1;

                    ASSERT(m_tileMimeTypes.size() == 2 && m_tileMimeTypes[PngIndex].GetString() == MimeType::GetType(ImageType::Png));

                    if( tile_data->size() >= _countof(PngSignature) &&
                        memcmp(tile_data->data<unsigned char>(), PngSignature, _countof(PngSignature)) == 0 )
                    {
                        mime_type_index = PngIndex;
                    }
                }

                return Tile { std::move(*tile_data), m_tileMimeTypes[mime_type_index] };
            }
        }
        catch(...) { } // ignore errors
    }

    return std::nullopt;
}


std::shared_ptr<const BinaryBlock> TPKReader::ReadZipFileAndCache(const std::string& path_in_zip)
{
    // keep the most frequently read files in memory rather than constantly rereading them from the zip file
    constexpr size_t FilesToKeepInMemory = 25;

    const auto& file_search = std::find_if(m_cachedZipFiles.cbegin(), m_cachedZipFiles.cend(),
                                           [&](const auto& read_file_zile) { return ( path_in_zip == std::get<0>(read_file_zile) ); });

    if( file_search != m_cachedZipFiles.cend() )
        return std::get<1>(*file_search);

    if( m_cachedZipFiles.size() == FilesToKeepInMemory )
        m_cachedZipFiles.erase(m_cachedZipFiles.begin());

    return std::get<1>(m_cachedZipFiles.emplace_back(path_in_zip, std::make_unique<BinaryBlock>(m_zipReader->Read(path_in_zip))));
}


void TPKReader::ReadData(void* const out_data, const BinaryBlock& in_data, const size_t offset, const size_t size)
{
    if( ( offset + size ) > in_data.size() )
        throw std::exception();

    memcpy(out_data, in_data.data() + offset, size);
}


std::optional<BinaryBlock> TPKReader::GetTileV1(const std::string& bundle_path, const int x, const int y)
{
    // read the bundle index and data
    ASSERT(!bundle_path.empty());
    std::string bundle_index_path = bundle_path;
    bundle_index_path.back() = 'x';

    const std::shared_ptr<const BinaryBlock> bundle_index_data = ReadZipFileAndCache(bundle_index_path);
    const std::shared_ptr<const BinaryBlock> bundle_data = ReadZipFileAndCache(bundle_path);

    // find the tile location from the bundle index
    constexpr size_t HeaderSize = 16;
    constexpr size_t TileIndexSize = 5;

    const size_t tile_index_offset = HeaderSize + TileIndexSize * ( m_packetSize * ( x % m_packetSize ) + ( y % m_packetSize ) );

    int64_t tile_index = 0;
    static_assert(sizeof(tile_index) == ( TileIndexSize + 3 ));
    ReadData(&tile_index, *bundle_index_data, tile_index_offset, TileIndexSize);

    constexpr size_t TileSizeSize = 4;

    int tile_size;
    static_assert(sizeof(tile_size) == TileSizeSize);
    ReadData(&tile_size, *bundle_data, static_cast<size_t>(tile_index), TileSizeSize);

    std::optional<BinaryBlock> tile_data;

    // load the tile image
    if( tile_size > 0 )
    {
        tile_data.emplace(tile_size);
        ReadData(tile_data->data(), *bundle_data, static_cast<size_t>(tile_index) + TileSizeSize, tile_size);
    }

    return tile_data;
}


std::optional<BinaryBlock> TPKReader::GetTileV2(const std::string& bundle_path, int x, int y)
{
    const std::shared_ptr<const BinaryBlock> bundle_data = ReadZipFileAndCache(bundle_path);

    // find the tile location
    constexpr size_t HeaderSize = 64;
    constexpr size_t TileIndexSize = 8;

    // V2 appears to reverse the order of row/column from the V1 version
    std::swap(x, y);

    const size_t tile_index_offset = HeaderSize + TileIndexSize * ( m_packetSize * ( x % m_packetSize ) + ( y % m_packetSize ) );

    uint64_t tile_index;
    static_assert(sizeof(tile_index) == TileIndexSize);
    ReadData(&tile_index, *bundle_data, tile_index_offset, TileIndexSize);

    constexpr uint64_t M = 0x10000000000; // 2 to the power of 40
    const size_t tile_offset = static_cast<size_t>(tile_index % M);
    const size_t tile_size = static_cast<size_t>(tile_index / M);

    std::optional<BinaryBlock> tile_data;

    // load the tile image
    if( tile_size != 0 )
    {
        tile_data.emplace(tile_size);
        ReadData(tile_data->data(), *bundle_data, tile_offset, tile_size);
    }

    return tile_data;
}



// --------------------------------------------------------------------------
// MetadataReader
// --------------------------------------------------------------------------

class TPKReader::MetadataReader
{
public:
    MetadataReader(ZipReader& zip_reader);
    virtual ~MetadataReader() { }

    virtual std::tuple<int, int> GetTileWidthAndHeight() = 0;
    virtual std::string GetTileFormat() = 0;
    virtual int GetPacketSize() = 0;
    virtual std::string GetStorageFormat() = 0;

    virtual std::string GetLevelBundlesDirectory() = 0;
    std::map<int, std::string> GetLevelBundlePaths();
    virtual void ForeachLevelOfDetail(const std::function<void(int, double)>& callback_function) = 0;

    static std::optional<int> ScaleToZoomLevel(double scale);

    virtual std::tuple<Bounds, Bounds> GetBounds() = 0;

protected:
    static Bounds ParseBounds(const JsonNode& extent_json_node);

protected:
    ZipReader& m_zipReader;
};


TPKReader::MetadataReader::MetadataReader(ZipReader& zip_reader)
    :   m_zipReader(zip_reader)
{
}


std::map<int, std::string> TPKReader::MetadataReader::GetLevelBundlePaths()
{
    std::map<int, std::string> level_bundle_paths;

    const std::string level_bundles_directory = GetLevelBundlesDirectory();
    std::string previous_directory;

    m_zipReader.ForeachFilePath(
        [&](const char* const path_in_zip)
        {
            constexpr bool continue_processing = true;

            const std::string_view path_in_zip_sv = path_in_zip;

            if( !SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(path_in_zip_sv), "bundle") )
                return continue_processing;

            std::string directory = PortableFunctions::PathGetDirectory(path_in_zip_sv);

            if( directory == previous_directory )
                return continue_processing;

            previous_directory = directory;

            if( !SO::StartsWithNoCase(directory, level_bundles_directory) )
                return continue_processing;

            directory = PortableFunctions::PathRemoveTrailingSlash(directory);

            const std::string level_directory_name = PortableFunctions::PathGetFilename(directory);

            if( level_directory_name.length() == 3 && level_directory_name.front() == 'L' )
                level_bundle_paths.try_emplace(atoi(level_directory_name.c_str() + 1), directory);

            return continue_processing;
        });

    return level_bundle_paths;
}


std::optional<int> TPKReader::MetadataReader::ScaleToZoomLevel(const double scale)
{
    struct ScaleZoomLevel { double scale; int zoom_level; };

    constexpr ScaleZoomLevel ScaleZoomLevels[] =
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
    constexpr double EPSILON = 1e-5;

    for( size_t i = 0; i < _countof(ScaleZoomLevels); ++i )
    {
        if( abs(ScaleZoomLevels[i].scale - scale) < EPSILON )
            return ScaleZoomLevels[i].zoom_level;
    }

    return std::nullopt;
}


OfflineTileReader::Bounds TPKReader::MetadataReader::ParseBounds(const JsonNode& extent_json_node)
{
    return Bounds { { extent_json_node.Get<double>("ymin"), extent_json_node.Get<double>("xmin") },
                    { extent_json_node.Get<double>("ymax"), extent_json_node.Get<double>("xmax") } };
}



// --------------------------------------------------------------------------
// MetadataReader_Tpk
// --------------------------------------------------------------------------

class TPKReader::MetadataReader_Tpk : public TPKReader::MetadataReader
{
public:
    MetadataReader_Tpk(ZipReader& zip_reader);

    std::tuple<int, int> GetTileWidthAndHeight() override;
    std::string GetTileFormat() override;
    int GetPacketSize() override;
    std::string GetStorageFormat() override;
    std::string GetLevelBundlesDirectory() override;
    void ForeachLevelOfDetail(const std::function<void(int, double)>& callback_function) override;
    std::tuple<Bounds, Bounds> GetBounds() override;

private:
    std::optional<std::string> m_confXmlFilePath;
    std::optional<XmlReader> m_confXmlReader;
    std::optional<XmlNode> m_cacheStorageInfoNode;
};


TPKReader::MetadataReader_Tpk::MetadataReader_Tpk(ZipReader& zip_reader)
    :   MetadataReader(zip_reader),
        m_confXmlFilePath(m_zipReader.FindPathInZip("conf.xml"))
{
    if( !m_confXmlFilePath.has_value() )
        throw std::exception();

    const BinaryBlock conf_xml_contents = m_zipReader.Read(*m_confXmlFilePath);
    m_confXmlReader = XmlReader::FromString(conf_xml_contents.data<char>());
}


std::tuple<int, int> TPKReader::MetadataReader_Tpk::GetTileWidthAndHeight()
{
    const XmlNode tile_cache_info_node = m_confXmlReader->SelectNode("/CacheInfo/TileCacheInfo");

    return std::make_tuple(tile_cache_info_node.GetChildValue<int>("TileCols"),
                           tile_cache_info_node.GetChildValue<int>("TileRows"));
}


std::string TPKReader::MetadataReader_Tpk::GetTileFormat()
{
    const XmlNode cache_tile_format_node = m_confXmlReader->SelectNode("/CacheInfo/TileImageInfo/CacheTileFormat");

    return cache_tile_format_node.GetChildValue<std::string>();
}


int TPKReader::MetadataReader_Tpk::GetPacketSize()
{
    ASSERT(!m_cacheStorageInfoNode.has_value());
    m_cacheStorageInfoNode = m_confXmlReader->SelectNode("/CacheInfo/CacheStorageInfo");

    return m_cacheStorageInfoNode->GetChildValue<int>("PacketSize");
}


std::string TPKReader::MetadataReader_Tpk::GetStorageFormat()
{
    ASSERT(m_cacheStorageInfoNode.has_value());

    return m_cacheStorageInfoNode->GetChildValue<std::string>("StorageFormat");
}


std::string TPKReader::MetadataReader_Tpk::GetLevelBundlesDirectory()
{
    return PortableFunctions::PathAppendForwardSlashToPath(PortableFunctions::PathGetDirectory(*m_confXmlFilePath), "_alllayers/");
}


void TPKReader::MetadataReader_Tpk::ForeachLevelOfDetail(const std::function<void(int, double)>& callback_function)
{
    for( const XmlNode& lod_info_node : m_confXmlReader->SelectNodes("/CacheInfo/TileCacheInfo/LODInfos/*") )
    {
        callback_function(lod_info_node.GetChildValue<int>("LevelID"),
                          lod_info_node.GetChildValue<double>("Scale"));
    }
}


std::tuple<OfflineTileReader::Bounds, OfflineTileReader::Bounds> TPKReader::MetadataReader_Tpk::GetBounds()
{
    const BinaryBlock mapserver_json_contents = m_zipReader.Read("servicedescriptions/mapserver/mapserver.json");
    const std::string_view mapserver_json_text_sv = mapserver_json_contents.as<std::string_view>();

    const JsonNode mapserver_json_node = Json::Parse(mapserver_json_text_sv);
    const JsonNode resource_info_json_node = mapserver_json_node.Get("resourceInfo");

    return std::make_tuple(ParseBounds(resource_info_json_node.Get("geoInitialExtent")),
                           ParseBounds(resource_info_json_node.Get("geoFullExtent")));
}



// --------------------------------------------------------------------------
// MetadataReader_Tpkx
// --------------------------------------------------------------------------

class TPKReader::MetadataReader_Tpkx : public TPKReader::MetadataReader
{
public:
    MetadataReader_Tpkx(ZipReader& zip_reader);

    std::tuple<int, int> GetTileWidthAndHeight() override;
    std::string GetTileFormat() override;
    int GetPacketSize() override;
    std::string GetStorageFormat() override;
    std::string GetLevelBundlesDirectory() override;
    void ForeachLevelOfDetail(const std::function<void(int, double)>& callback_function) override;
    std::tuple<Bounds, Bounds> GetBounds() override;

private:
    JsonNode ReadJsonFromZip(const char* path_in_zip) const;

private:
    JsonNode m_rootJsonNode;
    std::optional<JsonNode> m_tileInfoJsonNode;
    std::optional<JsonNode> m_storageInfoJsonNode;
};


TPKReader::MetadataReader_Tpkx::MetadataReader_Tpkx(ZipReader& zip_reader)
    :   MetadataReader(zip_reader),
        m_rootJsonNode(ReadJsonFromZip("root.json"))
{
}


JsonNode TPKReader::MetadataReader_Tpkx::ReadJsonFromZip(const char* const path_in_zip) const
{
    const BinaryBlock json_contents = m_zipReader.Read(path_in_zip);
    return Json::Parse(json_contents.as<std::string_view>());
}


std::tuple<int, int> TPKReader::MetadataReader_Tpkx::GetTileWidthAndHeight()
{
    ASSERT(!m_tileInfoJsonNode.has_value());
    m_tileInfoJsonNode = m_rootJsonNode.Get("tileInfo");

    return std::make_tuple(m_tileInfoJsonNode->Get<int>("rows"),
                           m_tileInfoJsonNode->Get<int>("cols"));
}


std::string TPKReader::MetadataReader_Tpkx::GetTileFormat()
{
    return m_rootJsonNode.Get("tileImageInfo").Get<std::string>("format");
}


int TPKReader::MetadataReader_Tpkx::GetPacketSize()
{
    ASSERT(!m_storageInfoJsonNode.has_value());
    m_storageInfoJsonNode = m_rootJsonNode.Get("storageInfo");

    return m_storageInfoJsonNode->Get<int>("packetSize");
}


std::string TPKReader::MetadataReader_Tpkx::GetStorageFormat()
{
    ASSERT(m_storageInfoJsonNode.has_value());

    return m_storageInfoJsonNode->Get<std::string>("storageFormat");
}


std::string TPKReader::MetadataReader_Tpkx::GetLevelBundlesDirectory()
{
    return "tile";
}


void TPKReader::MetadataReader_Tpkx::ForeachLevelOfDetail(const std::function<void(int, double)>& callback_function)
{
    ASSERT(m_tileInfoJsonNode.has_value());

    for( const JsonNode& lod_json_node : m_tileInfoJsonNode->GetArray("lods") )
    {
        callback_function(lod_json_node.Get<int>("level"),
                          lod_json_node.Get<double>("scale"));
    }
}


std::tuple<OfflineTileReader::Bounds, OfflineTileReader::Bounds> TPKReader::MetadataReader_Tpkx::GetBounds()
{
    const JsonNode item_info_json_node = ReadJsonFromZip("iteminfo.json");

    const Bounds bounds = ParseBounds(item_info_json_node.Get("extent"));
    return std::make_tuple(bounds, bounds);
}



// --------------------------------------------------------------------------
// TPKReader::ReadMetadata
// --------------------------------------------------------------------------

void TPKReader::ReadMetadata(const std::string& file_path)
{
    try
    {
        std::unique_ptr<MetadataReader> metadata_reader;

        if( SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(file_path), "tpkx") )
        {
            metadata_reader = std::make_unique<MetadataReader_Tpkx>(*m_zipReader);
        }

        else
        {
            ASSERT(SO::EqualsNoCase(PortableFunctions::PathGetFileExtension(file_path), "tpk"));
            metadata_reader = std::make_unique<MetadataReader_Tpk>(*m_zipReader);
        }

        // read and validate the metadata with minimal error checking
        std::tie(m_tileWidth, m_tileHeight) = metadata_reader->GetTileWidthAndHeight();

        if( m_tileWidth != m_tileHeight )
            throw CSProException("Tile packages without equal widths and heights are not supported.");

        const std::string tile_format = metadata_reader->GetTileFormat();

        // modify formats like PNG8 to simply be PNG
        if( SO::StartsWithNoCase(tile_format, "png") )
        {
            m_tileMimeTypes.emplace_back(MimeType::GetType(ImageType::Png));
        }

        // mixed packages contain JPEG and PNG images
        else if( SO::EqualsNoCase(tile_format, "MIXED") )
        {
            m_tileMimeTypes.emplace_back(MimeType::GetType(ImageType::Jpeg));
            m_tileMimeTypes.emplace_back(MimeType::GetType(ImageType::Png));
        }

        else
        {
            m_tileMimeTypes.emplace_back(ValueOrDefault(MimeType::GetTypeFromFileExtension(tile_format)));

            if( !MimeType::IsImageType(m_tileMimeTypes.front().GetString()) )
                throw CSProException("Tile packages with tile type '%s' are not supported.", tile_format.c_str());
        }

        m_packetSize = metadata_reader->GetPacketSize();

        const std::string storage_format = metadata_reader->GetStorageFormat();

        m_v1BundleType = ( storage_format == "esriMapCacheStorageModeCompact" )   ? true :
                         ( storage_format == "esriMapCacheStorageModeCompactV2" ) ? false :
                         throw CSProException("Tile packages with storage format '%s' are not supported.", storage_format.c_str());

        // load the layer details
        const std::map<int, std::string> level_bundle_paths = metadata_reader->GetLevelBundlePaths();

        metadata_reader->ForeachLevelOfDetail(
            [&](const int level, double scale)
            {
                const auto& level_bundle_paths_lookup = level_bundle_paths.find(level);

                // ignore layers without any tiles
                if( level_bundle_paths_lookup  == level_bundle_paths.cend() )
                    return;

                const std::optional<int> zoom_level = MetadataReader::ScaleToZoomLevel(scale);

                if( !zoom_level.has_value() )
                {
                    throw CSProException("Invalid scale %f in tiling scheme for level %d. "
                                         "Only scales from ArcGIS Online/Bing Maps/Google Maps scheme are supported.",
                                         scale, level);
                }

                m_zoomLevelBundlePathMap.try_emplace(*zoom_level, level_bundle_paths_lookup->second);
            });

        if( m_zoomLevelBundlePathMap.empty() )
            throw CSProException("Tile package does not contain any valid levels of detail.");

        // read the bounds, ignoring errors
        try
        {
            std::tie(m_initialExtent, m_fullExtent) = metadata_reader->GetBounds();
        }
        catch(...) { }
    }

    catch( const CSProException& )
    {
        throw;
    }

    catch(...)
    {
        throw CSProException("Tile package metadata could not be processed.");
    }
}



// --------------------------------------------------------------------------
// TPKReader::GetMetadataAsJson
// --------------------------------------------------------------------------

template<>
struct JsonSerializer<OfflineTileReader::Bounds>
{
    static void WriteJson(JsonWriter& json_writer, const OfflineTileReader::Bounds& value)
    {
        json_writer.BeginObject()
                   .Write("ymin", std::get<0>(value.min))
                   .Write("xmin", std::get<1>(value.min))
                   .Write("ymax", std::get<0>(value.max))
                   .Write("xmax", std::get<1>(value.max))
                   .EndObject();
    }
};


std::string TPKReader::GetMetadataAsJson() const
{
    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

    json_writer->BeginObject()
                .Write("tileWidth", m_tileWidth)
                .Write("tileHeight", m_tileHeight)
                .Write("tileMimeTypes", m_tileMimeTypes)
                .Write("packetSize", m_packetSize)
                .Write("v1BundleType", m_v1BundleType)
                .WriteMap("zoomLevelBundlePathMap", m_zoomLevelBundlePathMap)
                .WriteIfHasValue("initialExtent", m_initialExtent)
                .WriteIfHasValue("fullExtent", m_fullExtent)
                .EndObject();

    return json_writer->ReleaseString();
}
