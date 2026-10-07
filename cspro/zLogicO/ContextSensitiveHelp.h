#pragma once

#include <zLogicO/zLogicO.h>
#include <span>

namespace Logic { class ContextSensitiveHelp; struct FunctionDetails; }


class ZLOGICO_API Logic::ContextSensitiveHelp
{
public:
    static const char* const GetTopicFilename(std::string_view text_sv, const FunctionDetails** function_details = nullptr);
    static const char* const GetTopicFilename(std::span<const std::string> dot_notation_entries, std::string_view text_sv, const FunctionDetails** function_details);
    static const char* const GetIntroductionTopicFilename();
    static bool UpdateTopicFilenameForMultipleWordExpressions(std::string_view text_sv, std::string_view second_text_sv, const char** help_topic_filename);
};
