#pragma once

#include <zLogicO/FunctionTable.h>


namespace Nodes
{
    struct HtmlDialog
    {
        FunctionCode function_code;
        int filename_expression;
        int input_data_expression;
        int display_options_json_expression;
        int single_input_version;
    };


    struct Prompt
    {
        FunctionCode function_code;
        int title_expression;
        int initial_value_expression;
        int flags;

        static constexpr int NumericFlag   = 0x01;
        static constexpr int PasswordFlag  = 0x02;
        static constexpr int UppercaseFlag = 0x04;
        static constexpr int MultilineFlag = 0x08;
    };


    struct SetFont
    {
        FunctionCode function_code;
        int font_name_expression;
        int font_size_expression;
        int font_attributes;

        static constexpr int TypeMask    = 0x0FF;
        static constexpr int BoldMask    = 0x100;
        static constexpr int ItalicsMask = 0x200;
        static constexpr int DefaultMask = 0x400;
    };


    struct View
    {
        FunctionCode function_code;
        int symbol_index_or_source_expression; // this is a symbol if < 0
        int subscript_compilation;
        int viewer_options_node_index;
    };


    struct ViewerOptions
    {
        int width_expression;
        int height_expression;
        int title_expression;
        int show_close_button_expression;
    };
}
