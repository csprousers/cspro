//----------------------------------------------------------------------
//
//  INTCHARS.cpp      interpreting char functions
//
//----------------------------------------------------------------------

#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "Engine.h"
#include "Ctab.h"
#include "ScopeChangeNodeIterator.h"
#include <zEngineO/AllSymbols.h>
#include <zEngineO/PffExecutor.h>
#include <zEngineO/Nodes/Strings.h>
#include <zEngineO/Nodes/UserInterface.h>
#include <zEngineO/Nodes/Various.h>
#include <zToolsO/Tools.h>
#include <zUtilO/TransactionManager.h>
#include <zDictO/DDClass.h>
#include <zDictO/ValueProcessor.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>
#include <ZBRIDGEO/npff.h>
#include <Zissalib/CsDriver.h>
#include <zParadataO/Logger.h>
#include <zEngineF/EngineUI.h>


//----------------------------------------------------------------------
//  exedit: execute EDIT function
//----------------------------------------------------------------------
namespace
{
    struct PAT_DESC
    {
        int len;
        int num;
        int dec;
        TCHAR pad;
        int sig;
    };

    bool exedit_scan(CString pattern, PAT_DESC* pat_desc)
    {
        constexpr char DecimalSeparator = '.';

        pat_desc->len = 0;
        pat_desc->num = 0;
        pat_desc->dec = 0;
        pat_desc->pad = BLANK;
        pat_desc->sig = 9999;

        const TCHAR* pattern_itr = pattern.GetBuffer();
        bool bOnly9 = false;

        if( *pattern_itr != '9' && *pattern_itr != 'Z' && *pattern_itr != 0 )
        {
            pat_desc->len = 1;
            pat_desc->pad = *pattern_itr++;
        }

        for( ;  *pattern_itr != 0 && *pattern_itr != DecimalSeparator; pattern_itr++ )
        {
            if( *pattern_itr == '9' )
            {
                if( !bOnly9 )
                    pat_desc->sig = ( pattern_itr - pattern );

                bOnly9 = true;
                pat_desc->num++;
            }

            else if( *pattern_itr == 'Z' )
            {
                if( bOnly9 )
                    return false;

                pat_desc->num++;
            }

            pat_desc->len++;
        }

        if( *pattern_itr == DecimalSeparator )
        {
            pat_desc->len++;
            pattern_itr++;

            while( *pattern_itr )
            {
                if( *pattern_itr == '9' )
                {
                    bOnly9 = true;
                    pat_desc->dec++;
                }

                else if( *pattern_itr == 'Z' )
                {
                    if( bOnly9 )
                        return false;

                    pat_desc->dec++;
                }

                pat_desc->len++;
                pattern_itr++;
            }
        }

        pat_desc->num += pat_desc->dec;

        return true;
    }
}


double CIntDriver::exedit(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    CString pattern = EvalAlphaExprCS(va_node.arguments[0]);
    double value = evalexpr(va_node.arguments[1]);
    CString edit_result;

    // process normal values
    if( value > -MAXVALUE && !IsSpecial(value) )
    {
        PAT_DESC pat_desc;

        // check that the pattern is valid
        if( exedit_scan(pattern, &pat_desc) )
        {
            TCHAR* chvalue = edit_result.GetBufferSetLength(pattern.GetLength());

            bool value_is_negative = ( value < 0 );

            if( value_is_negative )
                value = -value;

            if( pat_desc.dec > 0 )
                value *= Power10[pat_desc.dec];

            value = floor(value + MAGICROUND);

            CString formattedValue;
            formattedValue.Format(_T("%.0f"), value);
            int len = formattedValue.GetLength();
            const TCHAR* pv = formattedValue.GetBuffer() + len - 1;

            _tmemset(chvalue, pat_desc.pad, pat_desc.len);

            const TCHAR* pp = pattern.GetBuffer() + pat_desc.len - 1;
            TCHAR* pr = chvalue + pat_desc.len - 1;

            int i = 0;

            for( ; i < pat_desc.len; i++ )
            {
                if( *pp == '9' )
                {
                    if( len > 0 )
                    {
                        *pr-- = *pv--;
                        len--;
                    }

                    else
                    {
                        *pr-- = '0';
                    }
                }

                else if( *pp == 'Z' )
                {
                    if( len > 0 )
                    {
                        *pr-- = *pv--;
                        len--;
                    }

                    else
                    {
                        break;
                    }
                }

                else
                {
                    if( len > 0 || ( pp - pattern ) >= pat_desc.sig )
                        *pr-- = *pp;
                }

                pp--;
            }

            if( value_is_negative )
            {
                if( i < pat_desc.len )
                {
                    *pr = '-';
                }

                else
                {
                    *(++pr) = '-';
                }
            }
        }
    }

    // process special values
    else if( IsSpecial(value) )
    {
        edit_result = UTF8_TODO::GetCString(SpecialValues::ValueToString(value));
        SO::MakeExactLength(edit_result, pattern.GetLength());
    }

    return AssignAlphaValue(edit_result);
}


//----------------------------------------------------------------------
//  extavar:  execute table alpha var
//----------------------------------------------------------------------
double CIntDriver::extavar(int iExpr)
{
    CString value;

#ifdef WIN_DESKTOP
    const TVAR_NODE* pTableNode = (TVAR_NODE*)PPT(iExpr);
    CTAB* pCtab = XPT( pTableNode->tvar_index );
    int subindex[3];

    for( int i = 0; i < 3; i++ )
    {
        subindex[i] = 0;
        if( pTableNode->tvar_exprindex[i] >= 0 )
            subindex[i] = Evaluate<int>(pTableNode->tvar_exprindex[i]);
    }

    const TCHAR* pBuf = (TCHAR*)pCtab->m_pAcum.GetValue( subindex[0], subindex[1], subindex[2] );

    if( pBuf != nullptr )
    {
        int len = pCtab->GetAcumType() / sizeof(TCHAR);
        value = CString(pBuf, len);
    }

#else
    // crosstabs don't exist in the portable environments
    ASSERT(false);

#endif

    return AssignAlphaValue(value);
}


//----------------------------------------------------------------------
//  exavar:  execute alpha var
//----------------------------------------------------------------------
double CIntDriver::exavar(int iEpxr)
{
    const SVAR_NODE* pSVAR = (SVAR_NODE*)PPT(iEpxr);
    const MVAR_NODE* pMVAR = (MVAR_NODE*)PPT(iEpxr);
    CString csValue;

    if( pSVAR->m_iVarType == SVAR_CODE )
    {
        VART* pVarT = VPT(pSVAR->m_iVarIndex);
        VARX* const pVarX = pVarT->GetVarX();
        csValue = CString((LPCTSTR)svaraddr(pVarX), pVarT->GetLength());
    }

    else if( pMVAR->m_iVarType == MVAR_CODE )
    {
        double subindex[DIM_MAXDIM];
        mvarGetSubindexes(pMVAR, subindex);

        VART* pVarT = VPT(pSVAR->m_iVarIndex);
        VARX* pVarX = pVarT->GetVarX();

        const TCHAR* variable_address = (TCHAR*)mvaraddr(pVarX, subindex);

        if( variable_address != nullptr )
        {
            csValue = CString(variable_address, pVarT->GetLength());
        }

        else
        {
            csValue = CString(_T(' '), pVarT->GetLength());
        }
    }

    return AssignAlphaValue(csValue);
}


//----------------------------------------------------------------------
//  excharobj : executes alpha object
//----------------------------------------------------------------------
double CIntDriver::excharobj(int program_index)
{
    const auto& string_expression_node = GetNode<Nodes::StringExpression>(program_index);
    std::wstring text;

    if( string_expression_node.string_expression >= 0 &&
        m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        text = EvalAlphaExpr(string_expression_node.string_expression);
    }

    else
    {
        int abs_string_expression = std::abs(string_expression_node.string_expression);
        FunctionCode string_expression_function_code = GetNode<FunctionCode>(abs_string_expression);

        if( string_expression_function_code == FunctionCode::SVAR_CODE || string_expression_function_code == FunctionCode::MVAR_CODE )
        {
            text = UTF8_TODO::GetWide(*GetWorkingSharableString(static_cast<size_t>(exavar(abs_string_expression))));
        }

        else
        {
            ASSERT(m_engineData->PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1));

            if( string_expression_function_code == FunctionCode::WORKSTRING_CODE && string_expression_node.substring_index_expression == -1 ) // UTF8_TODO here until all the objects return SharableStrings
                return AssignString(GetSymbolWorkString(GetNode<int>(abs_string_expression + 1)).GetSharableString());

            text = ( string_expression_function_code == FunctionCode::WORKSTRING_CODE ) ? UTF8_TODO::GetWide(GetSymbolWorkString(GetNode<int>(abs_string_expression + 1)).GetString()) :
                   ( string_expression_function_code == FunctionCode::TVAR_CODE )       ? UTF8_TODO::GetWide(*GetWorkingSharableString(static_cast<size_t>(extavar(abs_string_expression)))) :
                                                                                          EvalAlphaExpr(abs_string_expression);
        }
    }


    // done if no substring values are present
    if( string_expression_node.substring_index_expression == -1 )
        return AssignAlphaValue(std::move(text));


    // parse the substring values
    int text_length = text.length();
    int starting_position = Evaluate<int>(string_expression_node.substring_index_expression);

    if( starting_position < 0 )
    {
        // A negative start position means index from right end of string.
        // This allows code like "foobar"[-3] which results in "bar"
        starting_position = text_length + starting_position + 1;
    }

    // return a blank string if the substring values are not valid
    if( --starting_position < 0 || ( starting_position >= text_length && starting_position > 0 ) )
        return AssignStringNull();

    int length;

    if( string_expression_node.substring_length_expression != -1 )
    {
        length = Evaluate<int>(string_expression_node.substring_length_expression);
        length = std::max(0, length); // negative lengths are invalid
    }

    else
    {
        length = text_length;
    }

    if( starting_position + length > text_length )
        length = text_length - starting_position;

    return AssignAlphaValue(text.substr(starting_position, length));
}


double CIntDriver::exstringcompute(int program_index)
{
    // for assigning string expressions to strings, arrays, user-defined functions, and variables
    const Nodes::StringCompute* string_compute_node;
    const Nodes::SymbolValue* symbol_value_node;
    std::unique_ptr<std::tuple<Nodes::StringCompute, Nodes::SymbolValue>> simulated_nodes_for_pre80_pen_file;

    if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        string_compute_node = &GetNode<Nodes::StringCompute>(program_index);
        symbol_value_node = &GetNode<Nodes::SymbolValue>(string_compute_node->symbol_value_node_index);
    }

    else
    {
        // convert pre-8.0 nodes
        enum class MoveType : int { Variable = 1, LogicArray, UserFunction, CrossTab, WorkString };
        struct MOVE_NODE
        {
            int st_code;
            int next_st;
            MoveType move_type;
            int move_expr;
            int ssipos;
            int sslen;
            int char_obj;
        };

        const auto& move_node = GetNode<MOVE_NODE>(program_index);

        simulated_nodes_for_pre80_pen_file = std::make_unique<std::tuple<Nodes::StringCompute, Nodes::SymbolValue>>();
        Nodes::StringCompute& simulated_string_compute_node = std::get<0>(*simulated_nodes_for_pre80_pen_file);
        Nodes::SymbolValue& simulated_symbol_value_node = std::get<1>(*simulated_nodes_for_pre80_pen_file);
        string_compute_node = &simulated_string_compute_node;
        symbol_value_node = &simulated_symbol_value_node;

        simulated_string_compute_node.substring_index_expression = move_node.ssipos;
        simulated_string_compute_node.substring_length_expression = move_node.sslen;
        simulated_string_compute_node.string_expression = move_node.char_obj;

        switch( move_node.move_type )
        {
            case MoveType::LogicArray:
                simulated_symbol_value_node.symbol_index = GetNode<Nodes::ElementReference>(move_node.move_expr).symbol_index;
                break;

            case MoveType::UserFunction:
            case MoveType::Variable:
                simulated_symbol_value_node.symbol_index = GetNode<SVAR_NODE>(move_node.move_expr).m_iVarIndex;
                break;

            case MoveType::WorkString:
                simulated_symbol_value_node.symbol_index = move_node.move_expr;
                break;

            default:
                ASSERT(false);
        }

        simulated_symbol_value_node.symbol_compilation = move_node.move_expr;
    };

    // evaluate the value to be assigned
    std::wstring rhs_value = EvalAlphaExpr(string_compute_node->string_expression);

    // if there are no subscripts used, we can set the value directly
    if( string_compute_node->substring_index_expression == -1 )
    {
        AssignValueToSymbol(*symbol_value_node, SharableString(UTF8_TODO::GetUtf8(std::move(rhs_value))));
    }

    // otherwise get the variable's current value and apply the new value on top of it
    else
    {
        ModifySymbolValue<SharableString>(*symbol_value_node,
            [&](SharableString& temp_lhs_value)
            {
                std::wstring lhs_value = UTF8_TODO::GetWide(*temp_lhs_value);
                int starting_position = Evaluate<int>(string_compute_node->substring_index_expression) - 1;

                // return if the starting position is invalid
                if( starting_position < 0 )
                    return;

                int rhs_chars_to_copy = rhs_value.length();
                int chars_to_copy;

                // if no length is specified, copy the the entire RHS string
                if( string_compute_node->substring_length_expression == -1 )
                {
                    chars_to_copy = rhs_chars_to_copy;
                }

                // otherwise copy the number of characters requested
                else
                {
                    chars_to_copy = Evaluate<int>(string_compute_node->substring_length_expression);

                    // return if nothing to copy
                    if( chars_to_copy <= 0 )
                        return;

                    rhs_chars_to_copy = std::min(chars_to_copy, rhs_chars_to_copy);
                }

                // increase the LHS string length as necessary
                int max_string_length = starting_position + chars_to_copy;

                if( max_string_length > static_cast<int>(lhs_value.length()) )
                    lhs_value.resize(max_string_length, ' ');

                // copy all of some of the RHS string
                wchar_t* const lhs_value_starting_position = lhs_value.data() + starting_position;
                _tmemcpy(lhs_value_starting_position, rhs_value.c_str(), rhs_chars_to_copy);

                // if more characters were requested to copy than exist in the RHS string, pad the LHS string with spaces
                if( chars_to_copy > rhs_chars_to_copy )
                    _tmemset(lhs_value_starting_position + rhs_chars_to_copy, ' ', chars_to_copy - rhs_chars_to_copy);

                temp_lhs_value = UTF8_TODO::GetUtf8(lhs_value);
            });
    }

    return 0;
}


Symbol& CIntDriver::GetSymbolFromSymbolName(const std::string_view symbol_name_sv, SymbolType preferred_symbol_type/* = SymbolType::None*/)
{
    try
    {
        return m_symbolTable.FindSymbolWithDotNotation(symbol_name_sv, preferred_symbol_type);
    }

    catch( const Logic::SymbolTable::Exception& )
    {
        // an exception will be thrown if the symbol is not found; in that case, search any symbols that might have been declared locally
        // (this functionality could be moved to the SymbolTable methods, but this is a rare instance so now it will only be done here)
        const size_t dot_index = symbol_name_sv.find('.');
        const std::string_view base_symbol_name_sv = ( dot_index != std::string_view::npos ) ? symbol_name_sv.substr(0, dot_index) :
                                                                                               symbol_name_sv;
        Symbol* symbol = nullptr;

        IterateOverScopeChangeNodes(
            [&](const Nodes::ScopeChange& scope_change_node)
            {
                const Nodes::List& local_symbol_indices_node = GetListNode(scope_change_node.local_symbol_indices_list);

                // first check the current, replaced, symbols
                for( int i = 0; i < local_symbol_indices_node.number_elements; ++i )
                {
                    Symbol& this_symbol = NPT_Ref(local_symbol_indices_node.elements[i]);

                    if( SO::EqualsNoCase(base_symbol_name_sv, this_symbol.GetName()) )
                    {
                        symbol = &this_symbol;
                        return false;
                    }
                }

                // if not found, check the original symbol names; this will allow this to work, e.g., CS.Logic.getSymbol(name := "name_of_function_parameter")
                if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_4) &&
                    scope_change_node.local_symbol_names_list != -1)
                {
                    const Nodes::List& local_symbol_names_node = GetListNode(scope_change_node.local_symbol_names_list);
                    ASSERT(local_symbol_names_node.number_elements == local_symbol_indices_node.number_elements);

                    for( int i = 0; i < local_symbol_names_node.number_elements; ++i )
                    {
                        if( SO::EqualsNoCase(base_symbol_name_sv, *m_engineData->string_literals[local_symbol_names_node.elements[i]]) )
                        {
                            symbol = &NPT_Ref(local_symbol_indices_node.elements[i]);
                            return false;
                        }
                    }
                }

                return true;
            });

        if( symbol == nullptr )
            throw;

        // if the starting symbol was found, process any dot notation
        if( dot_index != std::string_view::npos )
        {
            try
            {
                for( const std::string_view name_sv : SO::SplitString<std::string_view>(symbol_name_sv.substr(dot_index + 1), '.') )
                    symbol = &m_symbolTable.FindSymbol(name_sv, symbol);
            }

            catch( const Logic::SymbolTable::NoSymbolsException& )
            {
                // throw an exception with the full symbol name
                throw Logic::SymbolTable::NoSymbolsException(std::string(symbol_name_sv));
            }
        }

        return *symbol;
    }
}


std::tuple<Symbol*, Symbol*> CIntDriver::GetEvaluatedSymbolFromSymbolName(const std::string& symbol_name_and_potential_subscript, const SymbolType preferred_symbol_type/* = SymbolType::None*/)
{
    std::string_view symbol_name_sv = symbol_name_and_potential_subscript;
    const size_t left_parenthesis_pos = symbol_name_and_potential_subscript.find('(');
    const bool explicit_subscript_specified = ( left_parenthesis_pos != std::string::npos );

    if( explicit_subscript_specified )
        symbol_name_sv = symbol_name_sv.substr(0, left_parenthesis_pos);

    Symbol& base_symbol = GetSymbolFromSymbolName(SO::Trim(symbol_name_sv));
    Symbol* wrapped_symbol = nullptr;

    if( explicit_subscript_specified && !base_symbol.IsA(SymbolType::Item) )
    {
        throw CSProException("A subscript cannot be provided for the symbol '%s' of type '%s'.",
                             base_symbol.GetName().c_str(), ToString(base_symbol.GetType()));
    }

    if( base_symbol.IsA(SymbolType::Item) )
    {
        EngineItem& engine_item = assert_cast<EngineItem&>(base_symbol);
        const char* const subscript_text = explicit_subscript_specified ? ( symbol_name_and_potential_subscript.c_str() + left_parenthesis_pos ) :
                                                                          nullptr;

        wrapped_symbol = &GetWrappedEngineItemSymbol(engine_item, subscript_text);
    }

    return std::make_tuple(&base_symbol, wrapped_symbol);
}


double CIntDriver::exgetlabel(int iExpr)
{
    const auto& fng_node = GetNode<FNG_NODE>(iExpr);
    int symbol_index = fng_node.symbol_index;

    // makes sure that getsymbol works when called from the userbar
    if( fng_node.m_iFunCode == FNGETSYMBOL_CODE && symbol_index == 0 && m_FieldSymbol != 0 )
        symbol_index = m_FieldSymbol;

    ASSERT(symbol_index != 0);

    // use the current symbol if none was supplied
    if( symbol_index == -1 )
    {
        if( m_iExSymbol <= 0 )
            return AssignStringNull();

        symbol_index = m_iExSymbol;
    }

    else if( symbol_index == 2147483647/*INT_MAX*/ )
    {
        // get the name of a partial save field
        const Case& data_case = DIX(0)->GetCase();

        if( data_case.GetPartialSaveCaseItemReference() == nullptr )
        {
            return AssignStringNull();
        }

        else
        {
            const CaseItemReference& partial_save_case_item_reference = *data_case.GetPartialSaveCaseItemReference();

            // return the name along with the occurrences
            return AssignString(partial_save_case_item_reference.GetName() +
                                partial_save_case_item_reference.GetItemIndexHelper().GetMinimalOccurrencesText(partial_save_case_item_reference));
        }
    }


    const Symbol* const symbol = NPT(symbol_index);

    // getsymbol: evaluate the symbol
    if( fng_node.m_iFunCode == FunctionCode::FNGETSYMBOL_CODE )
        return AssignString(symbol->GetName());


    // getlabel: evaluate the label

    // only 1 parameter was used in the function
    if( fng_node.m_iExpr == -1 )
    {
        return AssignString(SymbolCalculator::GetLabel(*symbol));
    }

    // otherwise 2 parameters were used, which means that we need
    // to search for a code or label in the value set
    else
    {
        ASSERT(symbol->IsOneOf(SymbolType::Variable, SymbolType::ValueSet));
        const ValueProcessor* value_processor;
        CString label;

        if( symbol->IsA(SymbolType::ValueSet) )
        {
            const ValueSet* value_set = assert_cast<const ValueSet*>(symbol);
            value_processor = &value_set->GetValueProcessor();
        }

        else
        {
            const VART* pVarT = assert_cast<const VART*>(symbol);
            value_processor = &pVarT->GetCurrentValueProcessor();
        }

        if( fng_node.m_iOper == (int)GetLabelSearchType::ByCode )
        {
            const DictValue* dict_value = nullptr;

            if( IsNumeric(*symbol) )
            {
                dict_value = value_processor->GetDictValue(evalexpr(fng_node.m_iExpr));
            }

            else
            {
                dict_value = value_processor->GetDictValue(EvalAlphaExprCS(fng_node.m_iExpr));
            }

            if( dict_value != nullptr )
                label = dict_value->GetLabel();
        }

        else
        {
            const DictValue* dict_value = value_processor->GetDictValueByLabel(EvalAlphaExprCS(fng_node.m_iExpr));

            // take the label from the first value pair
            if( dict_value != nullptr && dict_value->HasValuePairs() )
                label = dict_value->GetValuePair(0).GetFrom();
        }

        return AssignAlphaValue(label);
    }
}


double CIntDriver::exgetbuffer(int iExpr)
{
    const FNC_NODE* pFngNode = (FNC_NODE*)PPT(iExpr);
    SVAR_NODE* pSVAR = (SVAR_NODE*)PPT(pFngNode->isymb);
    MVAR_NODE* pMVAR = (MVAR_NODE*)PPT(pFngNode->isymb);
    CString csValue;

    if( pSVAR->m_iVarType == SVAR_CODE )
    {
        VART* const pVarT = VPT(pSVAR->m_iVarIndex);
        csValue = CString(pVarT->GetAsciiValue(0), pVarT->GetLength());
    }

    else if( pMVAR->m_iVarType == MVAR_CODE )
    {
        double subindex[DIM_MAXDIM];
        mvarGetSubindexes(pMVAR, subindex);

        VART* pVarT = VPT(pMVAR->m_iVarIndex);

        csValue = CString((LPCTSTR)pVarT->GetAsciiValue((int)subindex[0]), pVarT->GetLength());
    }

    else
    {
        ASSERT(0);
    }

    return AssignAlphaValue(csValue);
}


// this function returns a null NamedReference (and issues a warning) if there is a problem evaluating the field reference
std::tuple<std::shared_ptr<NamedReference>, int> CIntDriver::EvaluateNoteReference(const FNNOTE_NODE& note_node)
{
    std::shared_ptr<NamedReference> named_reference;
    int field_symbol = -1;

    // figure out what to evaluate
    int evaluate_simple_note_symbol_index = -1;
    bool evaluate_current_field = false;
    bool evaluate_passed_field = false;
    bool check_if_data_is_accessible = true;

    // no field argument so use the current field
    if( note_node.symbol_index < 0 )
    {
        if( m_iExSymbol > 0 )
        {
            const Symbol& symbol = NPT_Ref(m_iExSymbol);

            // a level
            if( symbol.IsA(SymbolType::Group) && SymbolCalculator::GetLevelNumber_base1(symbol) > 0 )
            {
                evaluate_simple_note_symbol_index = m_iExSymbol;
            }

            // a field
            else if( symbol.IsA(SymbolType::Variable) )
            {
                evaluate_current_field = true;
            }
        }
    }

    // a dictionary, level, or record
    else if( note_node.variable_expression < 0 )
    {
        evaluate_simple_note_symbol_index = note_node.symbol_index;
    }

    // a field
    else
    {
        evaluate_passed_field = true;
    }


    // do the evaluations
    if( evaluate_simple_note_symbol_index >= 0 )
    {
        const Symbol& symbol = NPT_Ref(evaluate_simple_note_symbol_index);
        field_symbol = evaluate_simple_note_symbol_index;
        named_reference = std::make_unique<NamedReference>(SymbolCalculator::GetBaseName(symbol), std::string());
        check_if_data_is_accessible = false;
    }

    else if( evaluate_current_field )
    {
        if( Issamod == ModuleType::Entry )
        {
            const DEFLD* defld = m_pCsDriver->GetCurDeFld();

            if( defld != nullptr )
                m_pEngineDriver->GetNamedReferenceFromField(*defld, named_reference, field_symbol);
        }

        else
        {
            field_symbol = m_iExSymbol;

            VART* pVarT = VPT(m_iExSymbol);

            CNDIndexes theCurrentIndexes(ONE_BASED);
            GetCurrentVarSubIndexes(m_iExSymbol, theCurrentIndexes);

            auto case_item_reference = std::make_unique<CaseItemReference>(*pVarT->GetCaseItem(), std::string());
            ConvertIndex(theCurrentIndexes, *case_item_reference);

            named_reference = std::move(case_item_reference);
        }
    }

    else if( evaluate_passed_field )
    {
        auto pMVAR = (const MVAR_NODE*)PPT(note_node.variable_expression);
        field_symbol = pMVAR->m_iVarIndex;

        VART* pVarT = VPT(field_symbol);

        auto case_item_reference = std::make_unique<CaseItemReference>(*pVarT->GetCaseItem(), std::string());

        if( pMVAR->m_iVarType == MVAR_CODE )
        {
            UserIndexesArray dIndex;
            mvarGetSubindexes(pMVAR, dIndex);

            C3DObject the3dObject;
            CsDriver::PassTo3D(&the3dObject, NPT(field_symbol), dIndex);

            ConvertIndex(the3dObject, *case_item_reference);
        }

        named_reference = std::move(case_item_reference);
    }


    bool success;

    // no valid entity found
    if( named_reference == nullptr )
    {
        issaerror(MessageType::Error, 46501);
        success = false;
    }

    // verify that the note data is accessible
    else if( check_if_data_is_accessible )
    {
        success = IsDataAccessible(NPT_Ref(field_symbol), true);
    }

    // otherwise the data accessibility checks will have been done at compile-time
    else
    {
        ASSERT(IsDataAccessible(NPT_Ref(field_symbol), true));
        success = true;
    }

    if( success )
        return std::make_tuple(std::move(named_reference), field_symbol);

    return { nullptr, -1 };
}


std::unique_ptr<std::string> CIntDriver::EvaluateNoteOperatorId(const FNNOTE_NODE& note_node, const int field_symbol)
{
    // operator IDs will be ignored for case notes
    if( !NPT_Ref(field_symbol).IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary) )
    {
        if( note_node.operator_id_expression != -1 )
        {
            return std::make_unique<std::string>(EvaluateString(note_node.operator_id_expression));
        }

        else if( Issamod == ModuleType::Entry )
        {
            return std::make_unique<std::string>(UTF8_TODO::GetUtf8(assert_cast<CEntryDriver*>(m_pEngineDriver)->GetOperatorId()));
        }
    }

    return nullptr;
}


double CIntDriver::exgetnote(const int program_index)
{
    const auto& note_node = GetNode<FNNOTE_NODE>(program_index);
    const auto [named_reference, field_symbol] = EvaluateNoteReference(note_node);

    if( named_reference == nullptr )
        return AssignStringNull();

    const std::unique_ptr<const std::string> operator_id = EvaluateNoteOperatorId(note_node, field_symbol);

    return AssignString(m_pEngineDriver->GetNoteContent(*named_reference, operator_id.get(), field_symbol));
}


double CIntDriver::exputnote(const int program_index)
{
    const auto& note_node = GetNode<FNNOTE_NODE>(program_index);
    const auto [named_reference, field_symbol] =  EvaluateNoteReference(note_node);

    if( named_reference == nullptr )
        return 0;

    const std::unique_ptr<const std::string> operator_id = EvaluateNoteOperatorId(note_node, field_symbol);

    m_pEngineDriver->SetNote(named_reference, operator_id.get(),
                             EvaluateSharableString(note_node.note_text_expression),
                             field_symbol);

    return 1;
}


double CIntDriver::exeditnote(const int program_index)
{
    const auto& note_node = GetNode<FNNOTE_NODE>(program_index);
    const auto [named_reference, field_symbol] =  EvaluateNoteReference(note_node);

    if( named_reference == nullptr )
        return AssignStringNull();

    const std::unique_ptr<const std::string> operator_id = EvaluateNoteOperatorId(note_node, field_symbol);

    return AssignString(std::get<SharableString>(m_pEngineDriver->EditNote(named_reference, operator_id.get(), field_symbol, false)));
}


double CIntDriver::exgetoperatorid(int iExpr)
{
    CString operator_id = ( Issamod == ModuleType::Entry ) ? assert_cast<CEntryDriver*>(m_pEngineDriver)->GetOperatorId() : CString();
    return AssignAlphaValue(operator_id);
}


double CIntDriver::exsetoperatorid(int iExpr)
{
    if( Issamod != ModuleType::Entry )
        return 0;

    const FNN_NODE* pFun = (FNN_NODE*)PPT(iExpr);
    CString operator_id = EvalAlphaExprCS(pFun->fn_expr[0]);

    // the maximum length of an operator ID is 32 characters
    constexpr int MaximumOperatorIdLength = 32;
    if( operator_id.GetLength() > MaximumOperatorIdLength )
        operator_id.Truncate(MaximumOperatorIdLength);

    assert_cast<CEntryDriver*>(m_pEngineDriver)->SetOperatorId(operator_id);

    return 1;
}


// 20140228 this function simply deletes the memory associated with a function that returns an alpha
// (in the past it wasn't possible to call a function like editnote() without having to assign the return
// value to something, which seemed a bit silly)
double CIntDriver::exfreealphamem(const int program_index)
{
    const auto& fnc_node = GetNode<FNC_NODE>(program_index);
    EvalAlphaExpr(fnc_node.isymb);
    return 1; // in case this is being used as a conditional, always return true
}



//----------------------------------------------------------------------
//  ExExecSystem: execute EXECSYSTEM function
//----------------------------------------------------------------------
double CIntDriver::ExExecSystem(int iExpr)
{
    const auto& execsystem_node = GetNode<FNEXECSYSTEM_NODE>(iExpr);
    bool success = false;
    std::string command = EvaluateString(execsystem_node.m_iCommand);

    std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event = ExExecCommonBeforeExecute(FNEXECSYSTEM_CODE, command, execsystem_node.m_iOptions);

#ifdef WIN_DESKTOP
    success = ExExecCommonExecute(command, execsystem_node.m_iOptions);

#else
    const bool wait = ( ( execsystem_node.m_iOptions & EXECSYSTEM_WAIT ) != 0 );

    // for a couple actions, fully evaluate the path before passing the file paths to Android functions
    const size_t colon_pos = command.find(':');

    if( colon_pos != std::wstring::npos )
    {
        constexpr const char* ActionsToFullyEvaluatePath[] = { "camera", "signature", "view" };
        const std::string_view command_sv = command;
        const std::string_view action_sv = command_sv.substr(0, colon_pos);

        for( size_t i = 0; i < _countof(ActionsToFullyEvaluatePath); ++i )
        {
            if( SO::EqualsNoCase(action_sv, ActionsToFullyEvaluatePath[i]) )
            {
                std::string file_path(SO::Trim(command_sv.substr(colon_pos + 1)));
                MakeAbsolutePath(file_path);
                command = SO::Concatenate(action_sv, ":", file_path);
                break;
            }
        }
    }

    success = PlatformInterface::GetInstance()->GetApplicationInterface()->ExecSystem(command, wait);

#endif

    return ExExecCommonAfterExecute(FNEXECSYSTEM_CODE, execsystem_node.m_iOptions, success, std::move(external_application_event));
}


double CIntDriver::ExExecPFF(int iExpr) // 20100601
{
    const auto& execsystem_node = GetNode<FNEXECSYSTEM_NODE>(iExpr);

    if( execsystem_node.m_iCommand < 0 )
    {
        LogicPff& logic_pff = GetSymbolLogicPff(-1 * execsystem_node.m_iCommand);
        return ExExecPFF(&logic_pff, execsystem_node.m_iOptions);
    }

    else
    {
        return ExExecPFF(EvaluatePath(execsystem_node.m_iCommand), execsystem_node.m_iOptions);
    }
}


double CIntDriver::ExExecPFF(std::variant<LogicPff*, std::string> logic_pff_or_pff_file_path, std::optional<int> flags/* = std::nullopt*/)
{
    LogicPff* logic_pff = nullptr;
    std::shared_ptr<const PFF> pff;
    std::shared_ptr<PffExecutor> pff_executor;
    bool success = true;

    if( std::holds_alternative<LogicPff*>(logic_pff_or_pff_file_path) )
    {
        logic_pff = std::get<LogicPff*>(logic_pff_or_pff_file_path);
        pff = logic_pff->GetSharedPff();
        pff_executor = logic_pff->GetSharedPffExecutor();

        // when called from pff.exec, set the default flags, which will run entry
        // applications after stopping and anything else as wait
        if( !flags.has_value() )
            flags = EXECSYSTEM_DEFAULT_OPTIONS | ( ( pff->GetAppType() == APPTYPE::ENTRY_TYPE ) ? EXECSYSTEM_STOP : EXECSYSTEM_WAIT );
    }

    else
    {
        // load the PFF
        auto loaded_pff = std::make_unique<PFF>();
        loaded_pff->SetPifFileName(UTF8_TODO::GetCString(std::get<std::string>(logic_pff_or_pff_file_path)));
        success = loaded_pff->LoadPifFile(true);
        pff = std::move(loaded_pff);
    }

    ASSERT(pff != nullptr && flags.has_value());

    // when a PFF is launched in wait mode try to use the PFF executor
    const bool use_pff_executor = ( ( *flags & EXECSYSTEM_WAIT ) != 0 &&
                                    PffExecutor::CanExecute(pff->GetAppType()) );

    std::string pff_file_path_or_name;

    if( logic_pff == nullptr || !logic_pff->IsModified() )
    {
        pff_file_path_or_name = UTF8_TODO::GetUtf8(pff->GetPifFileName());
    }

    else if( !use_pff_executor )
    {
        pff_file_path_or_name = logic_pff->GetRunnableFilePath();
    }

    else
    {
        pff_file_path_or_name = logic_pff->GetName();
    }

    std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event = ExExecCommonBeforeExecute(FNEXECPFF_CODE, pff_file_path_or_name, *flags);

    if( success )
    {
        if( use_pff_executor )
        {
            try
            {
                if( pff_executor == nullptr )
                    pff_executor = std::make_unique<PffExecutor>();

                EngineUI::RunPffExecutorNode run_pff_executor_node
                {
                    *pff,
                    pff_executor,
                    std::exception_ptr()
                };

                success = ( SendEngineUIMessage(EngineUI::Type::RunPffExecutor, run_pff_executor_node) != 0 );

                if( run_pff_executor_node.thrown_exception )
                    std::rethrow_exception(run_pff_executor_node.thrown_exception);
            }

            catch( const CSProException& exception )
            {
                issaerror(MessageType::Error, 47195, Path::GetFilename(pff_file_path_or_name).c_str(), exception.what());
                success = false;
            }
        }

        // otherwise launch the application using the executable (as in execsystem)
        else
        {
#ifdef WIN_DESKTOP
            const std::optional<std::string> exe_filename = pff->GetExecutableProgram();

            success = exe_filename.has_value() &&
                      ExExecCommonExecute(FormatText("%s \"%s\"", exe_filename->c_str(), pff_file_path_or_name.c_str()), *flags);
#else
            if( pff->GetAppType() == ENTRY_TYPE || PffExecutor::CanExecute(pff->GetAppType()) )
            {
                // 20140213 a temporary kludge ... we'll set a parameter concerning the next application to run, which will be run when this application ends
                success = PlatformInterface::GetInstance()->GetApplicationInterface()->ExecPff(pff_file_path_or_name);
            }

            else
            {
                success = false;
            }
#endif
        }
    }

    return ExExecCommonAfterExecute(FNEXECPFF_CODE, *flags, success, std::move(external_application_event));
}


std::unique_ptr<Paradata::ExternalApplicationEvent> CIntDriver::ExExecCommonBeforeExecute(const FunctionCode source, const std::string& command, const int flags)
{
    try
    {
        TransactionManager::CommitTransactions();
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10104, exception.what());
    }

    if( !Paradata::Logger::IsOpen() )
        return nullptr;

    const auto ext_app_source = ( source == FNEXECSYSTEM_CODE ) ? Paradata::ExternalApplicationEvent::Source::ExecSystem :
                                                                  Paradata::ExternalApplicationEvent::Source::ExecPff;

    return std::make_unique<Paradata::ExternalApplicationEvent>(ext_app_source,
                                                                command,
                                                                ( ( flags & EXECSYSTEM_STOP ) != 0 ));
}


#ifdef WIN_DESKTOP

bool CIntDriver::ExExecCommonExecute(const std::string& command, const int flags)
{
    const int show_window = ( ( flags & EXECSYSTEM_MAXIMIZED ) != 0 ) ? SW_MAXIMIZE :
                            ( ( flags & EXECSYSTEM_MINIMIZED ) != 0 ) ? SW_MINIMIZE :
                            ( ( flags & EXECSYSTEM_NORMAL ) != 0 )    ? SW_SHOWNA :
                                                                        SW_SHOWNA;

    const bool focus = ( ( flags & EXECSYSTEM_FOCUS ) != 0 )     ? true :
                       ( ( flags & EXECSYSTEM_NOFOCUS ) != 0 )   ? false :
                                                                   true;

    const bool wait = ( ( flags & EXECSYSTEM_WAIT ) != 0 )      ? true :
                      ( ( flags & EXECSYSTEM_NOWAIT ) != 0 )    ? false :
                                                                  false;

    int return_code = 0;
    return RunProgram(UTF8_TODO::GetWide(command), &return_code, show_window, focus, wait);
}

#endif // WIN_DESKTOP


double CIntDriver::ExExecCommonAfterExecute(const FunctionCode source, const int flags, const bool success,
                                            std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event)
{
    if( ( flags & EXECSYSTEM_STOP ) != 0 )
    {
        m_bStopProc = true;
        m_pEngineDriver->SetStopCode(1);

        // clear any OnExit command for the current application
        if( source == FNEXECPFF_CODE )
            m_pEngineDriver->m_pPifFile->SetOnExitFilename(L"");
    }

    if( external_application_event != nullptr )
    {
        const bool wait = ( ( flags & EXECSYSTEM_WAIT ) != 0 );
        external_application_event->SetPostExecutionValues(success, wait);
        m_paradataDriver->RegisterAndLogEvent(std::move(external_application_event));
    }

    return success ? 1 : 0;
}


double CIntDriver::exgetcaselabel(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);
    const Symbol* symbol = NPT(fn8_node.symbol_index);

    if( symbol->IsA(SymbolType::Dictionary) )
    {
        const EngineDictionary* engine_dictionary = assert_cast<const EngineDictionary*>(symbol);
        const Case& data_case = engine_dictionary->GetEngineCase().GetCase();

        return AssignString(data_case.GetCaseLabel());
    }

    else
    {
        const DICX* pDicX = DPX(fn8_node.symbol_index);
        return AssignString(pDicX->GetCase().GetCaseLabel());
    }
}


double CIntDriver::exsetcaselabel(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);
    Symbol* symbol = NPT(fn8_node.symbol_index);

    if( symbol->IsA(SymbolType::Dictionary) )
    {
        EngineDictionary* engine_dictionary = assert_cast<EngineDictionary*>(symbol);
        Case& data_case = engine_dictionary->GetEngineCase().GetCase();

        data_case.SetCaseLabel(EvaluateString(fn8_node.extra_parameter));

        // refresh the case listing
        if( engine_dictionary->GetSubType() == SymbolSubType::Input )
            WindowsDesktopMessage::Send(WM_IMSA_KEY_CHANGED, &data_case);
    }

    else
    {
        DICX* pDicX = DPX(fn8_node.symbol_index);
        Case& data_case = pDicX->GetCase();

        data_case.SetCaseLabel(EvaluateString(fn8_node.extra_parameter));

        // refresh the case listing
        if( symbol->GetSubType() == SymbolSubType::Input )
            WindowsDesktopMessage::Send(WM_IMSA_KEY_CHANGED, &data_case);
    }

    return 1;
}


SharableString CIntDriver::EvaluateTextFill(const int program_index)
{
    const auto& text_fill_node = GetNode<Nodes::TextFill>(program_index);

    if( IsBinary(text_fill_node.data_type) )
    {
        const BinarySymbol* const binary_symbol = GetFromSymbolOrEngineItem<BinarySymbol*>(text_fill_node.symbol_index_or_expression,
                                                                                           text_fill_node.subscript_compilation);

        return ( binary_symbol != nullptr ) ? LocalhostCreateMappingForBinarySymbol(*binary_symbol) :
                                              SharableString();
    }

    else
    {
        return EvaluateSharableString(text_fill_node.data_type, text_fill_node.symbol_index_or_expression);
    }
}
