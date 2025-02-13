#pragma once

#include <zAppO/zAppO.h>


enum class AppFileType
{
    ApplicationBatch,
    ApplicationEntry,
    ApplicationTabulation,
    Dictionary,
    Form,
    Code,
    Message,
    Order,
    QuestionText,
    Report,
    ResourceFolder,
    TableSpec,
};


ZAPPO_API const TCHAR* ToString(AppFileType app_file_type);

// extensions for the next two functions will not include / should not use dots
ZAPPO_API const TCHAR* GetFileExtension(AppFileType app_file_type);
ZAPPO_API std::optional<AppFileType> GetAppFileTypeFromFileExtension(wstring_view extension);


constexpr bool IsApplicationType(AppFileType app_file_type)
{
    return ( app_file_type == AppFileType::ApplicationBatch ||
             app_file_type == AppFileType::ApplicationEntry ||
             app_file_type == AppFileType::ApplicationTabulation );
}
