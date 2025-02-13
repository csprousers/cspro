#pragma once

#include <zLogicO/FunctionTable.h>
#include <zToolsO/Serializer.h>


namespace Nodes
{
    struct SqlQuery
    {
        enum class Type : int { Paradata = 1, Dictionary, File };

        static constexpr int SetReportDataDestinationJson = -2;
        static_assert(Serializer::GetEarliestSupportedVersion() < Serializer::Iteration_8_0_000_1);

        FunctionCode function_code;
        Type source_type;
        int source_symbol_index_or_expression;
        int destination_symbol_index;
        int sql_query_expression;
    };
}
