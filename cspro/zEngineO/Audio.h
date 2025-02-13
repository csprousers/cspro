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
    LogicAudio(std::wstring audio_name);
    LogicAudio(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor);
    LogicAudio(LogicAudio&& logic_audio) = delete;
    ~LogicAudio();

    LogicAudio& operator=(const LogicAudio& logic_audio);
    LogicAudio& operator=(const LogicDocument& logic_document);

    void Load(std::wstring filename);
    void Save(const std::wstring& filename, std::wstring application_name);

    void Record(std::optional<double> seconds);
    double Stop();
    double RecordInteractive(const std::wstring& message = std::wstring());

    void Play(const std::wstring& message = std::wstring());

    void Concat(const LogicAudio& logic_audio);
    void Concat(std::wstring filename);    

    double GetLength() const;

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void UpdateValueFromJson(const JsonNode<wchar_t>& json_node) override;

    // BinarySymbol overrides
    bool HasValidContent() const override;

private:
    using AudioStorage = std::variant<std::wstring, std::shared_ptr<TemporaryFile>>;

    struct Data
    {
        AudioStorage audio_storage;
        std::optional<int> sampling_rate;
        std::optional<double> duration;
        std::optional<bool> is_mp4a_format;
    };

private:
    static const std::wstring& GetPath(const AudioStorage& audio_storage);

    // returns a Data object, reading the sampling rate, duration, and type when possible
    static std::unique_ptr<Data> CreateData(AudioStorage audio_storage) noexcept;

    // if the audio data is coming from a dictionary item, it will be saved to a temporary file;
    // null is returned on error, or if no data is defined
    const Data* GetParsedData() const noexcept;

    const Data& GetParsedDataWithExceptions() const;

    BinaryData::ContentCallbackType CreateBinaryDataContentFromAudioCallback() const;

    double StopCurrentRecording();

    void Concat(AudioStorage audio_storage, const TCHAR* label, const TCHAR* source);

private:
    std::unique_ptr<Data> m_data;

    class InProgressRecording;
    std::unique_ptr<InProgressRecording> m_currentRecording;
};
