#include "stdafx.h"
#include "Video.h"
#include <zMultimediaO/WebMFile.h>


// --------------------------------------------------------------------------
// LogicVideo::Data
// --------------------------------------------------------------------------

struct LogicVideo::Data
{
    VideoStorage video_storage;
    std::optional<bool> is_webm;
    std::optional<double> length;
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
    return std::holds_alternative<std::string>(video_storage)                    ? std::get<std::string>(video_storage) :
           std::holds_alternative<std::shared_ptr<TemporaryFile>>(video_storage) ? std::get<std::shared_ptr<TemporaryFile>>(video_storage)->GetPath() :
                                                                                   ReturnProgrammingError(SO::Empty_string);
}


template<typename CF>
void LogicVideo::DoWithFilePathOrFile(const CF& callback_function) const
{
    ASSERT(m_data != nullptr);

    // map a dictionary item's memory
    if( std::holds_alternative<BinaryDictionaryItemData>(m_data->video_storage) )
    {
        const std::vector<std::byte>& content = m_binarySymbolData.GetContent();

#ifdef WIN32
        // on Windows, fmemopen is unavailable unfortunately
        auto temporary_file = std::make_unique<TemporaryFile>();
        FileIO::Write(temporary_file->GetPath(), content);
        const_cast<LogicVideo*>(this)->m_data->video_storage = std::move(temporary_file);
#else
        const void* const content_data = static_cast<const void*>(content.data());
        FILE* const file = fmemopen(const_cast<void*>(content_data), content.size(), "rb");

        if( file == nullptr )
            throw ProgrammingErrorException();

        callback_function(std::variant<cs::string_sz, FILE*>(file));

        fclose(file);

        return;
#endif
    }

    ASSERT(!std::holds_alternative<BinaryDictionaryItemData>(m_data->video_storage));

    callback_function(std::variant<cs::string_sz, FILE*>(GetPath(m_data->video_storage)));
}


const LogicVideo::Data* LogicVideo::GetEvaluatedData_noexcept() const noexcept
{
    // when the data is coming from a dictionary item, create a Data reference to it
    if( m_data == nullptr && m_binarySymbolData.IsDefined() )
        const_cast<LogicVideo*>(this)->m_data.reset(new Data { BinaryDictionaryItemData() });

    if( m_data == nullptr )
        return nullptr;

    // if not calculated, determine whether the data is a WebM file
    if( !m_data->is_webm.has_value() )
    {
        try
        {
            DoWithFilePathOrFile(
                [this](const std::variant<cs::string_sz, FILE*> file_path_or_file)
                {
                    const_cast<LogicVideo*>(this)->m_data->is_webm = WebMFile::IsValidFile(file_path_or_file);
                });
        }
        catch(...) { }
    }

    return m_data.get();
}


const LogicVideo::Data& LogicVideo::GetEvaluatedData() const
{
    const Data* const evaluated_data = GetEvaluatedData_noexcept();

    if( evaluated_data == nullptr )
        throw CSProException("Video has no recording");

    if( evaluated_data->is_webm != true )
        throw CSProException("Cannot access the video data, or it is not of type WebM, in '%s'", GetName().c_str());

    return *evaluated_data;
}


bool LogicVideo::HasValidContent() const
{
    const Data* const evaluated_data = GetEvaluatedData_noexcept();

    return( evaluated_data != nullptr &&
            evaluated_data->is_webm == true );
}


BinaryData::ContentCallbackType LogicVideo::CreateBinaryDataContentFromVideoCallback() const
{
    ASSERT(m_data != nullptr);
    ASSERT(!std::holds_alternative<BinaryDictionaryItemData>(m_data->video_storage));

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
    m_data.reset(new Data { file_path });
    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromVideoCallback(), std::move(file_path));
}


void LogicVideo::Save(const std::string& file_path)
{
    if( !m_binarySymbolData.IsDefined() )
        throw CSProException("Video has no recording");

    // save from the disk...
    if( m_data != nullptr && !std::holds_alternative<BinaryDictionaryItemData>(m_data->video_storage) )
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


double LogicVideo::GetLength() const noexcept
{
    const Data* const evaluated_data = GetEvaluatedData_noexcept();

    if( evaluated_data == nullptr )
        return 0;

    if( evaluated_data->length.has_value() )
        return *evaluated_data->length;

    // calculate the length
    std::optional<double>& length = const_cast<LogicVideo*>(this)->m_data->length;

    // length cannot be reported for non-WebM files
    if( evaluated_data->is_webm != true )
    {
        length = DEFAULT;
    }

    else
    {
        try
        {
            DoWithFilePathOrFile(
                [&length](const std::variant<cs::string_sz, FILE*> file_path_or_file)
                {
                    length = WebMFile::GetDuration(file_path_or_file, true);
                });
        }

        catch(...)
        {
            ASSERT(false);
            length = DEFAULT;
        }
    }

    return *length;
}
