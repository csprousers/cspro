#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/BinarySymbol.h>


class ZENGINEO_API LogicVideo : public BinarySymbol
{
private:
    LogicVideo(const LogicVideo& logic_video);

public:
    LogicVideo(std::string video_name);
    ~LogicVideo();

    // Symbol overrides
    std::unique_ptr<Symbol> CloneInInitialState() const override;
};
