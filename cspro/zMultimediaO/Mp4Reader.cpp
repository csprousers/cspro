#include "stdafx.h"
#include "Mp4Reader.h"
#include <mp4v2/mp4v2.h>
#include <zToolsO/Utf8Convert.h>

namespace
{
    MP4TrackId GetFirstAudioTrack(MP4FileHandle handle)
    {
        uint32_t num_tracks = MP4GetNumberOfTracks(handle);
        for (size_t i = 0; i < num_tracks; i++) {
            MP4TrackId track = MP4FindTrackId(handle, (uint16_t)i);
            const char* trackType = MP4GetTrackType(handle, track);
            if (strcmp(trackType, MP4_AUDIO_TRACK_TYPE) == 0)
                return track;
        }
        return MP4_INVALID_TRACK_ID;
    }
}


Mp4Reader::Mp4Reader(const std::wstring& filename)
{
    m_file_handle = MP4Read(UTF8Convert::WideToUTF8(filename).c_str());
    if (m_file_handle == MP4_INVALID_FILE_HANDLE) {
        throw Mp4ReaderError("Not a valid mp4 file");
    }
}


Mp4Reader::~Mp4Reader()
{
    MP4Close(m_file_handle);
}


double Mp4Reader::GetDuration() const
{
    MP4Duration duration = MP4GetDuration(m_file_handle);
    uint64_t ms = MP4ConvertFromMovieDuration(m_file_handle, duration, MP4_MILLISECONDS_TIME_SCALE);
    return ms/1000.0;
}


const char* Mp4Reader::GetAudioFormat() const
{
    MP4TrackId track1 = GetFirstAudioTrack(m_file_handle);
    if (track1 == MP4_INVALID_TRACK_ID)
        return "";

    return MP4GetTrackMediaDataName(m_file_handle, track1);
}


int Mp4Reader::GetAudioTimeScale() const
{
    MP4TrackId track1 = GetFirstAudioTrack(m_file_handle);
    if (track1 == MP4_INVALID_TRACK_ID)
        return 0;
    return MP4GetTrackTimeScale(m_file_handle, track1);
}


int Mp4Reader::GetAudioBitRate() const
{
    MP4TrackId track1 = GetFirstAudioTrack(m_file_handle);
    if (track1 == MP4_INVALID_TRACK_ID)
        return 0;
    return MP4GetTrackBitRate(m_file_handle, track1);
}



#ifdef WASM // WASM_TODO: build mp4v2 for Mp4Reader + Mp4Writer

uint64_t MP4ConvertFromMovieDuration(MP4FileHandle hFile, MP4Duration duration, uint32_t timeScale) { throw ProgrammingErrorException(); }
const char* MP4GetTrackType(MP4FileHandle hFile, MP4TrackId trackId)                                { throw ProgrammingErrorException(); }
uint32_t MP4GetTrackTimeScale(MP4FileHandle hFile, MP4TrackId trackId)                              { throw ProgrammingErrorException(); }
MP4SampleId MP4GetTrackNumberOfSamples(MP4FileHandle hFile, MP4TrackId trackId)                     { throw ProgrammingErrorException(); }
const char* MP4GetTrackMediaDataName(MP4FileHandle hFile, MP4TrackId trackId)                       { throw ProgrammingErrorException(); }
MP4Duration MP4GetDuration(MP4FileHandle hFile)                                                     { throw ProgrammingErrorException(); }
bool MP4TagsFetch(const MP4Tags* tags, MP4FileHandle hFile)                                         { throw ProgrammingErrorException(); }
uint32_t MP4GetNumberOfTracks(MP4FileHandle hFile, const char* type, uint8_t subType)               { throw ProgrammingErrorException(); }
MP4TrackId MP4FindTrackId(MP4FileHandle hFile, uint16_t index, const char* type, uint8_t subType)   { throw ProgrammingErrorException(); }
MP4FileHandle MP4Create(const char* fileName, uint32_t flags)                                       { throw ProgrammingErrorException(); }
void MP4Close(MP4FileHandle hFile, uint32_t flags)                                                  { throw ProgrammingErrorException(); }
bool MP4TagsAddArtwork(const MP4Tags*, MP4TagArtwork*)                                              { throw ProgrammingErrorException(); }
bool MP4TagsSetAlbumArtist(const MP4Tags*, const char*)                                             { throw ProgrammingErrorException(); }
bool MP4TagsSetName(const MP4Tags*, const char*)                                                    { throw ProgrammingErrorException(); }
bool MP4TagsSetAlbum(const MP4Tags*, const char*)                                                   { throw ProgrammingErrorException(); }
bool MP4TagsSetEncodingTool(const MP4Tags*, const char*)                                            { throw ProgrammingErrorException(); }
bool MP4TagsStore(const MP4Tags* tags, MP4FileHandle hFile)                                         { throw ProgrammingErrorException(); }
void MP4TagsFree(const MP4Tags* tags)                                                               { throw ProgrammingErrorException(); }
const MP4Tags* MP4TagsAlloc()                                                                       { throw ProgrammingErrorException(); }
MP4FileHandle MP4Read(const char* fileName)                                                         { throw ProgrammingErrorException(); }
MP4FileHandle MP4Modify(const char* fileName, uint32_t flags)                                       { throw ProgrammingErrorException(); }
MP4TrackId MP4CopyTrack(MP4FileHandle srcFile, MP4TrackId srcTrackId, MP4FileHandle dstFile,
                        bool applyEdits, MP4TrackId dstHintTrackReferenceTrack)                     { throw ProgrammingErrorException(); }
bool MP4CopySample(MP4FileHandle srcFile, MP4TrackId srcTrackId, MP4SampleId srcSampleId,
                   MP4FileHandle dstFile, MP4TrackId dstTrackId, MP4Duration dstSampleDuration)     { throw ProgrammingErrorException(); }

#endif
