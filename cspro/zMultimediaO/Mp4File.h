#pragma once

#include <zMultimediaO/zMultimediaO.h>


// --------------------------------------------------------------------------
// Mp4Metadata
// --------------------------------------------------------------------------

struct Mp4Metadata
{
    std::optional<unsigned int> sampling_rate;
    std::optional<double> duration; // in seconds
    std::optional<bool> is_mp4a_format;
};


// --------------------------------------------------------------------------
// Mp4File
//
// This class wraps GPAC functionality for ISO base media files created,
// opened for reading, or opened for in-place editing.
// --------------------------------------------------------------------------

class ZMULTIMEDIAO_API Mp4File
{
public:
    Mp4File() noexcept;

    // On destruction, if the file was created or opened for writing and it
    // has not been saved and closed, the file will be closed without saving changes.
    ~Mp4File() noexcept;

    // Creates a new file or opens an existing ISO base media file.
    // If possible, when opening an existing file for writing, the edits will be made
    // in-place, but if not, the edits will be made in a temporary file and the original
    // file will be replaced on SaveAndClose.
    // A CSProException is thrown on error.
    enum class OpenType { CreateNew, ReadOnlyExisting, ReadWriteExisting };
    void Open(std::string file_path, OpenType open_type);

    // Closes the file without saving any changes.
    // A CSProException is thrown on error.
    void Close();

    // Saves any changes (if applicable) and closes the file.
    // A CSProException is thrown on error.
    void SaveAndClose();

    // Returns the metadata read from the first audio track.
    // A CSProException is thrown only if no file is open.
    // If there are errors reading specific metadata, the fields
    // in the returned object will be std::nullopt.
    Mp4Metadata GetMetadata() const;

    // Sets a text-based tag.
    // A CSProException is thrown on error.
    enum class TextTag { AlbumArtist, AlbumName, EncodingTool, TitleName };
    void SetTextTag(TextTag tag_type, std::string_view text_sv);

    // Sets a binary-based tag.
    // A CSProException is thrown on error.
    enum class BinaryTag { CoverArt };
    void SetBinaryTag(BinaryTag tag_type, const std::string& binary_data_file_path);

    // Appends the audio from the first audio track in the source file.
    // The audio is appended to the first audio track in the destination file,
    // with the track being created if necessary.
    // A CSProException is thrown on error.
    void AppendAudio(std::string source_file_path);

private:
    struct Data;

    void CheckFileIsOpen() const;
    void CheckFileIsOpenForEditing() const;

    template<typename ErrorT>
    static void CheckResult(ErrorT error, const Data* data);

    template<typename ErrorT>
    void CheckResult(ErrorT error) const;

    // Returns the track number of the first audio track, or 0 on error.
    unsigned int GetFirstAudioTrackNumber() const;

    // Returns false if the files do not have compatible codecs for appending.
    template<typename GF_ISOFileT>
    static bool CheckCompatibilityForConcat(GF_ISOFileT* iso_file1, unsigned int track_number1,
                                            GF_ISOFileT* iso_file2, unsigned int track_number2) noexcept;

private:
    std::unique_ptr<Data> m_data;
};
