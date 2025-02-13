#pragma once

#include <zEngineO/zEngineO.h>

class DataRepository;
struct DataTransactionResource;
class LoopStack;
class MessageIssuer;
class Symbol;


// --------------------------------------------------------------------------
// LoopStackSource
// --------------------------------------------------------------------------

enum class LoopStackSource { Do, While, For, ForDictionary, ForCase, CountCases };



// --------------------------------------------------------------------------
// LoopStackEntry
// --------------------------------------------------------------------------

class LoopStackEntry
{
    friend class LoopStack;

public:
    struct Resource
    {
        virtual ~Resource() { }
    };

    bool IsValid() const { return ( m_loopStackIfValid != nullptr ); }

private:
    LoopStackEntry(LoopStack* loop_stack_if_valid, LoopStackSource source, const Symbol* symbol);

public:
    ~LoopStackEntry();

private:
    LoopStack* m_loopStackIfValid;
    const LoopStackSource m_source;
    const Symbol* m_symbol;
    std::vector<std::unique_ptr<Resource>> m_resources;
};



// --------------------------------------------------------------------------
// LoopStack
// --------------------------------------------------------------------------

class ZENGINEO_API LoopStack
{
    friend class LoopStackEntry;

public:
    LoopStack() = default;
    LoopStack(const LoopStack&) = delete;
    LoopStack& operator=(const LoopStack&) = delete;

    virtual ~LoopStack() { }

    size_t GetLoopStackCount() const { return m_loopStackEntries.size(); }
    size_t GetLoopStackCount(LoopStackSource source) const;

    LoopStackEntry PushOnLoopStack(LoopStackSource source, const Symbol* symbol = nullptr);

    void AddDataTransactionToLoopStack(DataRepository& data_repository);
    void RemoveDataTransactionFromLoopStack(DataRepository& data_repository);

protected:
    virtual MessageIssuer& GetMessageIssuer() = 0;

private:
    DataTransactionResource* FindDataTransactionOnLoopStack(const DataRepository& data_repository);

private:
    std::vector<LoopStackEntry*> m_loopStackEntries;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline LoopStackEntry::LoopStackEntry(LoopStack* loop_stack_if_valid, LoopStackSource source, const Symbol* symbol)
    :   m_loopStackIfValid(loop_stack_if_valid),
        m_source(source),
        m_symbol(symbol)
{
    if( IsValid() )
        m_loopStackIfValid->m_loopStackEntries.emplace_back(this);
}


inline LoopStackEntry::~LoopStackEntry()
{
    if( IsValid() )
    {
        ASSERT(this == m_loopStackIfValid->m_loopStackEntries.back());
        m_loopStackIfValid->m_loopStackEntries.pop_back();
    }
}
