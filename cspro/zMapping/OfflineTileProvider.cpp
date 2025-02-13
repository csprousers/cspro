#include "stdafx.h"
#include "OfflineTileProvider.h"
#include "OfflineTileReader.h"
#include <external/cpp-httplib/httplib.h>


OfflineTileProvider::OfflineTileProvider(std::shared_ptr<OfflineTileReader> offline_tile_reader)
    :   m_offlineTileReader(std::move(offline_tile_reader))
{
    ASSERT(m_offlineTileReader != nullptr);

    std::wstring tile_mime_type = m_offlineTileReader->GetTileMimeType();
    const TCHAR* const tile_extension = MimeType::GetFileExtensionFromType(m_offlineTileReader->GetTileMimeType());
    ASSERT(tile_extension != nullptr);

    m_tileMimeType = UTF8Convert::WideToUTF8(tile_mime_type);
    m_tileExtension = *tile_extension;

    m_server = new httplib::Server();
    m_port = m_server->bind_to_any_port("localhost");

    m_thread = std::thread([&]() { m_server->listen_after_bind(); });

    std::wstring pattern = FormatTextCS2WS(_T("(\\/\\d+\\/\\d+\\/\\d+\\.%s)"), m_tileExtension.c_str());

    m_server->Get(UTF8Convert::WideToUTF8(pattern),
        [&](const httplib::Request& req, httplib::Response& res)
        {
            std::vector<std::wstring> matches = SO::SplitString(UTF8Convert::UTF8ToWide(req.matches[1].str()), _T("/."), true, false);

            if( matches.size() != 4 )
            {
                ASSERT(false);
                return;
            }

            std::unique_ptr<std::vector<std::byte>> tile = m_offlineTileReader->GetTile(_ttoi(matches[0].c_str()),
                                                                                        _ttoi(matches[1].c_str()),
                                                                                        _ttoi(matches[2].c_str()));

            if( tile != nullptr )
                res.set_content(reinterpret_cast<const char*>(tile->data()), tile->size(), m_tileMimeType);
        });
}


OfflineTileProvider::~OfflineTileProvider()
{
    if( m_server->is_running() )
    {
        m_server->stop();
        m_thread.join();
    }

    delete m_server;
}


std::wstring OfflineTileProvider::GetTileLayerUrl() const
{
    return FormatTextCS2WS(_T("http://localhost:%d/{z}/{x}/{y}.%s"), m_port, m_tileExtension.c_str());
}


std::wstring OfflineTileProvider::GetLeafletTileLayerOptions() const
{
    auto json_writer = Json::CreateStringWriter();

    json_writer->BeginObject();

    // add any defined metadata
    ASSERT(m_offlineTileReader->GetTileWidth() == m_offlineTileReader->GetTileHeight());
    json_writer->Write(_T("tileSize"), m_offlineTileReader->GetTileWidth());

    json_writer->WriteIfHasValue(_T("minNativeZoom"), m_offlineTileReader->GetMinNativeZoom());
    json_writer->WriteIfHasValue(_T("maxNativeZoom"), m_offlineTileReader->GetMaxNativeZoom());

    std::optional<OfflineTileReader::Bounds> bounds = m_offlineTileReader->GetBounds();

    if( bounds.has_value() )
    {
        json_writer->BeginArray(_T("bounds"))
                    .Write(std::vector<double>{ std::get<0>(bounds->min), std::get<1>(bounds->min) })
                    .Write(std::vector<double>{ std::get<0>(bounds->max), std::get<1>(bounds->max) })
                    .EndArray();
    }

    json_writer->WriteIfHasValue(_T("attribution"), m_offlineTileReader->GetAttribution());

    json_writer->EndObject();

    return json_writer->GetString();
}
