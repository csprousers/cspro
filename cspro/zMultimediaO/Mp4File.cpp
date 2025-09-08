#include "stdafx.h"
#include "Mp4File.h"
#include <zUtilO/Interapp.h>
#include <zUtilO/TemporaryFile.h>
#include <gpac/isomedia.h>


// --------------------------------------------------------------------------
// Mp4File::Data
// --------------------------------------------------------------------------

struct Mp4File::Data
{
    OpenType open_type;
    std::string file_path;
    GF_ISOFile* iso_file;
    std::optional<TemporaryFile> temporary_file_for_non_in_place_edits;
};


// --------------------------------------------------------------------------
// Mp4File
// --------------------------------------------------------------------------

Mp4File::Mp4File() noexcept
{
    // initialize GPAC
    gf_sys_init(GF_MemTrackerNone, nullptr);
}


Mp4File::~Mp4File() noexcept
{
    if( m_data != nullptr )
    {
        ASSERT(m_data->iso_file != nullptr);
        gf_isom_delete(m_data->iso_file);
    }

    // deinitialize GPAC, cleaning up all resources
    gf_sys_close();
}


void Mp4File::CheckFileIsOpen() const
{
    if( m_data == nullptr )
    {
        ASSERT(false);
        throw CSProException("A MP4 file is not open.");
    }
}


void Mp4File::CheckFileIsOpenForEditing() const
{
    CheckFileIsOpen();

    if( m_data->open_type == OpenType::ReadOnlyExisting )
    {
        ASSERT(false);
        throw CSProException("The MP4 file was not opened for editing: %s",
                             Path::GetFilename(m_data->file_path).c_str());
    }
}


template<typename ErrorT>
void Mp4File::CheckResult(const ErrorT error, const Data* const data)
{
    ASSERT(data != nullptr);

    if( error != GF_OK )
    {
        throw CSProException("There was an error interacting with the MP4 file '%s': %s",
                             Path::GetFilename(data->file_path).c_str(),
                             gf_error_to_string(error));
    }
}


template<typename ErrorT>
void Mp4File::CheckResult(const ErrorT error) const
{
    CheckResult(error, m_data.get());
}


void Mp4File::Open(std::string file_path, const OpenType open_type)
{
    if( m_data != nullptr )
    {
        ASSERT(false);
        throw CSProException("A MP4 file is already open: %s", m_data->file_path.c_str());
    }

    std::unique_ptr<Data> data(new Data { open_type, std::move(file_path) });

    // create a new file...
    if( open_type == OpenType::CreateNew )
    {
        data->iso_file = gf_isom_open_GPAC_CSPRO(data->file_path.c_str(), GF_ISOM_WRITE_EDIT, nullptr, nullptr);
    }

    // ...or open a file for reading
    else if( open_type == OpenType::ReadOnlyExisting )
    {
        data->iso_file = gf_isom_open_GPAC_CSPRO(data->file_path.c_str(), GF_ISOM_OPEN_READ, nullptr, nullptr);
    }

    // ...or open a file for editing
    else
    {
        ASSERT(open_type == OpenType::ReadWriteExisting);

        const std::string temp_directory = GetTempDirectory();
        data->temporary_file_for_non_in_place_edits.emplace(TemporaryFile::FromPath(PortableFunctions::GetUniqueFilePathInDirectory(temp_directory, "gpac")));
        ASSERT(!PortableFunctions::FileExists(data->temporary_file_for_non_in_place_edits->GetPath()));

        data->iso_file = gf_isom_open_GPAC_CSPRO(data->file_path.c_str(), GF_ISOM_OPEN_EDIT,
                                                 temp_directory.c_str(),
                                                 data->temporary_file_for_non_in_place_edits->GetPath().c_str());
    }

    if( data->iso_file != nullptr )
    {
        m_data = std::move(data);
    }

    else if( open_type == OpenType::CreateNew )
    {
        throw FileIO::Exception::FileCreateError(data->file_path);
    }

    else if( !PortableFunctions::FileIsRegular(data->file_path) )
    {
        throw FileIO::Exception::FileNotFound(data->file_path);
    }

    else
    {
        throw CSProException("The file is not a valid MP4 file: %s", data->file_path.c_str());
    }
}


void Mp4File::Close()
{
    CheckFileIsOpen();

    // close the file without saving changes
    const std::unique_ptr<const Data> old_data = std::move(m_data);

    gf_isom_delete(old_data->iso_file);
}


void Mp4File::SaveAndClose()
{
    CheckFileIsOpenForEditing();

    // save the changes and close the file
    const std::unique_ptr<Data> old_data = std::move(m_data);

    CheckResult(gf_isom_close(old_data->iso_file), old_data.get());

    // if the changes were not made in-place, replace the existing file with the new one
    if( old_data->temporary_file_for_non_in_place_edits.has_value() &&
        PortableFunctions::FileIsRegular(old_data->temporary_file_for_non_in_place_edits->GetPath()) )
    {
        old_data->temporary_file_for_non_in_place_edits->Rename(old_data->file_path);
    }
}


unsigned int Mp4File::GetFirstAudioTrackNumber() const
{
    CheckFileIsOpen();

    const unsigned int track_count = gf_isom_get_track_count(m_data->iso_file);

    for( unsigned int i = 1; i <= track_count; ++i )
    {
        if( gf_isom_get_media_type(m_data->iso_file, i) == GF_ISOM_MEDIA_AUDIO )
            return i;
    }

    return 0;
}


Mp4Metadata Mp4File::GetMetadata() const
{
    CheckFileIsOpen();

    Mp4Metadata mp4_metadata;

    // read the metadata from the first audio track
    const unsigned int audio_track_number = GetFirstAudioTrackNumber();

    if( audio_track_number != 0 )
    {
        // get the sampling rate from the audio sample description
        unsigned int sample_rate;

        if( gf_isom_get_audio_info(m_data->iso_file, audio_track_number, 1, &sample_rate, nullptr, nullptr) == GF_OK )
            mp4_metadata.sampling_rate = sample_rate;

        // get the duration
        const uint64_t duration = gf_isom_get_media_duration(m_data->iso_file, audio_track_number);
        const unsigned int timescale = ( duration != 0 ) ? gf_isom_get_media_timescale(m_data->iso_file, audio_track_number) : 0;

        if( timescale != 0 )
            mp4_metadata.duration = static_cast<double>(duration) / timescale;

        // check if the subtype is of type MP4A
        const unsigned int subtype = gf_isom_get_mpeg4_subtype(m_data->iso_file, audio_track_number, 1);

        if( subtype != 0 )
            mp4_metadata.is_mp4a_format = ( subtype == GF_4CC('m', 'p', '4', 'a') );
    }

    return mp4_metadata;
}


void Mp4File::SetTextTag(const TextTag tag_type, const std::string_view text_sv)
{
    CheckFileIsOpenForEditing();

    const GF_ISOiTunesTag tag =
        ( tag_type == TextTag::AlbumArtist )  ? GF_ISOM_ITUNE_ALBUM_ARTIST :
        ( tag_type == TextTag::AlbumName )    ? GF_ISOM_ITUNE_ALBUM :
        ( tag_type == TextTag::EncodingTool ) ? GF_ISOM_ITUNE_TOOL :
        ( tag_type == TextTag::TitleName )    ? GF_ISOM_ITUNE_NAME :
                                                throw ProgrammingErrorException();

    CheckResult(gf_isom_apple_set_tag(
        m_data->iso_file,
        tag,
        reinterpret_cast<const u8*>(text_sv.data()),
        text_sv.length(),
        0,
        0
    ));
}


void Mp4File::SetBinaryTag(const BinaryTag tag_type, const std::string& binary_data_file_path)
{
    CheckFileIsOpenForEditing();

    const BinaryBlock binary_data = FileIO::ReadBinary(binary_data_file_path);

    const GF_ISOiTunesTag tag =
        ( tag_type == BinaryTag::CoverArt ) ? GF_ISOM_ITUNE_COVER_ART :
                                              throw ProgrammingErrorException();

    CheckResult(gf_isom_apple_set_tag(
        m_data->iso_file,
        tag,
        binary_data.data<u8>(),
        binary_data.size(),
        0,
        0
    ));
}


template<typename GF_ISOFileT>
bool Mp4File::CheckCompatibilityForConcat(GF_ISOFileT* const iso_file1, unsigned int track_number1,
                                          GF_ISOFileT* const iso_file2, unsigned int track_number2) noexcept
{
    ASSERT(iso_file1 != nullptr && iso_file2 != nullptr);

    // do not allow for different codecs
    if( gf_isom_get_media_subtype(iso_file1, track_number1, 1) !=
        gf_isom_get_media_subtype(iso_file2, track_number2, 1) )
    {
        return false;
    }

    // do not allow different sample descriptions
    const unsigned int sample_description_count = gf_isom_get_sample_description_count(iso_file1, track_number1);

    if( sample_description_count != gf_isom_get_sample_description_count(iso_file2, track_number2) )
        return false;

    for( unsigned int i = 1; i <= sample_description_count; ++i )
    {
        if( !gf_isom_is_same_sample_description(iso_file1, track_number1, i,
                                                iso_file2, track_number2, i) )
        {
            return false;
        }
    }

    return true;
}


void Mp4File::AppendAudio(std::string source_file_path)
{
    CheckFileIsOpenForEditing();

    Mp4File source_mp4_file;
    source_mp4_file.Open(std::move(source_file_path), OpenType::ReadOnlyExisting);

    const unsigned int source_track_number = source_mp4_file.GetFirstAudioTrackNumber();

    // if there is no audio track in the source file, there is nothing to do
    if( source_track_number == 0 )
        return;

    unsigned int destination_track_number = GetFirstAudioTrackNumber();
    uint64_t DTS_offset;

    // if there is no audio track in the destination file, clone it from the source's,
    // which will copy over information about the source's samples' codecs
    if( destination_track_number == 0 )
    {
        CheckResult(gf_isom_clone_track(
            source_mp4_file.m_data->iso_file,
            source_track_number,
            m_data->iso_file,
            static_cast<GF_ISOTrackCloneFlags>(0),
            &destination_track_number
        ));

        DTS_offset = 0;
    }

    // otherwise, make sure that the existing samples use the same codec
    // and determine the destination file's current duration, which will
    // be used to offset the added samples so that they are added sequentially
    // to the end of the existing audio
    else
    {
        if( !CheckCompatibilityForConcat(m_data->iso_file, destination_track_number,
                                         source_mp4_file.m_data->iso_file, source_track_number) )
        {
            throw CSProException("The audio samples in the MP4 file '%s' cannot be added to '%s' "
                                 "because they are of an incompatible type.",
                                 Path::GetFilename(source_mp4_file.m_data->file_path).c_str(),
                                 Path::GetFilename(m_data->file_path).c_str());
        }

        DTS_offset = gf_isom_get_media_duration(m_data->iso_file, destination_track_number);
    }

    ASSERT(destination_track_number != 0);

    const unsigned int sample_count = gf_isom_get_sample_count(source_mp4_file.m_data->iso_file, source_track_number);

    // add each sample
    for( unsigned int sample_index = 1; sample_index <= sample_count; ++sample_index )
    {
        unsigned int sample_description_index;
        GF_ISOSample* sample = gf_isom_get_sample(source_mp4_file.m_data->iso_file, source_track_number,
                                                  sample_index, &sample_description_index);

        if( sample == nullptr )
        {
            throw CSProException("Sample %d could not be read from the MP4 file '%s'",
                                 static_cast<int>(sample_index),
                                 Path::GetFilename(source_mp4_file.m_data->file_path).c_str());
        }

        // adjust the timing offset
        sample->DTS += DTS_offset;

        // add the sample
        CheckResult(gf_isom_add_sample(
            m_data->iso_file,
            destination_track_number,
            sample_description_index,
            sample
        ));

        gf_isom_sample_del(&sample);
    }
}
