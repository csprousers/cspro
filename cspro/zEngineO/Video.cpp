#include "stdafx.h"
#include "Video.h"
#include <zMultimediaO/WebMFile.h>


// --------------------------------------------------------------------------
// LogicVideo::Data
// --------------------------------------------------------------------------

struct LogicVideo::Data
{
    VideoStorage video_storage;
    bool is_webm = false;
};



// --------------------------------------------------------------------------
// LogicVideo
// --------------------------------------------------------------------------

LogicVideo::LogicVideo(std::string video_name)
    :   BinarySymbol(std::move(video_name), SymbolType::Video)
{
}


LogicVideo::LogicVideo(const EngineItem& engine_item, ItemIndex item_index,
                       cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor)
    :   BinarySymbol(engine_item, std::move(item_index), std::move(binary_data_accessor))
{
}


LogicVideo::LogicVideo(const LogicVideo& logic_video)
    :   BinarySymbol(logic_video)
{
    // the copy constructor is only used for symbols cloned in an initial state, so we do not need to copy the data from the other symbol
}


LogicVideo::~LogicVideo()
{
}


std::unique_ptr<Symbol> LogicVideo::CloneInInitialState() const
{
    return std::unique_ptr<LogicVideo>(new LogicVideo(*this));
}


void LogicVideo::Reset()
{
    BinarySymbol::Reset();
    m_data.reset();
}


const std::string& LogicVideo::GetPath(const VideoStorage& video_storage)
{
    return std::holds_alternative<std::string>(video_storage) ? std::get<std::string>(video_storage) :
                                                                std::get<std::shared_ptr<TemporaryFile>>(video_storage)->GetPath();
}


std::unique_ptr<LogicVideo::Data> LogicVideo::CreateData(VideoStorage video_storage) noexcept
{
    std::unique_ptr<Data> data(new Data { std::move(video_storage) });

    try
    {
        data->is_webm = WebMFile::IsValidFile(GetPath(data->video_storage));
    }
    catch(...) { ASSERT(false); }

    return data;
}



bool LogicVideo::HasValidContent() const
{
    // VIDEO_TODO ... check the data in a dictionary item
    return ( m_data != nullptr && m_data->is_webm );
}


BinaryData::ContentCallbackType LogicVideo::CreateBinaryDataContentFromVideoCallback() const
{
    ASSERT(m_data != nullptr);

    return
        [video_storage = m_data->video_storage]() -> std::shared_ptr<const std::vector<std::byte>>
        {
            try
            {
                return FileIO::Read(GetPath(video_storage));
            }

            catch(...)
            {
                return ReturnProgrammingError(std::make_shared<const std::vector<std::byte>>());
            }
        };
}


void LogicVideo::Load(std::string file_path)
{
    if( !PortableFunctions::FileIsRegular(file_path) )
        throw FileIO::Exception::FileNotFound(file_path);

    // because videos can be large, we do not load it into memory but
    // instead use a callback to read it from the disk when needed
    m_data = CreateData(file_path);
    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromVideoCallback(), std::move(file_path));
}


void LogicVideo::Save(const std::string& file_path)
{
    if( !m_binarySymbolData.IsDefined() )
        throw CSProException("Video has no recording");

    // save from the disk...
    if( m_data != nullptr )
    {
        FileIO::CreateDirectoriesForFile(file_path);
        PortableFunctions::FileCopyWithExceptions(GetPath(m_data->video_storage), file_path, FileOverwriteFlag::Always);

        m_data->video_storage = file_path;
    }

    // ...or from a binary dictionary item
    else
    {
        FileIO::Write(file_path, m_binarySymbolData.GetContent());
    }

    m_binarySymbolData.SetPath(file_path);
}
