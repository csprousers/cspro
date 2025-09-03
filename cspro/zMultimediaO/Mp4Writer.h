#pragma once

#include <zMultimediaO/zMultimediaO.h>


CREATE_CSPRO_EXCEPTION(Mp4WriterError);


class ZMULTIMEDIAO_API Mp4Writer
{
public:
    Mp4Writer(cs::string_sz file_path, bool create_new);
    ~Mp4Writer();

    void AppendAudioTracks(cs::string_sz other_file_path);

private:
    using MP4FileHandle = void*;
    MP4FileHandle m_file_handle;
};
