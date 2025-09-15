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

    // Loads the file, throwing exceptions on error.
    void Load(std::string file_path);

    // Saves the file, throwing exceptions on error.
    void Save(const std::string& file_path);

    // Returns the length of a WebM file, in seconds.
    // If the duration is defined in the file's header, it is returned.
    // Otherwise, the entire file is parsed to calculate the length.
    // 0 is returned if the object does not contain data, and DEFAULT is returned on error.
    double GetLength() const noexcept;

    // Returns the width and height of a WebM file, in pixels.
    // If the values are not defined in the header, an exception is thrown.
    const std::tuple<long long, long long>& GetWidthHeight() const;

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    // BinarySymbol overrides
    bool HasValidContent() const override;

private:
    struct BinaryDictionaryItemData { };
    struct Data;
    using VideoStorage = std::variant<std::string,                    // file path (if loaded from a file)
                                      std::shared_ptr<TemporaryFile>, // temporary file (if operated upon)
                                      BinaryDictionaryItemData>;      // binary dictionary item (in m_binarySymbolData)

private:
    // Returns the file path of the video storage.
    static const std::string& GetPath(const VideoStorage& video_storage);

    // When the data is stored in a binary dictionary item, this method returns a FILE*
    // handle to the data as stored in memory (on non-Windows systems where fmemopen is available).
    // On Windows, the data will be saved to a temporary file on the disk.
    // Exceptions are thrown on error.
    template<typename CF>
    void DoWithFilePathOrFile(const CF& callback_function) const;

    // Returns the Data object for this video, creating one if necessary when
    // the video data comes from a dictionary item.
    // If the video's status as a WebM file not been checked, it will be.
    // In the version with exceptions, an exception will be thrown if the file is not a valid WebM file.
    const Data* GetEvaluatedData_noexcept() const noexcept;
    const Data& GetEvaluatedData() const;

    BinaryData::ContentCallbackType CreateBinaryDataContentFromVideoCallback() const;

private:
    std::unique_ptr<Data> m_data;
};
