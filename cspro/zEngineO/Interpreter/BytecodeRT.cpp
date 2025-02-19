#include "stdafx.h"
#include "IncludesRT.h"


const Nodes::List& LogicInterpreter::GetListNode(const int program_index) const
{
    if( program_index != -1 )
    {
        return GetNode<Nodes::List>(program_index);
    }

    else
    {
        // return a dummy empty list node
        ASSERT(m_engineData->PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1));
        return GetOptionalListNode(-1);
    }
}


const Nodes::List& LogicInterpreter::GetOptionalListNode(const int program_index) const
{
    if( program_index != -1 )
    {
        return GetNode<Nodes::List>(program_index);
    }

    else
    {
        // return a dummy empty list node
        const static Nodes::List list_node = { 0, 0 };
        return list_node;
    }
}


std::vector<int> LogicInterpreter::GetListNodeContents(const int program_index) const
{
    const Nodes::List& list_node = GetListNode(program_index);
    return std::vector<int>(list_node.elements, list_node.elements + list_node.number_elements);
}
