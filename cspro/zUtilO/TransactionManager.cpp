#include "StdAfx.h"
#include "TransactionManager.h"


namespace TransactionManager
{
    std::set<TransactionGenerator*> TransactionGenerators;

    void CommitTransactions()
    {
        for( auto* const transaction_generator : TransactionGenerators )
            transaction_generator->CommitTransactions();
    }

    void Register(TransactionGenerator& transaction_generator)
    {
        TransactionGenerators.insert(&transaction_generator);
    }

    void Deregister(TransactionGenerator& transaction_generator)
    {
        TransactionGenerators.erase(&transaction_generator);
    }
}
