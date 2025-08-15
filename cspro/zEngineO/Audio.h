#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/BinarySymbol.h>
#include <zUtilO/TemporaryFile.h>

class LogicDocument;


class ZENGINEO_API LogicAudio : public BinarySymbol
{
private:
    LogicAudio(const LogicAudio& logic_audio);

public:
    LogicAudio(std::string audio_name, const EngineData& engine_data);
    LogicAudio(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor, const EngineData& engine_data);
    LogicAudio(LogicAudio&& logic_audio) = delete;
    ~LogicAudio();

    LogicAudio& operator=(const LogicAudio& logic_audio);
    LogicAudio& operator=(const LogicDocument& logic_document);

    void Load(std::string file_path);
    void Save(const std::string& file_path);

    void Record(std::optional<double> seconds);
    double Stop();
    double RecordInteractive(const SharableString& message = SharableString());

    void Play(const SharableString& message = SharableString());

    void Concat(const LogicAudio& logic_audio);
    void Concat(std::string file_path);

    double GetLength() const;

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void SetValueFromJson(const JsonNode& json_node) override;

    // BinarySymbol overrides
    bool HasValidContent() const override;

private:
    using AudioStorage = std::variant<std::string, std::shared_ptr<TemporaryFile>>;

    struct Data
    {
        AudioStorage audio_storage;
        std::optional<int> sampling_rate;
        std::optional<double> duration;
        std::optional<bool> is_mp4a_format;
    };

private:
    static const std::string& GetPath(const AudioStorage& audio_storage);

    // returns a Data object, reading the sampling rate, duration, and type when possible
    static std::unique_ptr<Data> CreateData(AudioStorage audio_storage) noexcept;

    // if the audio data is coming from a dictionary item, it will be saved to a temporary file;
    // null is returned on error, or if no data is defined
    const Data* GetParsedData() const noexcept;

    const Data& GetParsedDataWithExceptions() const;

    BinaryData::ContentCallbackType CreateBinaryDataContentFromAudioCallback() const;

    double StopCurrentRecording();

    void Concat(AudioStorage audio_storage, const char* label, const char* source);

private:
    const EngineData& m_engineData;
    std::unique_ptr<Data> m_data;

    class InProgressRecording;
    std::unique_ptr<InProgressRecording> m_currentRecording;
};
