#include "stdafx.h"
#include "WebMFile.h"
#include <zToolsO/SafePath.h>
#include <mkvparser/mkvreader.h>


std::unique_ptr<mkvparser::MkvReader> WebMFile::CreateReader(const std::variant<cs::string_sz, FILE*> file_path_or_file)
{
    if( std::holds_alternative<FILE*>(file_path_or_file) )
    {
        ASSERT(std::get<FILE*>(file_path_or_file) != nullptr);
        return std::make_unique<mkvparser::MkvReader>(std::get<FILE*>(file_path_or_file));
    }

    const char* const file_path = std::get<cs::string_sz>(file_path_or_file).c_str();
    auto reader = std::make_unique<mkvparser::MkvReader>();

    if( reader->Open(SafePath(file_path).GetPath()) == 0 )
    {
        return reader;
    }

    else if( PortableFunctions::FileIsRegular(file_path) )
    {
        throw FileIO::Exception::FileOpenError(file_path);
    }

    else
    {
        throw FileIO::Exception::FileNotFound(file_path);
    }
}


struct WebMFile::ReaderAndSegment
{
    std::unique_ptr<mkvparser::MkvReader> reader;
    std::unique_ptr<mkvparser::Segment> segment;
    const mkvparser::SegmentInfo* segment_info;
};


WebMFile::ReaderAndSegment WebMFile::CreateReaderLoadSegment(const std::variant<cs::string_sz, FILE*> file_path_or_file)
{
    WebMFile::ReaderAndSegment reader_and_segment { CreateReader(file_path_or_file) };

    long long pos = 0;
    mkvparser::Segment* segment_ptr;

    if( mkvparser::Segment::CreateInstance(reader_and_segment.reader.get(), pos, segment_ptr) != 0 )
        throw CSProException("Could not find the WebM segment.");

    reader_and_segment.segment.reset(segment_ptr);

    if( ( reader_and_segment.segment->Load() != 0 ) ||
        ( ( reader_and_segment.segment_info = reader_and_segment.segment->GetInfo() ) == nullptr ) )
    {
        throw CSProException("Error reading WebM segment info.");
    }

    return reader_and_segment;
}


bool WebMFile::IsValidFile(const std::variant<cs::string_sz, FILE*> file_path_or_file)
{
    const std::unique_ptr<mkvparser::MkvReader> reader = CreateReader(file_path_or_file);
    mkvparser::EBMLHeader ebml_header;
    long long pos = 0;

    return ( ebml_header.Parse(reader.get(), pos) >= 0 &&
             strcmp(ebml_header.m_docType, "webm") == 0 );
}


double WebMFile::GetDuration(const std::variant<cs::string_sz, FILE*> file_path_or_file, const bool use_segment_duration_if_set)
{
    constexpr long long TimeCodeScale_ns = static_cast<long long>(1e9);
    constexpr long long TimeCodeScale_ms = static_cast<long long>(1e6);

    const WebMFile::ReaderAndSegment reader_and_segment = CreateReaderLoadSegment(file_path_or_file);

    // when allowed, we can use the segment's metadata if it contains a valid duration value
    if( use_segment_duration_if_set )
    {
        const long long duration = reader_and_segment.segment_info->GetDuration();

        if( duration > 0 )
        {
            const double time_code_scale = static_cast<double>(reader_and_segment.segment_info->GetTimeCodeScale());
            ASSERT(time_code_scale == TimeCodeScale_ms);

            // the duration's time code scale is for milliseconds, but the duration is reported in nanoseconds,
            // so durations must be further divided by 1000 to get seconds
            return ( duration / time_code_scale ) / 1000.0;
        }
    }

    // otherwise we will iterate over each cluster to calculate the highest timestamp (in milliseconds)
    struct MaxValues
    {
        long long time_unscaled = 0;
        long long track_number = -1;
        int block_frame_count = 0;
    };

    MaxValues cluster_max_values;

    const mkvparser::Cluster* cluster = reader_and_segment.segment->GetFirst();

    while( cluster != nullptr && !cluster->EOS() )
    {
        // determine the maximum time code in the blocks
        MaxValues block_max_values;

        const mkvparser::BlockEntry* block_entry;
        long block_entry_read_result = cluster->GetFirst(block_entry);

        while( true )
        {
            if( block_entry_read_result != 0 )
                throw CSProException("Error reading WebM cluster block entry.");

            if( block_entry == nullptr || block_entry->EOS() )
                break;

            const mkvparser::Block* block = block_entry->GetBlock();
            ASSERT(block != nullptr);

            const long long block_time_code_unscaled = block->GetTimeCode(nullptr);

            if( block_time_code_unscaled > block_max_values.time_unscaled )
            {
                block_max_values.time_unscaled = block_time_code_unscaled;
                block_max_values.track_number = block->GetTrackNumber();
                block_max_values.block_frame_count = block->GetFrameCount();
            }

            block_entry_read_result = cluster->GetNext(block_entry, block_entry);
        }

        // offset by the cluster start time
        ASSERT(( cluster->GetTimeCode() * reader_and_segment.segment_info->GetTimeCodeScale() ) == cluster->GetTime());
        block_max_values.time_unscaled += cluster->GetTimeCode();

        if( block_max_values.time_unscaled > cluster_max_values.time_unscaled )
            cluster_max_values = block_max_values;

        cluster = reader_and_segment.segment->GetNext(cluster);
    }

    // because the calculations were made without scaling,
    // we do not need to factor in the time scale to calculate the seconds
    double duration_s = cluster_max_values.time_unscaled / 1000.0;

    // the calculations were made off the start time of each block, so adjust by adding
    // the expected duration of the selected block (which is based in nanoseconds)
    if( cluster_max_values.block_frame_count != 0 )
    {
        const mkvparser::Track* const track = reader_and_segment.segment->GetTracks()->GetTrackByNumber(static_cast<long>(cluster_max_values.track_number));
        const unsigned long long default_duration = ( track != nullptr ) ? track->GetDefaultDuration() : 0;

        if( default_duration > 0 )
        {
            const double expected_block_duration_ns = static_cast<double>(cluster_max_values.block_frame_count) * default_duration;
            duration_s += expected_block_duration_ns / TimeCodeScale_ns;
        }
    }

    return duration_s;
}


std::tuple<long long, long long> WebMFile::GetWidthHeight(const std::variant<cs::string_sz, FILE*> file_path_or_file)
{
    const WebMFile::ReaderAndSegment reader_and_segment = CreateReaderLoadSegment(file_path_or_file);
    const mkvparser::Tracks* const tracks = reader_and_segment.segment->GetTracks();
    const unsigned long num_tracks = ( tracks != nullptr ) ? tracks->GetTracksCount() : 0;

    for( unsigned long i = 0; i < num_tracks; ++i )
    {
        const mkvparser::Track* const track = tracks->GetTrackByIndex(i);

        if( track != nullptr && track->GetType() == mkvparser::Track::kVideo )
        {
            const mkvparser::VideoTrack* const video_track = static_cast<const mkvparser::VideoTrack*>(track);

            return std::make_tuple(video_track->GetDisplayWidth(), video_track->GetDisplayHeight());
        }
    }

    throw CSProException("Could not find the video track in the WebM file.");
}
