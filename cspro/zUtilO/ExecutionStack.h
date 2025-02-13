#pragma once

#include <zUtilO/zUtilO.h>
#include <zToolsO/PointerClasses.h>

class ExecutionStackEntry;
class PFF;


// --------------------------------------------------------------------------
// ExecutionStack: a class to keep track of the programs being executed
// --------------------------------------------------------------------------

class CLASS_DECL_ZUTILO ExecutionStack
{
    friend ExecutionStackEntry;

public:
    // entry types
    struct ActionInvokerActivity { };
    using EntryType = std::variant<cs::non_null_shared_or_raw_ptr<const PFF>, ActionInvokerActivity>;

    // the state ID changes every time a program is added or removed from the execution stack
    static size_t GetStateId() { return m_stateId; }

    // adds an entry, returning a RAII object that removes the entry on deletion
    static ExecutionStackEntry AddEntry(EntryType entry);

    // returns the current entry stack
    static const std::map<size_t, EntryType>& GetEntries() { return m_entryStack; }

private:
    // removes the entry
    static void RemoveEntry(size_t entry_id);

private:
    static size_t m_stateId;
    static std::map<size_t, EntryType> m_entryStack;
};



// --------------------------------------------------------------------------
// ExecutionStackEntry: the RAII entry holder
// --------------------------------------------------------------------------

class ExecutionStackEntry
{
    friend ExecutionStack;

private:
    ExecutionStackEntry(size_t entry_id)
        :   m_entryId(entry_id)
    {
        ASSERT(m_entryId != 0);
    }

public:
    ExecutionStackEntry(const ExecutionStackEntry&) = delete;

    ExecutionStackEntry(ExecutionStackEntry&& rhs)
        :   m_entryId(rhs.m_entryId)
    {
        rhs.m_entryId = 0;
    }

    ~ExecutionStackEntry()
    {
        if( m_entryId != 0 )
            ExecutionStack::RemoveEntry(m_entryId);
    }

private:
    size_t m_entryId;
};
