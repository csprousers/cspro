#include "stdafx.h"
#include "Audio.h"
#include "Document.h"
#include <zUtilO/Interapp.h>
#include <zMultimediaO/Mp4Reader.h>
#include <zMultimediaO/Mp4Writer.h>


// --------------------------------------------------------------------------
// LogicAudio::InProgressRecording
// --------------------------------------------------------------------------

class LogicAudio::InProgressRecording
{
public:
    InProgressRecording()
        :   m_temporaryFile(std::make_unique<TemporaryFile>())
    {
    }

    ~InProgressRecording()
    {
        if( m_temporaryFile != nullptr )
        {
            try
            {
#ifndef WIN_DESKTOP
                PlatformInterface::GetInstance()->GetApplicationInterface()->AudioStopRecording();
#endif
            }
            catch(...) { ASSERT(false); }
        }
    }

    const std::string& GetPath() const { return m_temporaryFile->GetPath(); }

    std::unique_ptr<TemporaryFile> ReleaseTemporaryFile() { return std::move(m_temporaryFile); }

private:
    std::unique_ptr<TemporaryFile> m_temporaryFile;
};



// --------------------------------------------------------------------------
// LogicAudio
// --------------------------------------------------------------------------

LogicAudio::LogicAudio(std::string audio_name, const EngineData& engine_data)
    :   BinarySymbol(std::move(audio_name), SymbolType::Audio),
        m_engineData(engine_data)
{
}


LogicAudio::LogicAudio(const EngineItem& engine_item, ItemIndex item_index,
                       cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor,
                       const EngineData& engine_data)
    :   BinarySymbol(engine_item, std::move(item_index), std::move(binary_data_accessor)),
        m_engineData(engine_data)
{
}


LogicAudio::LogicAudio(const LogicAudio& logic_audio)
    :   BinarySymbol(logic_audio),
        m_engineData(logic_audio.m_engineData)
{
    // the copy constructor is only used for symbols cloned in an initial state, so we do not need to copy the data from the other symbol
}


LogicAudio::~LogicAudio()
{
}


std::unique_ptr<Symbol> LogicAudio::CloneInInitialState() const
{
    return std::unique_ptr<LogicAudio>(new LogicAudio(*this));
}


LogicAudio& LogicAudio::operator=(const LogicAudio& logic_audio)
{
    if( this != &logic_audio )
    {
        m_binarySymbolData = logic_audio.m_binarySymbolData;

        m_data = ( logic_audio.m_data != nullptr) ? std::make_unique<Data>(*logic_audio.m_data) :
                                                    nullptr;

        m_currentRecording.reset();
    }

    return *this;
}


LogicAudio& LogicAudio::operator=(const LogicDocument& logic_document)
{
    const BinarySymbolData& document_binary_symbol_data = logic_document.GetBinarySymbolData();
    std::unique_ptr<LogicAudio::Data> document_audio_data;

    if( document_binary_symbol_data.IsDefined() )
    {
        AudioStorage audio_storage = document_binary_symbol_data.GetPath();

        // if the Document exists on the disk, use it; otherwise save it to a temporary file
        if( !PortableFunctions::FileIsRegular(GetPath(audio_storage)) )
        {
            audio_storage = std::make_shared<TemporaryFile>();
            FileIO::Write(GetPath(audio_storage), document_binary_symbol_data.GetContent());
        }

        // make sure this is compatible audio
        document_audio_data = CreateData(std::move(audio_storage));

        if( document_audio_data->is_mp4a_format != true )
            throw CSProException("The Document '%s' has data that cannot be converted to Audio.", logic_document.GetName().c_str());
    }

    m_binarySymbolData = document_binary_symbol_data;
    m_data = std::move(document_audio_data);
    m_currentRecording.reset();

    return *this;
}


void LogicAudio::Reset()
{
    BinarySymbol::Reset();
    m_data.reset();
    m_currentRecording.reset();
}


const std::string& LogicAudio::GetPath(const AudioStorage& audio_storage)
{
    return std::holds_alternative<std::string>(audio_storage) ? std::get<std::string>(audio_storage) :
                                                                std::get<std::shared_ptr<TemporaryFile>>(audio_storage)->GetPath();
}


std::unique_ptr<LogicAudio::Data> LogicAudio::CreateData(AudioStorage audio_storage) noexcept
{
    auto data = std::make_unique<Data>(Data { std::move(audio_storage) });

    try
    {
        Mp4Reader reader(GetPath(data->audio_storage));

        // catch all errors reading the properties
        try { data->sampling_rate = reader.GetAudioTimeScale(); } catch( const Mp4ReaderError& ) { }

        try { data->duration = reader.GetDuration(); } catch( const Mp4ReaderError& ) { }

        try { data->is_mp4a_format = ( strcmp(reader.GetAudioFormat(), "mp4a") == 0 ); } catch( const Mp4ReaderError& ) { }
    }

    catch(...)
    {
        // probably not mp4 which is okay as long as we don't append to it
    }

    return data;
}


const LogicAudio::Data* LogicAudio::GetParsedData() const noexcept
{
    if( m_data == nullptr && m_binarySymbolData.IsDefined() )
    {
        // we will be here when the data is coming from a dictionary item
        try
        {
            auto temporary_file = std::make_unique<TemporaryFile>();
            FileIO::Write(temporary_file->GetPath(), m_binarySymbolData.GetContent());
            const_cast<LogicAudio*>(this)->m_data = CreateData(std::move(temporary_file));
        }

        catch(...)
        {
            ASSERT(false);
        }
    }

    return m_data.get();
}


const LogicAudio::Data& LogicAudio::GetParsedDataWithExceptions() const
{
    ASSERT(m_binarySymbolData.IsDefined());

    const Data* const parsed_data = GetParsedData();

    if( parsed_data == nullptr )
        throw CSProException("Cannot access the audio data in '%s'", GetName().c_str());

    return *parsed_data;
}


bool LogicAudio::HasValidContent() const
{
    const Data* const parsed_data = GetParsedData();

    return ( parsed_data != nullptr &&
             parsed_data->is_mp4a_format == true );
}


BinaryData::ContentCallbackType LogicAudio::CreateBinaryDataContentFromAudioCallback() const
{
    ASSERT(m_data != nullptr);

    return
        [audio_storage = m_data->audio_storage]() -> std::shared_ptr<const std::vector<std::byte>>
        {
            try
            {
                return FileIO::Read(GetPath(audio_storage));
            }

            catch(...)
            {
                return ReturnProgrammingError(std::make_shared<const std::vector<std::byte>>());
            }
        };
}


void LogicAudio::Load(std::string file_path)
{
    if( !PortableFunctions::FileIsRegular(file_path) )
        throw FileIO::Exception::FileNotFound(file_path);

    m_data = CreateData(file_path);
    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromAudioCallback(), std::move(file_path));
}


void LogicAudio::Save(const std::string& file_path, std::string application_name)
{
    if( m_currentRecording != nullptr )
        StopCurrentRecording();

    if( !m_binarySymbolData.IsDefined() )
        throw CSProException("Audio has no recording");

    if( m_data != nullptr )
    {
        FileIO::CreateDirectoriesForFile(file_path);
        PortableFunctions::FileCopyWithExceptions(GetPath(m_data->audio_storage), file_path, FileOverwriteFlag::Always);

        m_data->audio_storage = file_path;
    }

    else
    {
        FileIO::Write(file_path, m_binarySymbolData.GetContent());
    }

    m_binarySymbolData.SetPath(file_path);

    // on a successful write, set the tags, ignoring errors doing so
    try
    {
        Mp4Writer writer(file_path, false);

        std::string artwork_image_path = Path::Combine(Html::GetDirectory(Html::Subdirectory::Images),
                                                       "cspro-logo-medium.png");
        ASSERT(PortableFunctions::FileIsRegular(artwork_image_path));

        writer.SetTags(Mp4Metadata { Path::GetFilenameWithoutExtension(file_path),
                                     "CSPro",
                                     std::move(application_name),
                                     std::move(artwork_image_path),
                                   });
    }
    catch(...) { }
}


void LogicAudio::Record(const std::optional<double> seconds)
{
    if( m_currentRecording != nullptr )
        StopCurrentRecording();

    std::optional<int> sampling_rate;

    if( m_binarySymbolData.IsDefined() )
    {
        const Data& parsed_data = GetParsedDataWithExceptions();

        if( parsed_data.is_mp4a_format != true )
            throw CSProException("The format and bitrate of this audio file are not compatible with CSPro audio recording");

        sampling_rate = parsed_data.sampling_rate;
    }

#ifdef WIN_DESKTOP
    UNREFERENCED_PARAMETER(seconds);
#else
    auto in_progress_recording = std::make_unique<InProgressRecording>();

    if( PlatformInterface::GetInstance()->GetApplicationInterface()->AudioStartRecording(in_progress_recording->GetPath(), seconds, sampling_rate) )
    {
        m_currentRecording = std::move(in_progress_recording);
    }

    else
#endif
    {
        throw CSProException("Failed to start audio recorder");
    }
}


double LogicAudio::Stop()
{
    if( m_currentRecording == nullptr )
        throw CSProException("No recording in progress");

    return StopCurrentRecording();
}


double LogicAudio::StopCurrentRecording()
{
    ASSERT(m_currentRecording != nullptr);

#ifndef WIN_DESKTOP
    if( !PlatformInterface::GetInstance()->GetApplicationInterface()->AudioStopRecording() )
        throw CSProException("Failed to stop audio recorder");
#endif

    std::unique_ptr<LogicAudio::Data> recorded_data = CreateData(m_currentRecording->ReleaseTemporaryFile());
    m_currentRecording.reset();

    Concat(std::move(recorded_data->audio_storage), "Audio Recording (Background)", "Audio.record");

    ASSERT(recorded_data->duration.has_value());
    return recorded_data->duration.value_or(DEFAULT);
}


double LogicAudio::RecordInteractive(const SharableString& message/* = SharableString()*/)
{
    if( m_currentRecording != nullptr )
        StopCurrentRecording();

    std::optional<int> sampling_rate;

    if( m_binarySymbolData.IsDefined() )
    {
        const Data& parsed_data = GetParsedDataWithExceptions();

        if( parsed_data.is_mp4a_format != true )
            throw CSProException("The format and bitrate of this audio file are not compatible with CSPro audio recording");

        sampling_rate = parsed_data.sampling_rate;
    }

    std::unique_ptr<TemporaryFile> temporary_file;

#ifdef WIN_DESKTOP
    UNREFERENCED_PARAMETER(message);
#else
    temporary_file = PlatformInterface::GetInstance()->GetApplicationInterface()->AudioRecordInteractive(*message, sampling_rate);
#endif

    if( temporary_file == nullptr )
        throw CSProException("Failed to record audio");

    if( PortableFunctions::FileSize(temporary_file->GetPath()) <= 0 )
        return DEFAULT;

    std::unique_ptr<LogicAudio::Data> recorded_data = CreateData(std::move(temporary_file));

    Concat(std::move(recorded_data->audio_storage), "Audio Recording (Interactive)", "Audio.recordInteractive");

    ASSERT(recorded_data->duration.has_value());
    return recorded_data->duration.value_or(DEFAULT);
}


void LogicAudio::Play(const SharableString& message/* = SharableString()*/)
{
    if( m_currentRecording != nullptr )
        StopCurrentRecording();

    if( !m_binarySymbolData.IsDefined() )
        throw CSProException("Audio has no recording");

    const Data& parsed_data = GetParsedDataWithExceptions();

#ifdef WIN_DESKTOP
    UNREFERENCED_PARAMETER(message);
    parsed_data;
#else
    if( !PlatformInterface::GetInstance()->GetApplicationInterface()->AudioPlay(GetPath(parsed_data.audio_storage), *message) )
#endif
    {
        throw CSProException("Failed to play audio");
    }
}


void LogicAudio::Concat(const LogicAudio& logic_audio)
{
    if( !logic_audio.m_binarySymbolData.IsDefined() )
        return;

    const Data& rhs_parsed_data = logic_audio.GetParsedDataWithExceptions();

    Concat(rhs_parsed_data.audio_storage, nullptr, "Audio.concat");
}


void LogicAudio::Concat(std::string file_path)
{
    if( !PortableFunctions::FileIsRegular(file_path) )
        throw FileIO::Exception::FileNotFound(file_path);

    Concat(AudioStorage(std::move(file_path)), nullptr, "Audio.concat");
}


void LogicAudio::Concat(AudioStorage audio_storage, const char* const label, const char* const source)
{
    ASSERT(PortableFunctions::FileIsRegular(GetPath(audio_storage)));

    // no need to concatenate anything if this object has no data
    if( !m_binarySymbolData.IsDefined() )
    {
        m_data = CreateData(std::move(audio_storage));
    }

    // otherwise concatenate the two files
    else
    {
        const Data& lhs_parsed_data = GetParsedDataWithExceptions();
        const std::string& lhs_path = GetPath(lhs_parsed_data.audio_storage);
        const std::string& rhs_path = GetPath(audio_storage);

        // we can append in place when the destination is a temporary file that is not used by other objects
        if( std::holds_alternative<std::shared_ptr<TemporaryFile>>(lhs_parsed_data.audio_storage) &&
            std::get<std::shared_ptr<TemporaryFile>>(lhs_parsed_data.audio_storage).use_count() == 1 )
        {
            // Mp4Writer closes the file on destruction
            {
                Mp4Writer writer(lhs_path, false);
                writer.AppendAudioTracks(rhs_path);
            }

            m_data = CreateData(lhs_parsed_data.audio_storage);
        }

        // otherwise create a new file
        else
        {
            auto concatenated_file = std::make_unique<TemporaryFile>();

            // Mp4Writer closes the file on destruction
            {
                Mp4Writer writer(concatenated_file->GetPath(), true);
                writer.AppendAudioTracks(lhs_path);
                writer.AppendAudioTracks(rhs_path);
            }

            m_data = CreateData(std::move(concatenated_file));
        }
    }

    m_binarySymbolData.SetBinaryData(CreateBinaryDataContentFromAudioCallback(),
                                     std::string(), // no filename
                                     m_data->is_mp4a_format.value_or(false) ? MimeType::Type::AudioM4A : std::string());

    // update the metadata
    BinaryDataMetadata& binary_data_metadata = m_binarySymbolData.GetMetadata();

    if( label != nullptr )
        binary_data_metadata.SetProperty("label", label);

    if( source != nullptr )
        binary_data_metadata.SetProperty("source", source);

    binary_data_metadata.SetProperty("timestamp", GetTimestamp<double>());
}


double LogicAudio::GetLength() const
{
    if( !m_binarySymbolData.IsDefined() )
        return 0;

    const Data* const parsed_data = GetParsedData();

    if( parsed_data != nullptr && parsed_data->duration.has_value() )
        return *parsed_data->duration;

    return DEFAULT;
}


void LogicAudio::SetValueFromJson(const JsonNode& json_node)
{
    struct LogicAudioContentValidator : public BinarySymbolDataContentValidator
    {
    public:
        std::unique_ptr<Data> ReleaseData() { return std::move(m_data); }

        bool ValidateContent(std::shared_ptr<const std::vector<std::byte>> content) override
        {
            AudioStorage audio_storage(std::make_shared<TemporaryFile>());
            FileIO::Write(GetPath(audio_storage), *content);

            m_data = CreateData(std::move(audio_storage));

            if( m_data->is_mp4a_format != true )
                throw CSProException("The data cannot be converted to Audio.");

            return true;
        }

    private:
        std::unique_ptr<Data> m_data;
    };

    LogicAudioContentValidator logic_audio_content_validator;

    m_binarySymbolData.SetSymbolValueFromJson(*this, json_node, &logic_audio_content_validator);

    if( m_binarySymbolData.IsDefined() )
        m_binarySymbolData.GetMetadata().SetMimeType(MimeType::Type::AudioM4A);

    m_data = std::move(logic_audio_content_validator.ReleaseData());
    m_currentRecording.reset();
}
