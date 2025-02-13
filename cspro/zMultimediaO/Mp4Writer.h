#pragma once

#include <zMultimediaO/zMultimediaO.h>


CREATE_CSPRO_EXCEPTION(Mp4WriterError)


struct Mp4Metadata
{
    std::wstring name;
    std::wstring artist;
    std::wstring album;
    std::wstring artwork_image_path;
};


class ZMULTIMEDIAO_API Mp4Writer
{
public:
    Mp4Writer(const std::wstring& filename, bool create_new);
    ~Mp4Writer();

    void AppendAudioTracks(const std::wstring& other_file);

    void SetTags(const Mp4Metadata& metadata);

private:
    using MP4FileHandle = void*;
    MP4FileHandle m_file_handle;
};
