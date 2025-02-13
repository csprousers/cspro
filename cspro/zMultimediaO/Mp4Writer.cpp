#include "stdafx.h"
#include "Mp4Writer.h"
#include "Mp4Reader.h"
#include <zToolsO/FileIO.h>
#include <zToolsO/Utf8Convert.h>
#include <mp4v2/mp4v2.h>
#include <iostream>


namespace
{
    void DumpTracks(MP4FileHandle file)
    {
        uint32_t numTracks = MP4GetNumberOfTracks(file);

        for (uint32_t i = 0; i < numTracks; i++) {
            MP4TrackId trackId = MP4FindTrackId(file, (uint16_t)i);
            const char* trackType = MP4GetTrackType(file, trackId);
            const char* media_data_name = MP4GetTrackMediaDataName(file, trackId);
            uint32_t timeScale = MP4GetTrackTimeScale(file, trackId);
            MP4Duration trackDuration = MP4GetTrackDuration(file, trackId);
            double msDuration = double(MP4ConvertFromTrackDuration(file, trackId,
                trackDuration, MP4_MSECS_TIME_SCALE));
            uint32_t avgBitRate = MP4GetTrackBitRate(file, trackId);

            std::cout << "Track " << trackId << " " << trackType << " " << media_data_name
                << " " << msDuration / 1000.0 << " secs "
                << (avgBitRate + 500) / 1000 << " kbps "
                << timeScale << "Hz" << std::endl;
        }
    }

    std::vector<MP4TrackId> GetAudioTracks(MP4FileHandle handle)
    {
        std::vector<MP4TrackId> audio_tracks;
        uint32_t num_tracks = MP4GetNumberOfTracks(handle);
        for (size_t i = 0; i < num_tracks; i++) {
            MP4TrackId track = MP4FindTrackId(handle, (uint16_t) i);
            const char* trackType = MP4GetTrackType(handle, track);
            if (strcmp(trackType, MP4_AUDIO_TRACK_TYPE) == 0)
                audio_tracks.emplace_back(track);
        }

        return audio_tracks;
    }

    void AppendAudioTrack(MP4FileHandle dest_file, MP4TrackId dest_track_id, MP4FileHandle source_file, MP4TrackId source_track_id)
    {
        const char* source_media_data_name = MP4GetTrackMediaDataName(source_file, source_track_id);
        const char* dest_media_data_name = MP4GetTrackMediaDataName(dest_file, dest_track_id);
        if (strcmp(source_media_data_name, dest_media_data_name) != 0)
            throw Mp4WriterError("Incompatible media type");

        uint32_t source_time_scale = MP4GetTrackTimeScale(source_file, source_track_id);
        uint32_t dest_time_scale = MP4GetTrackTimeScale(dest_file, dest_track_id);
        if (source_time_scale != dest_time_scale)
            throw Mp4WriterError("Incompatible bit rates");

        MP4SampleId sampleId = 0;
        MP4SampleId numSamples = MP4GetTrackNumberOfSamples(source_file, source_track_id);

        while (true) {
            MP4Duration sampleDuration = MP4_INVALID_DURATION;

            sampleId++;
            if (sampleId > numSamples)
                break;

            bool rc = MP4CopySample(
                source_file,
                source_track_id,
                sampleId,
                dest_file,
                dest_track_id,
                sampleDuration);

            if (!rc)
                throw Mp4WriterError("Failed to copy samples from track");
        }
    }
}


Mp4Writer::Mp4Writer(const std::wstring& filename, bool create_new)
{
    if (create_new) {
        m_file_handle = MP4Create(UTF8Convert::WideToUTF8(filename).c_str());
        if (m_file_handle == MP4_INVALID_FILE_HANDLE) {
            throw Mp4WriterError("Failed to create new mp4 file");
        }
    }
    else {
        m_file_handle = MP4Modify(UTF8Convert::WideToUTF8(filename).c_str());
        if (m_file_handle == MP4_INVALID_FILE_HANDLE) {
            throw Mp4WriterError("Failed to open mp4 file for modification");
        }
    }
}


Mp4Writer::~Mp4Writer()
{
    MP4Close(m_file_handle);
}


void Mp4Writer::AppendAudioTracks(const std::wstring& other_file)
{
    Mp4Reader reader(other_file);
    MP4FileHandle other_handle = reader.m_file_handle;

    std::vector<MP4TrackId> existing_tracks = GetAudioTracks(m_file_handle);
    std::vector<MP4TrackId> to_append = GetAudioTracks(other_handle);
    for (size_t i = 0; i < to_append.size(); i++) {
        MP4TrackId source_track = to_append[i];
        if (i >= existing_tracks.size()) {
            MP4CopyTrack(other_handle, source_track, m_file_handle);
        }
        else {
            MP4TrackId dest_track = existing_tracks[i];
            AppendAudioTrack(m_file_handle, dest_track, other_handle, source_track);
        }
    }
}


void Mp4Writer::SetTags(const Mp4Metadata& metadata)
{
    const MP4Tags* mdata = MP4TagsAlloc();
    MP4TagsFetch(mdata, m_file_handle);

    MP4TagArtwork artwork;

    if( !metadata.artwork_image_path.empty() )
    {
        try
        {
            std::unique_ptr<std::vector<std::byte>> artwork_image = FileIO::Read(metadata.artwork_image_path);

            ASSERT(PortableFunctions::PathGetFileExtension(metadata.artwork_image_path) == _T("png"));
            artwork.type = MP4_ART_PNG;

            artwork.size = artwork_image->size();
            artwork.data = artwork_image->data();

            MP4TagsAddArtwork(mdata, &artwork);
        }
        catch(...) { ASSERT(false); }
    }

    MP4TagsSetAlbumArtist(mdata, UTF8Convert::WideToUTF8(metadata.artist).c_str());
    MP4TagsSetName(mdata, UTF8Convert::WideToUTF8(metadata.name).c_str());
    MP4TagsSetAlbum(mdata, UTF8Convert::WideToUTF8(metadata.album).c_str());
    MP4TagsSetEncodingTool(mdata, "CSPro");
    MP4TagsStore(mdata, m_file_handle);
    MP4TagsFree(mdata);
}
