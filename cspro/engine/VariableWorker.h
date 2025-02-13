#pragma once

#include <zEngineO/Block.h>
#include <engine/Form2.h>
#include <engine/VarT.h>
#include <Zissalib/GroupT.h>
#include <Zissalib/SecT.h>


template<typename VariableFunction>
int VariableWorker(const Logic::SymbolTable& symbol_table, Symbol* symbol, const VariableFunction& variable_worker_function)
{
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return symbol_table; };

    ASSERT(symbol != nullptr);
    int num_fields_processed = 0;

    // a variable
    if( symbol->IsA(SymbolType::Variable) )
    {
        num_fields_processed += variable_worker_function(assert_cast<VART*>(symbol)) ? 1 : 0;
    }

    // a block
    else if( symbol->IsA(SymbolType::Block) )
    {
        const EngineBlock& engine_block = assert_cast<const EngineBlock&>(*symbol);

        for( VART* pVarT : engine_block.GetVarTs() )
            num_fields_processed += variable_worker_function(pVarT);
    }

    // a form or group
    else if( symbol->IsOneOf(SymbolType::Form, SymbolType::Group) )
    {
        GROUPT* pGroupT = nullptr;

        if( symbol->IsA(SymbolType::Form) )
        {
            FORM* pFormT = assert_cast<FORM*>(symbol);
            int iSymGroup = pFormT->GetSymGroup();
            pGroupT = ( iSymGroup > 0 ) ? assert_cast<GROUPT*>(symbol) : nullptr;
        }

        else
        {
            pGroupT = assert_cast<GROUPT*>(symbol);
        }

        if( pGroupT != nullptr )
        {
            for( int i = 0; i < pGroupT->GetNumItems(); i++ )
            {
                int iSymItem = pGroupT->GetItemSymbol(i);

                if( iSymItem <= 0 )
                    continue;

                Symbol* pSymbolOnGoup = &NPT_Ref(iSymItem);

                if( pSymbolOnGoup->IsA(SymbolType::Variable) )
                {
                    num_fields_processed += VariableWorker(symbol_table, (VART*)pSymbolOnGoup, variable_worker_function);
                }

                else if( pSymbolOnGoup->IsA(SymbolType::Group) )
                {
                    num_fields_processed += VariableWorker(symbol_table, (GROUPT*)pSymbolOnGoup, variable_worker_function);
                }
            }
        }
    }

    // a record
    else if( symbol->IsA(SymbolType::Section) )
    {
        SECT* pSecT = assert_cast<SECT*>(symbol);

        int iSymVar = pSecT->SYMTfvar;

        while( iSymVar > 0 )
        {
            VART* pVarT = VPT(iSymVar);
            num_fields_processed += VariableWorker(symbol_table, pVarT, variable_worker_function);
            iSymVar = pVarT->SYMTfwd;
        }
    }

    // a dictionary
    else if( symbol->IsA(SymbolType::Pre80Dictionary) )
    {
        DICT* pDicT = assert_cast<DICT*>(symbol);

        int iSymSec = pDicT->SYMTfsec;

        // process all of the fields
        while( iSymSec > 0 )
        {
            SECT* pSecT = SPT(iSymSec);
            num_fields_processed += VariableWorker(symbol_table, pSecT, variable_worker_function);
            iSymSec = pSecT->SYMTfwd;
        }
    }

    else
    {
        ASSERT(false);
    }

    return num_fields_processed;
}
