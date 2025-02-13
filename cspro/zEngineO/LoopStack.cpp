#include "stdafx.h"
#include "LoopStack.h"
#include "Messages/EngineMessages.h"
#include "Messages/MessageIssuer.h"
#include <zDataO/DataRepositoryTransaction.h>


// --------------------------------------------------------------------------
// DataTransactionResource
// --------------------------------------------------------------------------

struct DataTransactionResource : public LoopStackEntry::Resource
{
    DataTransactionResource(DataRepository& data_repository_)
        :   transaction(std::make_unique<DataRepositoryTransaction>(data_repository_)),
            data_repository(data_repository_)
    {
    }

    std::unique_ptr<DataRepositoryTransaction> transaction;
    const DataRepository& data_repository;
};



// --------------------------------------------------------------------------
// LoopStack
// --------------------------------------------------------------------------

size_t LoopStack::GetLoopStackCount(LoopStackSource source) const
{
    return std::count_if(m_loopStackEntries.cbegin(), m_loopStackEntries.cend(),
                         [&](const LoopStackEntry* loop_stack_entry) { return ( loop_stack_entry->m_source == source ); });
}


LoopStackEntry LoopStack::PushOnLoopStack(LoopStackSource source, const Symbol* symbol/* = nullptr*/)
{
    ASSERT(symbol == nullptr || symbol->IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary));
    bool valid = true;

    for( const LoopStackEntry& loop_stack_entry : VI_V(m_loopStackEntries) )
    {
        // checks for relation and group for loops can be added later
        if( source == LoopStackSource::For )
            continue;

        // if in traditional dictionary for loop, don't allow any nested traditional for loops
        if( source == LoopStackSource::ForDictionary && loop_stack_entry.m_source == LoopStackSource::ForDictionary )
        {
            GetMessageIssuer().IssueError(MGF::LoopStack_nested_for_loop_751);
            valid = false;
        }

        // don't allow nested loops using the same symbol
        else if( loop_stack_entry.m_symbol == symbol && symbol != nullptr )
        {
            const TCHAR* const function_name = ( source == LoopStackSource::ForDictionary ) ? _T("for") :
                                               ( source == LoopStackSource::ForCase )       ? _T("forcase") :
                                               ( source == LoopStackSource::CountCases )    ? _T("countcases") :
                                                                                              ReturnProgrammingError(_T(""));

            GetMessageIssuer().IssueError(MGF::LoopStack_nested_dictionary_loop_753, function_name, symbol->GetName().c_str());
            valid = false;
        }
    }

    // add the symbol to the loop stack
    return LoopStackEntry(valid ? this : nullptr, source, symbol);
}


DataTransactionResource* LoopStack::FindDataTransactionOnLoopStack(const DataRepository& data_repository)
{
    // search for a transaction for this repository
    for( LoopStackEntry& loop_stack_entry : VI_V(m_loopStackEntries) )
    {
        for( LoopStackEntry::Resource* resource : VI_P(loop_stack_entry.m_resources) )
        {
            DataTransactionResource* data_transaction_resource = dynamic_cast<DataTransactionResource*>(resource);

            if( data_transaction_resource != nullptr && &data_transaction_resource->data_repository == &data_repository &&
                                                        data_transaction_resource->transaction != nullptr )
            {
                return data_transaction_resource;
            }
        }
    }

    return nullptr;
}


void LoopStack::AddDataTransactionToLoopStack(DataRepository& data_repository)
{
    // if not in a loop, then there is no need to do anything
    if( m_loopStackEntries.empty() )
        return;

    // see if a transaction has already been started for this repository
    const DataTransactionResource* data_transaction_resource = FindDataTransactionOnLoopStack(data_repository);

    // if not, add a new transaction
    if( data_transaction_resource == nullptr )
        m_loopStackEntries.back()->m_resources.emplace_back(std::make_unique<DataTransactionResource>(data_repository));
}


void LoopStack::RemoveDataTransactionFromLoopStack(DataRepository& data_repository)
{
    // if not in a loop, then there is no need to do anything
    if( m_loopStackEntries.empty() )
        return;

    // if a transaction has been started for this repository, end it
    DataTransactionResource* data_transaction_resource = FindDataTransactionOnLoopStack(data_repository);

    if( data_transaction_resource != nullptr )
        data_transaction_resource->transaction.reset();
}
