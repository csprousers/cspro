#pragma once

#include <zLogicO/zLogicO.h>
#include <zAppO/LogicSettings.h>


class ZLOGICO_API LogicFormatter
{
public:
    LogicFormatter(LogicSettings::Version logic_version) noexcept;

    std::string FormatLogic(const std::string& logic) const;

private:
    std::string_view m_multilineCommentStart_sv;
    std::string_view m_multilineCommentEnd_sv;
};
