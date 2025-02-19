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
    Resource,
    TableSpec,
};


ZAPPO_API const char* ToString(AppFileType app_file_type);

// extensions for the next two functions will not include / should not use dots
ZAPPO_API const char* GetFileExtension(AppFileType app_file_type);
ZAPPO_API std::optional<AppFileType> GetAppFileTypeFromFileExtension(std::string_view extension_sv);

// returns the order that application files should appear in a tree
ZAPPO_API int GetTreeOrder(AppFileType app_file_type);


constexpr bool IsApplicationType(AppFileType app_file_type)
{
    return ( app_file_type == AppFileType::ApplicationBatch ||
             app_file_type == AppFileType::ApplicationEntry ||
             app_file_type == AppFileType::ApplicationTabulation );
}
