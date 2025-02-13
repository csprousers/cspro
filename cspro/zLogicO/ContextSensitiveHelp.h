#pragma once

#include <zLogicO/zLogicO.h>
#include <zToolsO/span.h>


namespace Logic
{
    struct FunctionDetails;


    class ZLOGICO_API ContextSensitiveHelp
    {
    public:
        static const TCHAR* const GetTopicFilename(wstring_view text, const FunctionDetails** function_details = nullptr);
        static const TCHAR* const GetTopicFilename(cs::span<const std::wstring> dot_notation_entries, wstring_view text, const FunctionDetails** function_details);
        static const TCHAR* const GetIntroductionTopicFilename();
        static bool UpdateTopicFilenameForMultipleWordExpressions(wstring_view text, wstring_view second_text, const TCHAR** help_topic_filename);
    };
}
