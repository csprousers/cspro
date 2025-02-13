#pragma once


// --------------------------------------------------------------------------
// inline implementations of LogicCompiler's node creation functions
// --------------------------------------------------------------------------

inline int* LogicCompiler::CreateCompilationSpace(int ints_needed)
{
    int* compilation_space = m_engineData->logic_byte_code.AdvancePosition(ints_needed);

    if( compilation_space == nullptr )
        IssueError(4);

    return compilation_space;
}


template<typename NodeType>
NodeType& LogicCompiler::GetNode(int program_index)
{
    return *reinterpret_cast<NodeType*>(m_engineData->logic_byte_code.GetCodeAtPosition(program_index));
}


template<typename NodeType>
NodeType& LogicCompiler::CreateNode(std::optional<FunctionCode> function_code/* = std::nullopt*/, int node_size_offset/* = 0*/)
{
    static_assert(sizeof(NodeType) % sizeof(int) == 0);

    int* compilation_space = CreateCompilationSpace(( sizeof(NodeType) / sizeof(int) ) + node_size_offset);

    if( function_code.has_value() )
        *compilation_space = static_cast<int>(*function_code);

    return *reinterpret_cast<NodeType*>(compilation_space);
}


template<typename NodeType>
NodeType& LogicCompiler::CreateVariableSizeNode(std::optional<FunctionCode> function_code, int number_arguments)
{
    return CreateNode<NodeType>(std::move(function_code), number_arguments - 1);
}


template<typename NodeType>
NodeType& LogicCompiler::CreateVariableSizeNode(int number_arguments)
{
    return CreateNode<NodeType>(std::nullopt, number_arguments - 1);
}


template<typename NodeType>
void LogicCompiler::InitializeNode(NodeType& compilation_node, int value, int node_start_offset/* = 0*/)
{
    int values_to_initialize = ( sizeof(NodeType) / sizeof(int) ) - node_start_offset;
    ASSERT(values_to_initialize >= 0);

    for( int* node_value = reinterpret_cast<int*>(&compilation_node) + node_start_offset;
         values_to_initialize > 0;
         --values_to_initialize, ++node_value )
    {
        *node_value = value;
    }
}


template<typename NodeType>
int LogicCompiler::GetProgramIndex(const NodeType& compilation_node)
{
    static_assert(!std::is_same_v<NodeType, int>);
    return m_engineData->logic_byte_code.GetPositionAtCode(reinterpret_cast<const int*>(&compilation_node));
}


template<typename NodeType>
int LogicCompiler::GetOptionalProgramIndex(const NodeType* compilation_node)
{
    return ( compilation_node != nullptr ) ? GetProgramIndex(*compilation_node) :
                                             -1;
}
