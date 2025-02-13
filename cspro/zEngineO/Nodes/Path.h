#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    namespace Path
    {
        enum class FilterType : int { Audio = -2, Geometry = -3, Image = -4 };

        namespace Text
        {
            constexpr const TCHAR* FileType = _T("FileType");
            constexpr const TCHAR* Audio    = _T("Audio");
            constexpr const TCHAR* Geometry = _T("Geometry");
            constexpr const TCHAR* Image    = _T("Image");
        }
    }


    struct DirectoryVariant
    {
        enum class Type : int { String = 0, Symbol, Path, Media };

        enum class PathType : int
        {
            Temp = 1,
            Application,
            InputFile,
            CSPro,
            Desktop,
            Windows,
            Documents,
            ProgramFiles32,
            ProgramFiles64,
            CSEntry,
            CSEntryExternal,
            Html,
            Downloads,
        };            

        Type type;
        int code_or_expression;
    };


    struct DirList
    {
        FunctionCode function_code;
        int list_symbol_index;
        int directory_variant_index;
        int filter_type_or_expression;
        int recursive_expression;
    };


    struct PathSelectFile
    {
        FunctionCode function_code;
        int title_expression;
        int show_directories_expression;
        int filter_type_or_expression;
        int start_directory_variant_index;
        int root_directory_variant_index;
    };
}
