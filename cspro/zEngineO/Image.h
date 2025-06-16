#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/BinarySymbol.h>

class ExifReader;
class LogicDocument;
struct ViewerOptions;
namespace Multimedia { class Image; struct ImageDetails; }

// when an EngineImage wraps a LogicImage, the image is not parsed until necessary, so nearly
// all methods can throw exceptions because the image parsing may occur at that point


class ZENGINEO_API LogicImage : public BinarySymbol
{
private:
    LogicImage(const LogicImage& logic_image);

public:
    LogicImage(std::string image_name);
    LogicImage(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor);

    LogicImage& operator=(const LogicImage& logic_image);
    LogicImage& operator=(const LogicDocument& logic_document);
    LogicImage& operator=(const BinarySymbolData& binary_symbol_data);

    bool HasValidImage(bool parse_image_if_necessary) const noexcept;

    int GetWidth() const;
    int GetHeight() const;

    // Throws an exception if the ExifReader cannot be created (which should not happen).
    const ExifReader& GetExifReader();

    void Resample(int width, int height);

    void Load(std::string file_path, bool file_path_is_temporary = false);
    void Load(std::unique_ptr<const Multimedia::Image> image, std::string path_or_filename);

    // loads the image from a data URL, using binary_data_metadata as the base metadata
    void LoadFromDataUrl(std::string_view data_url_sv, BinaryDataMetadata binary_data_metadata = BinaryDataMetadata());

    void Save(std::string file_path, std::optional<int> jpeg_quality = std::nullopt);

    void View(const ViewerOptions* viewer_options) const;
    static void View(const std::vector<std::byte>& image_content, std::optional<Multimedia::ImageDetails> image_details, const std::string& image_file_path,
                     const ViewerOptions* viewer_options);

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;

    void Reset() override;

    void SetValueFromJson(const JsonNode& json_node) override;

    // BinarySymbol overrides
    bool HasValidContent() const override;

private:
    bool IsImageSet() const;
    void SetImage(std::unique_ptr<const Multimedia::Image> image);

    const Multimedia::Image& GetParsedImage();

    BinaryData::ContentCallbackType CreateBinaryDataContentFromImageCallback() const;

    // returns a non-null content validator
    static std::unique_ptr<BinarySymbolDataContentValidator> CreateContentValidator();

private:
    struct RuntimeData;
    std::shared_ptr<RuntimeData> m_runtimeData;
};
