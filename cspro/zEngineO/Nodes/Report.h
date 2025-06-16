#pragma once

#include <zLogicO/FunctionTable.h>

namespace Nodes { namespace Report { struct Save; struct View; } }


struct Nodes::Report::Save
{
    FunctionCode function_code;
    int symbol_index;
    int filename_expression;
};


struct Nodes::Report::View
{
    FunctionCode function_code;
    int symbol_index;
    int viewer_options_node_index;
};
