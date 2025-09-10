#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/BinarySymbol.h>


class ZENGINEO_API LogicVideo : public BinarySymbol
{
private:
    LogicVideo(const LogicVideo& logic_video);

public:
    LogicVideo(std::string video_name);
    LogicVideo(const EngineItem& engine_item, ItemIndex item_index, cs::non_null_shared_or_raw_ptr<BinaryDataAccessor> binary_data_accessor);
    ~LogicVideo();

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;
};
