#include "stdafx.h"
#include "Video.h"


LogicVideo::LogicVideo(std::string video_name)
    :   BinarySymbol(std::move(video_name), SymbolType::Video)
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
