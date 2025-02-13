#include "stdafx.h"
#include "IncludesCC.h"
#include "AllSymbols.h"


int LogicCompiler::CompileDestinationVariable(std::variant<DataType, const Symbol*> data_type_or_symbol)
{
    // returns a node that can be used for assignments to: variables, objects (Array, HashMap, List), named frequencies, and user-defined functions
    DataType data_type;
    const Symbol* symbol;

    if( std::holds_alternative<DataType>(data_type_or_symbol) )
    {
        if( Tkn != TOKVAR     && Tkn != TOKWORKSTRING && Tkn != TOKARRAY && Tkn != TOKFREQ &&
            Tkn != TOKHASHMAP && Tkn != TOKLIST       && Tkn != TOKUSERFUNCTION )
        {
            IssueError(MGF::Variable_destination_invalid_93011);
        }

        data_type = std::get<DataType>(data_type_or_symbol);
        symbol = &NPT_Ref(Tokstindex);

        // check that the type matches
        if( SymbolCalculator::GetDataType(*symbol) != data_type )
            IssueError(MGF::Variable_destination_not_correct_data_type_93012, ToString(data_type));
    }

    else
    {
        symbol = std::get<const Symbol*>(data_type_or_symbol);
        ASSERT(symbol != nullptr);
        data_type = SymbolCalculator::GetDataType(*symbol);
    }

    ASSERT(DataTypeIsOneOf(data_type, DataType::Numeric, DataType::String));


    // compile any potential indices
    int symbol_compilation = -1;

    // Array object
    if( symbol->IsA(SymbolType::Array) )
    {
        symbol_compilation = CompileLogicArrayReference();
    }


    // HashMap object
    else if( symbol->IsA(SymbolType::HashMap) )
    {
        symbol_compilation = CompileLogicHashMapReference();
    }


    // List object
    else if( symbol->IsA(SymbolType::List) )
    {
        const LogicList& logic_list = GetSymbolLogicList(Tokstindex);

        if( logic_list.IsReadOnly() )
            IssueError(MGF::List_read_only_cannot_be_modified_965, logic_list.GetName().c_str());

        symbol_compilation = CompileLogicListReference(&logic_list);
    }


    // named frequency
    else if( symbol->IsA(SymbolType::NamedFrequency) )
    {
        symbol_compilation = CompileNamedFrequencyReference();
    }


    // user-defined function
    else if( symbol->IsA(SymbolType::UserFunction) )
    {
        // assignments only allowed when in the function
        if( !IsCompiling(*symbol) )
            IssueError(MGF::Variable_destination_function_error_93013);

        NextToken();
    }


    // variable (non-working)
    else if( symbol->IsA(SymbolType::Variable) )
    {
        const VART* pVarT = assert_cast<const VART*>(symbol);
        const_cast<VART*>(pVarT)->SetUsed(true);

        // make sure that the assignment is not for a variable at a higher level
        if( pVarT->GetSubType() != SymbolSubType::External && pVarT->GetLevel() > GetCompilationLevelNumber_base1() )
            IssueError(MGF::variable_belongs_to_lower_level_93);

        // check that changing the ID is valid
        CheckIdChanger(pVarT);

        symbol_compilation = varsanal_COMPILER_DLL_TODO(pVarT->GetFmt());
    }


    // work string
    else if( symbol->IsA(SymbolType::WorkString) )
    {
        NextToken();
    }


    // work variable
    else if( symbol->IsA(SymbolType::WorkVariable) )
    {
        NextToken();
    }


    else
    {
        throw ProgrammingErrorException();
    }

    auto& symbol_value_node = CreateNode<Nodes::SymbolValue>();

    symbol_value_node.symbol_index = symbol->GetSymbolIndex();
    symbol_value_node.symbol_compilation = symbol_compilation;

    return GetProgramIndex(symbol_value_node);
}
