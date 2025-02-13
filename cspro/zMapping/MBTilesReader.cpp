#include "stdafx.h"
#include "MBTilesReader.h"
#include <SQLite/SQLite.h>
#include <SQLite/SQLiteHelpers.h>


MBTilesReader::MBTilesReader(NullTerminatedString filename)
    :   m_db(nullptr),
        m_stmtTileQuery(nullptr)
{
    try
    {
        // open the database
        if( sqlite3_open_v2(ToUtf8(filename), &m_db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK )
            throw CSProException("database could not be opened");

        // read the metadata
        ReadMetadata();

        // prepare the tile reading statement
        const char* sql = "SELECT `tile_data` "
                          "FROM `tiles` "
                          "WHERE `zoom_level` = ? AND `tile_column` = ? AND `tile_row` = ? "
                          "LIMIT 1;";

        if( sqlite3_prepare_v2(m_db, sql, -1, &m_stmtTileQuery, nullptr) != SQLITE_OK )
            throw CSProException("tiles could not be read");
    }

    catch( const CSProException& exception )
    {
        CloseDatabase();

        throw CSProException(_T("Could not read the MBTiles file %s (%s)"),
                             PortableFunctions::PathGetFilename(filename), exception.GetErrorMessage().c_str());
    }
}


MBTilesReader::~MBTilesReader()
{
    CloseDatabase();
}


void MBTilesReader::CloseDatabase()
{
    if( m_db != nullptr )
    {
        safe_sqlite3_finalize(m_stmtTileQuery);

        sqlite3_close(m_db);
        m_db = nullptr;
    }
}


void MBTilesReader::ReadMetadata()
{
    std::map<std::string, std::string> metadata;
    sqlite3_stmt* stmt_metadata_query;

    if( sqlite3_prepare_v2(m_db, "SELECT `name`, `value` FROM `metadata`;", -1, &stmt_metadata_query, nullptr) != SQLITE_OK )
        throw CSProException("metadata could not be read");

    while( sqlite3_step(stmt_metadata_query) == SQLITE_ROW )
        metadata[(const char*)sqlite3_column_text(stmt_metadata_query, 0)] = (const char*)sqlite3_column_text(stmt_metadata_query, 1);

    sqlite3_finalize(stmt_metadata_query);

    // parse the metadata
    auto get_value = [&](const char* name) -> std::optional<std::wstring>
    {
        const auto& name_search = metadata.find(name);

        if( name_search != metadata.cend() )
            return UTF8Convert::UTF8ToWide(name_search->second);

        return std::nullopt;
    };

    auto get_optional_int_value = [&](const char* name) -> std::optional<int>
    {
        std::optional<std::wstring> value = get_value(name);

        if( value.has_value() && CIMSAString::IsNumeric(*value) )
            return static_cast<int>(CIMSAString::Val(*value));

        return std::nullopt;
    };

    std::wstring format = get_value("format").value_or(_T("png"));
    m_tileMimeType = ValueOrDefault(MimeType::GetTypeFromFileExtension(format));

    if( !MimeType::IsImageType(m_tileMimeType) )
        throw CSProException(_T("tile type \"%s\" not supported"), format.c_str());

    m_minNativeZoom = get_optional_int_value("minzoom");
    m_maxNativeZoom = get_optional_int_value("maxzoom");

    std::optional<std::wstring> bounds_value = get_value("bounds");

    if( bounds_value.has_value() )
    {
        std::vector<double> values;

        for( wstring_view value_text : SO::SplitString<wstring_view>(*bounds_value, ',') )
        {
            if( CIMSAString::IsNumeric(value_text) )
                values.emplace_back(CIMSAString::fVal(value_text));
        }

        // "The bounds are represented as WGS 84 latitude and longitude values, in the
        //  OpenLayers Bounds format (left, bottom, right, top)."
        if( values.size() == 4 )
        {
            m_bounds.emplace(Bounds { { values[1], values[0] },
                                      { values[3], values[2] } });
        }
    }

    m_attribution = get_value("attribution");
}


std::unique_ptr<std::vector<std::byte>> MBTilesReader::GetTile(int z, int x, int y)
{
    // only allow one query to the database at any time
    std::lock_guard lock_guard(m_tileQueryMutex);

    std::unique_ptr<std::vector<std::byte>> tile;

    // "Note that in the TMS tiling scheme, the Y axis is reversed from the "XYZ" coordinate system
    //  commonly used in the URLs to request individual tiles, so the tile commonly referred to as 11/327/791
    //  is inserted as zoom_level 11, tile_column 327, and tile_row 1256, since 1256 is 2^11 - 1 - 791."
    y = ( 1 << z ) - y - 1;

    sqlite3_reset(m_stmtTileQuery);
    sqlite3_bind_int(m_stmtTileQuery, 1, z);
    sqlite3_bind_int(m_stmtTileQuery, 2, x);
    sqlite3_bind_int(m_stmtTileQuery, 3, y);

    if( sqlite3_step(m_stmtTileQuery) == SQLITE_ROW )
    {
        auto tile_size = sqlite3_column_bytes(m_stmtTileQuery, 0);
        tile = std::make_unique<std::vector<std::byte>>(tile_size);
        memcpy(tile->data(), sqlite3_column_blob(m_stmtTileQuery, 0), tile_size);
    }

    return tile;
}
