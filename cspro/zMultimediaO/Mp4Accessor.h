#pragma once

#include <zMultimediaO/zMultimediaO.h>


// --------------------------------------------------------------------------
// Mp4Accessor
// --------------------------------------------------------------------------

class ZMULTIMEDIAO_API Mp4Accessor
{
public:
    Mp4Accessor();
    ~Mp4Accessor();
};


// --------------------------------------------------------------------------
// Mp4TagSetter
// --------------------------------------------------------------------------

class ZMULTIMEDIAO_API Mp4TagSetter : public Mp4Accessor
{
public:
    Mp4TagSetter();
    ~Mp4TagSetter();

    // Opens an existing ISO base media file for setting tags.
    // If possible, the edits will be made in-place, but if not, the edits will be
    // made in a temporary file and the original file replaced on Close.
    // An CSProException is thrown on error.
    void Open(std::string file_path);

    // Saves any changes.
    // An CSProException is thrown on error.
    void Close();

    // Sets a text-based tag.
    // An CSProException is thrown on error.
    enum class TextTag { AlbumArtist, AlbumName, EncodingTool, TitleName };
    void SetTextTag(TextTag tag_type, std::string_view text_sv);

    // Sets a binary-based tag.
    // An CSProException is thrown on error.
    enum class BinaryTag { CoverArt };
    void SetBinaryTag(BinaryTag tag_type, const std::string& binary_data_file_path);

private:
    void CheckFileIsOpen() const;

    template<typename ErrorT>
    void CheckResult(ErrorT error) const;

private:
    struct Data;
    std::unique_ptr<Data> m_data;
};
