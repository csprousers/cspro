#include "stdafx.h"
#include "Mp4Reader.h"
#include <mp4v2/mp4v2.h>


namespace
{
    MP4TrackId GetFirstAudioTrack(MP4FileHandle handle)
    {
        const uint16_t num_tracks = static_cast<uint16_t>(MP4GetNumberOfTracks(handle));

        for( uint16_t i = 0; i < num_tracks; ++i )
        {
            MP4TrackId track = MP4FindTrackId(handle, i);
            const char* const trackType = MP4GetTrackType(handle, track);

            if( strcmp(trackType, MP4_AUDIO_TRACK_TYPE) == 0 )
                return track;
        }

        return MP4_INVALID_TRACK_ID;
    }
}


Mp4Reader::Mp4Reader(const cs::string_sz file_path)
{
    m_file_handle = MP4Read(file_path.c_str());

    if( m_file_handle == MP4_INVALID_FILE_HANDLE )
        throw Mp4ReaderError("Not a valid mp4 file");
}


Mp4Reader::~Mp4Reader()
{
    MP4Close(m_file_handle);
}



#ifdef WASM // WASM_TODO: build mp4v2 for Mp4Reader + Mp4Writer

const char* MP4GetTrackType(MP4FileHandle hFile, MP4TrackId trackId)                                { throw ProgrammingErrorException(); }
uint32_t MP4GetTrackTimeScale(MP4FileHandle hFile, MP4TrackId trackId)                              { throw ProgrammingErrorException(); }
MP4SampleId MP4GetTrackNumberOfSamples(MP4FileHandle hFile, MP4TrackId trackId)                     { throw ProgrammingErrorException(); }
const char* MP4GetTrackMediaDataName(MP4FileHandle hFile, MP4TrackId trackId)                       { throw ProgrammingErrorException(); }
bool MP4TagsFetch(const MP4Tags* tags, MP4FileHandle hFile)                                         { throw ProgrammingErrorException(); }
uint32_t MP4GetNumberOfTracks(MP4FileHandle hFile, const char* type, uint8_t subType)               { throw ProgrammingErrorException(); }
MP4TrackId MP4FindTrackId(MP4FileHandle hFile, uint16_t index, const char* type, uint8_t subType)   { throw ProgrammingErrorException(); }
MP4FileHandle MP4Create(const char* fileName, uint32_t flags)                                       { throw ProgrammingErrorException(); }
void MP4Close(MP4FileHandle hFile, uint32_t flags)                                                  { throw ProgrammingErrorException(); }
MP4FileHandle MP4Read(const char* fileName)                                                         { throw ProgrammingErrorException(); }
MP4FileHandle MP4Modify(const char* fileName, uint32_t flags)                                       { throw ProgrammingErrorException(); }
MP4TrackId MP4CopyTrack(MP4FileHandle srcFile, MP4TrackId srcTrackId, MP4FileHandle dstFile,
                        bool applyEdits, MP4TrackId dstHintTrackReferenceTrack)                     { throw ProgrammingErrorException(); }
bool MP4CopySample(MP4FileHandle srcFile, MP4TrackId srcTrackId, MP4SampleId srcSampleId,
                   MP4FileHandle dstFile, MP4TrackId dstTrackId, MP4Duration dstSampleDuration)     { throw ProgrammingErrorException(); }

#endif
