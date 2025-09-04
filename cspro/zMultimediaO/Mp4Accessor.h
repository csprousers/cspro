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
// Mp4Metadata
// --------------------------------------------------------------------------

struct Mp4Metadata
{
    std::optional<unsigned int> sampling_rate;
    std::optional<double> duration; // duration in seconds
    std::optional<bool> is_mp4a_format;
};


// --------------------------------------------------------------------------
// Mp4MetadataReader
//
// This object returns a Mp4Metadata object with the available metadata read
// from an an existing ISO base media file.
// --------------------------------------------------------------------------

class ZMULTIMEDIAO_API Mp4MetadataReader
{
public:
    // Returns the metadata read from the first audio track.
    // An CSProException is thrown if the file cannot be read or if the file is
    // not a valid MP4 file, but not if there are errors reading specific metadata.
    // In that case, the returned metadata fields will be std::nullopt.
    static Mp4Metadata Read(const std::string& file_path);
};



// --------------------------------------------------------------------------
// Mp4TagSetter
//
// This object allows the modification of tags in an existing ISO base media
// file.
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
