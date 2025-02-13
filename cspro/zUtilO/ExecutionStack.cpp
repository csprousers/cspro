#include "StdAfx.h"
#include "ExecutionStack.h"


size_t ExecutionStack::m_stateId = 0;
std::map<size_t, ExecutionStack::EntryType> ExecutionStack::m_entryStack;


ExecutionStackEntry ExecutionStack::AddEntry(EntryType entry)
{
    ++m_stateId;

    m_entryStack.try_emplace(m_stateId, std::move(entry));

    return ExecutionStackEntry(m_stateId);
}


void ExecutionStack::RemoveEntry(const size_t entry_id)
{
    ASSERT(entry_id != 0 && m_entryStack.count(entry_id) == 1);

    m_entryStack.erase(entry_id);

    ++m_stateId;
}
