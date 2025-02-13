#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/BinarySymbol.h>

class LogicAudio;
class LogicGeometry;
class LogicImage;
struct ViewerOptions;


class ZENGINEO_API LogicDocument : public BinarySymbol
{
private:
    LogicDocument(const LogicDocument& logic_document);

public:
    LogicDocument(std::wstring document_name);
    LogicDocument(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor);

    LogicDocument& operator=(const LogicDocument& logic_document);
    LogicDocument& operator=(const LogicAudio& logic_audio);
    LogicDocument& operator=(const LogicGeometry& logic_geometry);
    LogicDocument& operator=(const LogicImage& logic_image);
    LogicDocument& operator=(const std::wstring& document_text);

    void Load(std::wstring filename);
    void Save(std::wstring filename);

    bool View(const ViewerOptions* viewer_options) const;

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;
};
