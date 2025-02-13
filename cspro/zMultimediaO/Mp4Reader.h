#pragma once

#include <zMultimediaO/zMultimediaO.h>


CREATE_CSPRO_EXCEPTION(Mp4ReaderError)


class ZMULTIMEDIAO_API Mp4Reader
{
    friend class Mp4Writer;

public:
    Mp4Reader(const std::wstring& filename);
    ~Mp4Reader();

    double GetDuration() const;

    const char* GetAudioFormat() const;

    int GetAudioTimeScale() const;

    int GetAudioBitRate() const;

private:
    typedef void* MP4FileHandle;
    MP4FileHandle m_file_handle;
};
