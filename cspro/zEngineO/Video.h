#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/BinarySymbol.h>
#include <zUtilO/TemporaryFile.h>


class ZENGINEO_API LogicVideo : public BinarySymbol
{
private:
    LogicVideo(const LogicVideo& logic_video);

public:
    LogicVideo(std::string video_name);
    LogicVideo(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor);
    ~LogicVideo();

    void Load(std::string file_path);
    void Save(const std::string& file_path);

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    // BinarySymbol overrides
    bool HasValidContent() const override;

private:
    using VideoStorage = std::variant<std::string,        // file path (if loaded from a file)
                         std::shared_ptr<TemporaryFile>>; // temporary file (if operated upon)
    struct Data;

private:
    // Returns the file path of the video storage.
    static const std::string& GetPath(const VideoStorage& video_storage);

    // Returns a Data object, checking if the file is a WebM video.
    static std::unique_ptr<Data> CreateData(VideoStorage video_storage) noexcept;

    BinaryData::ContentCallbackType CreateBinaryDataContentFromVideoCallback() const;

private:
    std::unique_ptr<Data> m_data;
};
