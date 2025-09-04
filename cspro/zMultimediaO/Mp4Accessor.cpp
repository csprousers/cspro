#include "stdafx.h"
#include "Mp4Accessor.h"
#include <zUtilO/Interapp.h>
#include <gpac/isomedia.h>


// --------------------------------------------------------------------------
// Mp4Accessor
// --------------------------------------------------------------------------

Mp4Accessor::Mp4Accessor()
{
    // initialize GPAC
    gf_sys_init(GF_MemTrackerNone, nullptr);
}


Mp4Accessor::~Mp4Accessor()
{
    // deinitialize GPAC, cleaning up all resources
    gf_sys_close();
}



// --------------------------------------------------------------------------
// Mp4MetadataReader
// --------------------------------------------------------------------------

Mp4Metadata Mp4MetadataReader::Read(const std::string& file_path)
{
    // ensure GPAC is initialized
    const Mp4Accessor mp4_accessor;

    // open the file for reading
    GF_ISOFile* const iso_file = gf_isom_open_GPAC_CSPRO(
        file_path.c_str(),
        GF_ISOM_OPEN_READ,
        nullptr,
        nullptr
    );

    if( iso_file == nullptr )
    {
        if( !PortableFunctions::FileIsRegular(file_path) )
            throw FileIO::Exception::FileNotFound(file_path);

        throw CSProException("The file is not a valid MP4 file: %s", file_path.c_str());
    }

    Mp4Metadata mp4_metadata;

    // read the metadata from the first audio track
    const unsigned int track_count = gf_isom_get_track_count(iso_file);

    for( unsigned int i = 1; i <= track_count; ++i )
    {
        if( gf_isom_get_media_type(iso_file, i) != GF_ISOM_MEDIA_AUDIO )
            continue;

        // get the sampling rate from the audio sample description
        unsigned int sample_rate;

        if( gf_isom_get_audio_info(iso_file, i, 1, &sample_rate, nullptr, nullptr) == GF_OK )
            mp4_metadata.sampling_rate = sample_rate;

        // get the duration
        const uint64_t duration = gf_isom_get_media_duration(iso_file, i);
        const unsigned int timescale = ( duration != 0 ) ? gf_isom_get_media_timescale(iso_file, i) : 0;

        if( timescale != 0 )
            mp4_metadata.duration = static_cast<double>(duration) / timescale;

        // check if the subtype is of type M4A
        const unsigned int subtype = gf_isom_get_mpeg4_subtype(iso_file, i, 1);

        if( subtype != 0 )
            mp4_metadata.is_mp4a_format = ( subtype == GF_4CC('m', 'p', '4', 'a') );
    }

    // close the file
    gf_isom_delete(iso_file);

    return mp4_metadata;
}



// --------------------------------------------------------------------------
// Mp4TagSetter
// --------------------------------------------------------------------------

struct Mp4TagSetter::Data
{
    std::string file_path;
    std::string temp_file_path;
    GF_ISOFile* iso_file;
};


Mp4TagSetter::Mp4TagSetter()
{
}


Mp4TagSetter::~Mp4TagSetter()
{
    if( m_data != nullptr )
    {
        ASSERT(m_data->iso_file != nullptr);
        gf_isom_delete(m_data->iso_file);
    }
}


void Mp4TagSetter::Open(std::string file_path)
{
    if( m_data != nullptr )
    {
        ASSERT(false);
        throw CSProException("A file is already open for setting tags: %s", m_data->file_path.c_str());
    }

    const std::string temp_directory = GetTempDirectory();
    std::string temp_file_path = PortableFunctions::GetUniqueFilePathInDirectory(temp_directory, "gpac");
    ASSERT(!PortableFunctions::FileExists(temp_file_path));

    // open the file for editing
    GF_ISOFile* const iso_file = gf_isom_open_GPAC_CSPRO(
        file_path.c_str(),
        GF_ISOM_OPEN_EDIT,
        temp_directory.c_str(),
        temp_file_path.c_str()
    );

    if( iso_file == nullptr )
    {
        if( !PortableFunctions::FileIsRegular(file_path) )
            throw FileIO::Exception::FileNotFound(file_path);

        throw CSProException("There was an error opening the file for setting tags: %s", file_path.c_str());
    }

    m_data.reset(new Data { std::move(file_path),
                            std::move(temp_file_path),
                            iso_file });
}


void Mp4TagSetter::CheckFileIsOpen() const
{
    if( m_data == nullptr )
    {
        ASSERT(false);
        throw CSProException("A file is not open for setting tags.");
    }
}


template<typename ErrorT>
void Mp4TagSetter::CheckResult(const ErrorT error) const
{
    ASSERT(m_data != nullptr);

    if( error != GF_OK )
    {
        throw CSProException("There was an error modifying '%s': %s",
                             Path::GetFilename(m_data->file_path).c_str(),
                             gf_error_to_string(error));
    }
}


void Mp4TagSetter::Close()
{
    CheckFileIsOpen();

    // save the changes and close the file
    CheckResult(gf_isom_close(m_data->iso_file));

    std::unique_ptr<Data> old_data = std::move(m_data);

    // if the changes were not made in-place, replace the existing file with the new one
    if( PortableFunctions::FileIsRegular(old_data->temp_file_path) )
    {
        PortableFunctions::FileDeleteWithExceptions(old_data->file_path);
        PortableFunctions::FileRenameWithExceptions(old_data->temp_file_path, old_data->file_path);
    }
}


void Mp4TagSetter::SetTextTag(const TextTag tag_type, const std::string_view text_sv)
{
    CheckFileIsOpen();

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


void Mp4TagSetter::SetBinaryTag(const BinaryTag tag_type, const std::string& binary_data_file_path)
{
    CheckFileIsOpen();

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
