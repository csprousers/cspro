#pragma once

#include <zEngineO/Block.h>
#include <engine/Form2.h>
#include <engine/VarT.h>
#include <zIssaLib/GroupT.h>
#include <zIssaLib/SecT.h>


template<typename VariableFunction>
size_t ForeachVariable(const Logic::SymbolTable& symbol_table, Symbol& symbol, const VariableFunction& variable_worker_function)
{
    auto GetSymbolTable = [&]() -> const Logic::SymbolTable& { return symbol_table; };

    size_t variables_processed = 0;

    switch( symbol.GetType() )
    {
        // variable
        case SymbolType::Variable:
        {
            variables_processed += variable_worker_function(assert_cast<VART&>(symbol));
            break;
        }

        // block
        case SymbolType::Block:
        {
            const EngineBlock& engine_block = assert_cast<const EngineBlock&>(symbol);

            for( VART* const pVarT : engine_block.GetVarTs() )
                variables_processed += variable_worker_function(*pVarT);

            break;
        }

        // form or group
        case SymbolType::Form:
        case SymbolType::Group:
        {
            GROUPT* pGroupT;

            if( symbol.IsA(SymbolType::Form) )
            {
                FORM& form = assert_cast<FORM&>(symbol);
                const int iSymGroup = form.GetSymGroup();
                pGroupT = ( iSymGroup > 0 ) ? GPT_Positive(iSymGroup) : nullptr;
            }

            else
            {
                pGroupT = &assert_cast<GROUPT&>(symbol);
            }

            if( pGroupT != nullptr )
            {
                for( int i = 0; i < pGroupT->GetNumItems(); ++i )
                {
                    const int iSymItem = pGroupT->GetItemSymbol(i);

                    if( iSymItem <= 0 )
                        continue;

                    Symbol& symbol_on_group = NPT_Ref(iSymItem);

                    if( symbol_on_group.IsA(SymbolType::Variable) )
                    {
                        variables_processed += ForeachVariable(symbol_table, assert_cast<VART&>(symbol_on_group), variable_worker_function);
                    }

                    else if( symbol_on_group.IsA(SymbolType::Group) )
                    {
                        variables_processed += ForeachVariable(symbol_table, assert_cast<GROUPT&>(symbol_on_group), variable_worker_function);
                    }
                }
            }

            break;
        }

        // record
        case SymbolType::Section:
        {
            SECT& sect = assert_cast<SECT&>(symbol);

            int iSymVar = sect.SYMTfvar;

            while( iSymVar > 0 )
            {
                VART* const pVarT = VPT(iSymVar);
                variables_processed += ForeachVariable(symbol_table, *pVarT, variable_worker_function);
                iSymVar = pVarT->SYMTfwd;
            }

            break;
        }

        // dictionary
        case SymbolType::Pre80Dictionary:
        {
            DICT& dict = assert_cast<DICT&>(symbol);

            int iSymSec = dict.SYMTfsec;

            // process all of the fields
            while( iSymSec > 0 )
            {
                SECT* const pSecT = SPT(iSymSec);
                variables_processed += ForeachVariable(symbol_table, *pSecT, variable_worker_function);
                iSymSec = pSecT->SYMTfwd;
            }

            break;
        }

        // invalid
        default:
            ASSERT(false);
            break;
    }

    return variables_processed;
}
