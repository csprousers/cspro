#pragma once

#include <zUtilO/zUtilO.h>


// interface for the transaction generator
struct TransactionGenerator
{
    virtual ~TransactionGenerator() { }
    virtual bool CommitTransactions() = 0;
};


namespace TransactionManager
{
    CLASS_DECL_ZUTILO void CommitTransactions();
    CLASS_DECL_ZUTILO void Register(TransactionGenerator& transaction_generator);
    CLASS_DECL_ZUTILO void Deregister(TransactionGenerator& transaction_generator);
}
