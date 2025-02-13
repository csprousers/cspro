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
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/Encryption.h>
#include <zEngineO/Nodes/Strings.h>
#include <zEngineO/Nodes/UserInterface.h>
#include <zEngineO/Nodes/Various.h>
#include <zToolsO/Encoders.h>
#include <zToolsO/Encryption.h>
#include <zToolsO/Tools.h>
#include <zToolsO/Utf8Convert.h>
#include <zUtilO/TraceMsg.h>
#include <zUtilO/CommonStore.h>
#include <zUtilO/TransactionManager.h>
#include <zDictO/DDClass.h>
#include <zDictO/ValueProcessor.h>
#include <zCaseO/Case.h>
#include <zCaseO/CaseItemReference.h>
#include <ZBRIDGEO/npff.h>
#include <Zissalib/CsDriver.h>
#include <zParadataO/Logger.h>
#include <zEngineF/EngineUI.h>


namespace
{
    constexpr TCHAR DecimalSeparator = '.';
}



template<typename T>
T CIntDriver::CharacterObjectToString(double working_string_index)
{
    const size_t size_t_working_string_index = static_cast<size_t>(working_string_index);

    // if the string is the last one in the array, which should almost always be the case, remove it
    if( ( m_workingStrings.size() - size_t_working_string_index ) == 1 )
    {
        if constexpr(std::is_same_v<T, CString>)
        {
            CString value = WS2CS(m_workingStrings.back());
            m_workingStrings.pop_back();
            return value;
        }

        else
        {
            T value = std::move(m_workingStrings.back());
            m_workingStrings.pop_back();
            return value;
        }
    }

    else if( size_t_working_string_index < m_workingStrings.size() )
    {
        if constexpr(std::is_same_v<T, CString>)
        {
            return WS2CS(m_workingStrings[size_t_working_string_index]);
        }

        else
        {
            return m_workingStrings[size_t_working_string_index];
        }
    }

    return ReturnProgrammingError(T());
}

template std::wstring CIntDriver::CharacterObjectToString(double working_string_index);
template StringNoCase CIntDriver::CharacterObjectToString(double working_string_index);
template CString CIntDriver::CharacterObjectToString(double working_string_index);


template<typename T/* = std::wstring*/>
T CIntDriver::EvalAlphaExpr(const int program_index)
{
    const double evaluated_char_expression = evalexpr(program_index);

    if constexpr(std::is_same_v<T, std::string>)
    {
        return UTF8Convert::WideToUTF8(CharacterObjectToString<std::wstring>(evaluated_char_expression));
    }

    else
    {
        return CharacterObjectToString<T>(evaluated_char_expression);
    }
}

template std::wstring CIntDriver::EvalAlphaExpr(int program_index);
template std::string CIntDriver::EvalAlphaExpr(int program_index);
template StringNoCase CIntDriver::EvalAlphaExpr(int program_index);
template CString CIntDriver::EvalAlphaExpr(int program_index);


double CIntDriver::AssignAlphaValue(const std::optional<std::wstring>& value)
{
    return value.has_value() ? AssignAlphaValue(*value) :
                               AssignBlankAlphaValue();
}


double CIntDriver::AssignAlphaValue(std::optional<std::wstring>&& value)
{
    return value.has_value() ? AssignAlphaValue(std::move(*value)) :
                               AssignBlankAlphaValue();
}


namespace
{
    struct EscapeTypeDetails
    {
        std::wstring_view newline_chars_actual_sv;
        bool escape_backslashes;
    };

    constexpr EscapeTypeDetails EscapeTypeDetailsMap[] = 
    {
        { _T("\n"),   false },
        { _T("\r\n"), false },
        { _T("\n"),   true  },
        { _T("\r\n"), true  },
    };

    static_assert(_countof(EscapeTypeDetailsMap) == ( static_cast<size_t>(CIntDriver::V0_EscapeType::NewlinesToSlashRN_Backslashes) + 1 ));
}


void CIntDriver::ConvertV0EscapesWorker(std::wstring& text, const V0_EscapeType v0_escape_type)
{
    ASSERT(m_usingLogicSettingsV0);
    const EscapeTypeDetails& escape_type_details = EscapeTypeDetailsMap[static_cast<size_t>(v0_escape_type)];

    size_t backslash_pos = 0;

    while( ( backslash_pos = text.find('\\', backslash_pos) ) != std::wstring::npos )
    {
        if( ( backslash_pos + 1 ) == text.length() )
            break;

        const TCHAR escape_ch = text[backslash_pos + 1];

        // convert "\\n" characters to "\n" or "\r\n"
        if( escape_ch == 'n' )
        {
            text.replace(backslash_pos, 2, escape_type_details.newline_chars_actual_sv);
            backslash_pos += escape_type_details.newline_chars_actual_sv.length();
        }

        // optionally convert "\\\\" characters to "\\"
        else if( escape_type_details.escape_backslashes && escape_ch == '\\' )
        {
            text.erase(backslash_pos, 1);
            ++backslash_pos;
        }

        else
        {
            ++backslash_pos;
        }
    }
}


void CIntDriver::ConvertV0EscapesWorker(std::vector<std::wstring>& text_lines, const V0_EscapeType v0_escape_type)
{
    for( std::wstring& text : text_lines )
        ConvertV0EscapesWorker(text, v0_escape_type);
}


void CIntDriver::ApplyV0EscapesWorker(std::wstring& text, const V0_EscapeType v0_escape_type)
{
    ASSERT(m_usingLogicSettingsV0);
    const EscapeTypeDetails& escape_type_details = EscapeTypeDetailsMap[static_cast<size_t>(v0_escape_type)];

    ASSERT(text.find('\r') == std::wstring::npos);

    size_t escape_pos = 0;

    while( ( escape_pos = text.find_first_of(_T("\n\\"), escape_pos) ) != std::wstring::npos )
    {
        if( text[escape_pos] == '\n' )
        {
            text.replace(escape_pos, 1, _T("\\n"));
            escape_pos += 2;
        }

        else if( escape_type_details.escape_backslashes )
        {
            text.replace(escape_pos, 1, _T("\\\\"));
            escape_pos += 2;
        }

        else
        {
            ++escape_pos;
        }        
    }
}


//----------------------------------------------------------------------
//  extonumber: execute TONUMBER function
//----------------------------------------------------------------------
double CIntDriver::extonumber(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const std::wstring number_string = EvalAlphaExpr(fnn_node.fn_expr[0]);
    const wstring_view trimmed_number_string_sv = SO::Trim(number_string);

    enum class Sign { None, Negative, Positive };
    Sign sign = Sign::None;
    const TCHAR* start_number_section = nullptr;
    bool reached_decimal_area = false;
    size_t length_number_section = 0;

    for( const TCHAR& ch : trimmed_number_string_sv )
    {
        if( start_number_section == nullptr && ( ch == '-' || ch == '+' ) )
        {
            // allow duplicate signs, but only if they are of the same type
            const Sign new_sign = ( ch == '-' ) ? Sign::Negative :
                                                  Sign::Positive;

            if( sign != Sign::None && sign != new_sign )
                break;

            sign = new_sign;
        }

        else if( !reached_decimal_area && ch == DecimalSeparator )
        {
            reached_decimal_area = true;

            // allow decimals at the beginning of the text
            if( start_number_section == nullptr)
                start_number_section = &ch;
        }

        else if( is_digit(ch) )
        {
            if( start_number_section == nullptr )
                start_number_section = &ch;
        }

        else if( start_number_section == nullptr && ch == BLANK )
        {
            // ignore blanks
        }

        else
        {
            break;
        }

        if( start_number_section != nullptr )
            ++length_number_section;
    }

    // if there was no numeric portion of the string, return 1/0 if boolean, or DEFAULT otherwise
    if( length_number_section == 0 )
    {
        return SO::EqualsNoCase(trimmed_number_string_sv, _T("true"))  ? 1 :
               SO::EqualsNoCase(trimmed_number_string_sv, _T("false")) ? 0 :
                                                                         DEFAULT;
    }

    // get just the string portion
    double value = atod(wstring_view(start_number_section, length_number_section));

    if( value == IMSA_BAD_DOUBLE || IsSpecial(value) )
        return DEFAULT;

    return ( sign == Sign::Negative ) ? -value :
                                        value;
}


//----------------------------------------------------------------------
//  exsysparm: execute SYSPARM function
//----------------------------------------------------------------------
double CIntDriver::exsysparm(int iExpr)
{
    // previously this function only returned the one Parameter= parameter, but
    // now it can also return values from a map of parameters
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    std::wstring parameter;

    if( fnn_node.fn_nargs == 0 )
    {
        parameter = CS2WS(m_pEngineDriver->m_pPifFile->GetParamString());
    }

    else
    {
        std::wstring argument = EvalAlphaExpr(fnn_node.fn_expr[0]);
        parameter = m_pEngineDriver->m_pPifFile->GetCustomParamString(argument);

#ifdef WIN_DESKTOP
        // on Windows, if the parameter isn't specified in the PFF file, check if it is a command line argument;
        // if so, return the argument (meaning that checking if sysparm isn't blank is a way of seeing
        // if something is defined on the command line)
        if( parameter.empty() )
        {
            std::wstring command_line = GetCommandLine();
            size_t argument_pos = SO::ToLower(command_line).find(SO::ToLower(argument));

            // make sure that the argument is a standalone argument
            if( argument_pos != std::wstring::npos && argument_pos > 0 )
            {
                size_t argument_end_pos = argument_pos + argument.length();

                if( std::iswspace(command_line[argument_pos - 1]) &&
                    ( argument_end_pos == command_line.length() || std::iswspace(command_line[argument_end_pos]) ) )
                {
                    parameter = command_line.substr(argument_pos, argument.length());
                }
            }
        }
#endif
    }

    return AssignAlphaValue(std::move(parameter));
}


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
    CString pattern = EvalAlphaExpr<CString>(va_node.arguments[0]);
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
        edit_result = SpecialValues::ValueToString(value);
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
            subindex[i] = evalexpr<int>( pTableNode->tvar_exprindex[i] );
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

        CString* pVariableLengthString = pVarT->GetLogicStringPtr(); // 20140325

        if( pVariableLengthString != nullptr )
        {
            csValue = *pVariableLengthString;
        }

        else
        {
            VARX* pVarX = pVarT->GetVarX();
            csValue = CString((LPCTSTR)svaraddr(pVarX), pVarT->GetLength());
        }
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
//  exstringliteral : executes a string literal
//----------------------------------------------------------------------
double CIntDriver::exstringliteral(int iExpr)
{
    const auto& string_literal_node = GetNode<Nodes::StringLiteral>(iExpr);
    return AssignAlphaValue(GetStringLiteral(string_literal_node.string_literal_index));
}


//----------------------------------------------------------------------
//  exworkstring : returns the value of a WorkString
//----------------------------------------------------------------------
double CIntDriver::exworkstring(int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    return AssignAlphaValue(GetSymbolWorkString(va_node.arguments[0]).GetString());
}


double CIntDriver::exworkstringcompute(int program_index)
{
    // assigning a value to a WorkString; this can also happen in exstringcompute
    const auto& symbol_reset_node = GetNode<Nodes::SymbolReset>(program_index);
    WorkString& work_string = GetSymbolWorkString(symbol_reset_node.symbol_index);

    work_string.SetString(EvalAlphaExpr(symbol_reset_node.initialize_value));

    return 0;
}


//----------------------------------------------------------------------
//  excharobj : executes alpha object
//----------------------------------------------------------------------
double CIntDriver::excharobj(int program_index)
{
    const auto& string_expression_node = GetNode<Nodes::StringExpression>(program_index);
    std::wstring text;

    if( string_expression_node.string_expression >= 0 &&
        Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        text = EvalAlphaExpr(string_expression_node.string_expression);
    }

    else
    {
        int abs_string_expression = std::abs(string_expression_node.string_expression);
        FunctionCode string_expression_function_code = GetNode<FunctionCode>(abs_string_expression);

        if( string_expression_function_code == FunctionCode::SVAR_CODE || string_expression_function_code == FunctionCode::MVAR_CODE )
        {
            text = CharacterObjectToString(exavar(abs_string_expression));
        }

        else
        {
            ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1));

            text = ( string_expression_function_code == FunctionCode::WORKSTRING_CODE ) ? GetSymbolWorkString(GetNode<int>(abs_string_expression + 1)).GetString() :
                   ( string_expression_function_code == FunctionCode::TVAR_CODE )       ? CharacterObjectToString(extavar(abs_string_expression)) :
                                                                                          EvalAlphaExpr(abs_string_expression);
        }
    }


    // done if no substring values are present
    if( string_expression_node.substring_index_expression == -1 )
        return AssignAlphaValue(std::move(text));


    // parse the substring values
    int text_length = text.length();
    int starting_position = evalexpr<int>(string_expression_node.substring_index_expression);

    if( starting_position < 0 )
    {
        // A negative start position means index from right end of string.
        // This allows code like "foobar"[-3] which results in "bar"
        starting_position = text_length + starting_position + 1;
    }

    // return a blank string if the substring values are not valid
    if( --starting_position < 0 || ( starting_position >= text_length && starting_position > 0 ) )
        return AssignBlankAlphaValue();

    int length;

    if( string_expression_node.substring_length_expression != -1 )
    {
        length = evalexpr<int>(string_expression_node.substring_length_expression);
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

    if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
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
        AssignValueToSymbol(*symbol_value_node, std::move(rhs_value));
    }

    // otherwise get the variable's current value and apply the new value on top of it
    else
    {
        ModifySymbolValue<std::wstring>(*symbol_value_node,
            [&](std::wstring& lhs_value)
            {
                int starting_position = evalexpr<int>(string_compute_node->substring_index_expression) - 1;

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
                    chars_to_copy = evalexpr<int>(string_compute_node->substring_length_expression);

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
                TCHAR* lhs_value_starting_position = lhs_value.data() + starting_position;
                _tmemcpy(lhs_value_starting_position, rhs_value.c_str(), rhs_chars_to_copy);

                // if more characters were requested to copy than exist in the RHS string, pad the LHS string with spaces
                if( chars_to_copy > rhs_chars_to_copy )
                    _tmemset(lhs_value_starting_position + rhs_chars_to_copy, ' ', chars_to_copy - rhs_chars_to_copy);
        });
    }

    return 0;
}


Symbol& CIntDriver::GetSymbolFromSymbolName(const StringNoCase& symbol_name, SymbolType preferred_symbol_type/* = SymbolType::None*/)
{
    try
    {
        return m_symbolTable.FindSymbolWithDotNotation(symbol_name, preferred_symbol_type);
    }

    catch( const Logic::SymbolTable::Exception& )
    {
        // an exception will be thrown if the symbol is not found; in that case, search any symbols that might have been declared locally
        // (this functionality could be moved to the SymbolTable methods, but this is a rare instance so now it will only be done here)
        const size_t dot_index = symbol_name.find('.');
        const wstring_view base_symbol_name_sv = ( dot_index != StringNoCase::npos ) ? wstring_view(symbol_name).substr(0, dot_index) :
                                                                                       wstring_view(symbol_name);
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
                if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_4) &&
                    scope_change_node.local_symbol_names_list != -1)
                {
                    const Nodes::List& local_symbol_names_node = GetListNode(scope_change_node.local_symbol_names_list);
                    ASSERT(local_symbol_names_node.number_elements == local_symbol_indices_node.number_elements);

                    for( int i = 0; i < local_symbol_names_node.number_elements; ++i )
                    {
                        if( SO::EqualsNoCase(base_symbol_name_sv, GetStringLiteral(local_symbol_names_node.elements[i])) )
                        {
                            symbol = &NPT_Ref(local_symbol_indices_node.elements[i]);;
                            return false;
                        }
                    }
                }

                return true;
            });

        if( symbol == nullptr )
            throw;

        // if the starting symbol was found, process any dot notation
        if( dot_index != StringNoCase::npos )
        {
            try
            {
                for( const wstring_view name_sv : SO::SplitString<wstring_view>(wstring_view(symbol_name).substr(dot_index + 1), '.') )
                    symbol = &m_symbolTable.FindSymbol(name_sv, symbol);
            }

            catch( const Logic::SymbolTable::NoSymbolsException& )
            {
                // throw an exception with the full symbol name
                throw Logic::SymbolTable::NoSymbolsException(symbol_name);
            }
        }

        return *symbol;
    }
}


std::tuple<Symbol*, Symbol*> CIntDriver::GetEvaluatedSymbolFromSymbolName(const std::wstring& symbol_name_and_potential_subscript, SymbolType preferred_symbol_type/* = SymbolType::None*/)
{
    wstring_view symbol_name_sv = symbol_name_and_potential_subscript;
    const size_t left_parenthesis_pos = symbol_name_and_potential_subscript.find('(');
    const bool explicit_subscript_specified = ( left_parenthesis_pos != std::wstring::npos );

    if( explicit_subscript_specified )
        symbol_name_sv = symbol_name_sv.substr(0, left_parenthesis_pos);

    Symbol& base_symbol = GetSymbolFromSymbolName(SO::Trim(symbol_name_sv));
    Symbol* wrapped_symbol = nullptr;

    if( explicit_subscript_specified && !base_symbol.IsA(SymbolType::Item) )
    {
        throw CSProException(_T("A subscript cannot be provided for the symbol '%s' of type '%s'."),
                                base_symbol.GetName().c_str(), ToString(base_symbol.GetType()));
    }

    if( base_symbol.IsA(SymbolType::Item) )
    {
        EngineItem& engine_item = assert_cast<EngineItem&>(base_symbol);
        const TCHAR* subscript_text = explicit_subscript_specified ? ( symbol_name_and_potential_subscript.c_str() + left_parenthesis_pos ) :
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

    if( symbol_index == 0 )
    {
        ASSERT(Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_7_000_1));
        symbol_index = -1;
    }


    // use the current symbol if none was supplied
    if( symbol_index == -1 )
    {
        if( m_iExSymbol <= 0 )
            return AssignBlankAlphaValue();

        symbol_index = m_iExSymbol;
    }

    else if( symbol_index == 2147483647/*INT_MAX*/ )
    {
        // get the name of a partial save field
        CString partial_save_field_name;
        const Case& data_case = DIX(0)->GetCase();

        if( data_case.GetPartialSaveCaseItemReference() != nullptr )
        {
            const auto& partial_save_case_item_reference = *data_case.GetPartialSaveCaseItemReference();

            partial_save_field_name = partial_save_case_item_reference.GetName();

            // add occurrences
            partial_save_field_name.Append(partial_save_case_item_reference.GetItemIndexHelper().GetMinimalOccurrencesText(partial_save_case_item_reference));
        }

        return AssignAlphaValue(partial_save_field_name);
    }


    const Symbol* symbol = NPT(symbol_index);

    // getsymbol: evaluate the symbol
    if( fng_node.m_iFunCode == FunctionCode::FNGETSYMBOL_CODE )
        return AssignAlphaValue(symbol->GetName());


    // getlabel: evaluate the label

    // only 1 parameter was used in the function
    if( fng_node.m_iExpr == -1 )
    {
        return AssignAlphaValue(SymbolCalculator::GetLabel(*symbol));
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
                dict_value = value_processor->GetDictValue(EvalAlphaExpr<CString>(fng_node.m_iExpr));
            }

            if( dict_value != nullptr )
                label = dict_value->GetLabel();
        }

        else
        {
            const DictValue* dict_value = value_processor->GetDictValueByLabel(EvalAlphaExpr<CString>(fng_node.m_iExpr));

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
        VART* pVarT = VPT(pSVAR->m_iVarIndex);

        if( pVarT->GetLogicStringPtr() != nullptr ) // 20140326 a variable length string
        {
            csValue = *(pVarT->GetLogicStringPtr());
        }

        else
        {
            csValue = CString(pVarT->GetAsciiValue(0), pVarT->GetLength());
        }
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


// this function returns false (and issues a warning) if there is a problem evaluating the field reference
bool CIntDriver::EvaluateNoteReference(const FNNOTE_NODE& note_node, std::shared_ptr<NamedReference>& named_reference, int& field_symbol)
{
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
            const Symbol* symbol = NPT(m_iExSymbol);

            // a level
            if( symbol->IsA(SymbolType::Group) && SymbolCalculator::GetLevelNumber_base1(*symbol) > 0 )
            {
                evaluate_simple_note_symbol_index = m_iExSymbol;
            }

            // a field
            else if( symbol->IsA(SymbolType::Variable) )
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
    named_reference.reset();

    if( evaluate_simple_note_symbol_index >= 0 )
    {
        const Symbol* symbol = NPT(evaluate_simple_note_symbol_index);
        field_symbol = evaluate_simple_note_symbol_index;
        named_reference = std::make_shared<NamedReference>(WS2CS(SymbolCalculator::GetBaseName(*symbol)), CString());
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

            auto case_item_reference = std::make_shared<CaseItemReference>(*pVarT->GetCaseItem(), CString());
            ConvertIndex(theCurrentIndexes, *case_item_reference);

            named_reference = case_item_reference;
        }
    }

    else if( evaluate_passed_field )
    {
        auto pMVAR = (const MVAR_NODE*)PPT(note_node.variable_expression);
        field_symbol = pMVAR->m_iVarIndex;

        VART* pVarT = VPT(field_symbol);

        auto case_item_reference = std::make_shared<CaseItemReference>(*pVarT->GetCaseItem(), CString());

        if( pMVAR->m_iVarType == MVAR_CODE )
        {
            UserIndexesArray dIndex;
            mvarGetSubindexes(pMVAR, dIndex);

            C3DObject the3dObject;
            CsDriver::PassTo3D(&the3dObject, NPT(field_symbol), dIndex);

            ConvertIndex(the3dObject, *case_item_reference);
        }

        named_reference = case_item_reference;
    }


    // no valid entity found
    if( named_reference == nullptr )
    {
        issaerror(MessageType::Error, 46501);
        return false;
    }

    // verify that the note data is accessible
    else if( check_if_data_is_accessible )
    {
        return IsDataAccessible(NPT_Ref(field_symbol), true);
    }

    // otherwise the data accessibility checks will have been done at compile-time
    else
    {
        ASSERT(IsDataAccessible(NPT_Ref(field_symbol), true));
        return true;
    }
}


std::optional<CString> CIntDriver::EvaluateNoteOperatorId(const FNNOTE_NODE& note_node, int field_symbol)
{
    // operator IDs will be ignored for case notes
    if( !NPT(field_symbol)->IsOneOf(SymbolType::Dictionary, SymbolType::Pre80Dictionary) )
    {
        if( note_node.operator_id_expression != -1 )
        {
            return EvalAlphaExpr<CString>(note_node.operator_id_expression);
        }

        else if( Issamod == ModuleType::Entry )
        {
            return assert_cast<CEntryDriver*>(m_pEngineDriver)->GetOperatorId();
        }
    }

    return std::nullopt;
}


double CIntDriver::exgetnote(int iExpr)
{
    const auto& note_node = GetNode<FNNOTE_NODE>(iExpr);
    std::shared_ptr<NamedReference> named_reference;
    int field_symbol;
    CString note_content;

    if( EvaluateNoteReference(note_node, named_reference, field_symbol) )
    {
        std::optional<CString> operator_id = EvaluateNoteOperatorId(note_node, field_symbol);
        note_content = m_pEngineDriver->GetNoteContent(named_reference, operator_id, field_symbol);
    }

    return AssignAlphaValue(note_content);
}


double CIntDriver::exputnote(int iExpr)
{
    const auto& note_node = GetNode<FNNOTE_NODE>(iExpr);
    std::shared_ptr<NamedReference> named_reference;
    int field_symbol;

    if( !EvaluateNoteReference(note_node, named_reference, field_symbol) )
        return 0;

    std::optional<CString> operator_id = EvaluateNoteOperatorId(note_node, field_symbol);
    CString note_content = EvalAlphaExpr<CString>(note_node.note_text_expression);

    m_pEngineDriver->SetNote(named_reference, operator_id, note_content, field_symbol);

    return 1;
}


double CIntDriver::exeditnote(int iExpr)
{
    const auto& note_node = GetNode<FNNOTE_NODE>(iExpr);
    std::shared_ptr<NamedReference> named_reference;
    int field_symbol;
    CString note_content;

    if( EvaluateNoteReference(note_node, named_reference, field_symbol) )
    {
        std::optional<CString> operator_id = EvaluateNoteOperatorId(note_node, field_symbol);
        note_content = std::get<CString>(m_pEngineDriver->EditNote(named_reference, operator_id, field_symbol, false));
    }

    return AssignAlphaValue(note_content);
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
    CString operator_id = EvalAlphaExpr<CString>(pFun->fn_expr[0]);

    // the maximum length of an operator ID is 32 characters
    constexpr int MaximumOperatorIdLength = 32;
    if( operator_id.GetLength() > MaximumOperatorIdLength )
        operator_id.Truncate(MaximumOperatorIdLength);

    assert_cast<CEntryDriver*>(m_pEngineDriver)->SetOperatorId(operator_id);

    return 1;
}


double CIntDriver::exgetusername(int iExpr) // 20111028
{
    return AssignAlphaValue(GetDeviceUserName());
}


double CIntDriver::exgetos(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    int additional_details_node = va_node.arguments[0];

    // the return value: Windows = 10, Android = 20
    constexpr double os_number = OnWindows() ? 10 : 20;

    if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1) )
    {
        const FNN_NODE& fnn_node = GetNode<FNN_NODE>(iExpr);
        additional_details_node = ( fnn_node.fn_expr[0] > 0 ) ? fnn_node.fn_expr[0] : -1;
    }

    if( additional_details_node != -1 )
    {
        const OperatingSystemDetails& operating_system_details = GetOperatingSystemDetails();

        // fill in a hashmap with all details...
        if( additional_details_node == -2 )
        {
            LogicHashMap& hashmap = GetSymbolLogicHashMap(va_node.arguments[1]);
            ASSERT(hashmap.IsValueTypeString() &&
                   hashmap.GetNumberDimensions() == 1 &&
                   hashmap.DimensionTypeHandles(0, DataType::String));

            hashmap.Reset();

            hashmap.SetValue({ _T("name") }, operating_system_details.operating_system);
            hashmap.SetValue({ _T("version") }, operating_system_details.version_number);

            if( operating_system_details.build_number.has_value() )
                hashmap.SetValue({ _T("build") }, *operating_system_details.build_number);
        }

        // ...or a string with the operating system and version
        else
        {
            std::wstring text_description = SO::Concatenate(operating_system_details.operating_system,
                                                            _T(";"),
                                                            operating_system_details.version_number);

            if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_7_6_000_1) )
            {
                int iSymVarBuf = additional_details_node;

                VART* pVarT = VPT(iSymVarBuf);

                if( pVarT->GetLogicStringPtr() ) // 20140326 a variable length string
                {
                    *( pVarT->GetLogicStringPtr() ) = WS2CS(text_description);
                }

                else
                {
                    VARX* pVarX = VPX(iSymVarBuf);
                    TCHAR* pBuff = (TCHAR*)svaraddr(pVarX);
                    SO::MakeExactLength(text_description, pVarT->GetLength());
                    _tmemcpy(pBuff, text_description.data(), pVarT->GetLength());
                }
            }

            else
            {
                AssignValueToSymbol(GetNode<Nodes::SymbolValue>(additional_details_node), std::move(text_description));
            }
        }
    }

    return os_number;
}


double CIntDriver::exgetdeviceid(int /*iExpr*/)
{
    // originally this function was called getmac and returend the MAC address; on 20141218 it was decided
    // to change it so that it returns a unique device ID; on Windows it will return the MAC address, while on
    // Android it will return the ANDROID_ID (which is longer than the MAC address)
    return AssignAlphaValue(GetDeviceId());
}


double CIntDriver::exuuid(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    std::wstring uuid;

    if( va_node.arguments[0] == -1 )
    {
        uuid = CreateUuid();
    }

    else
    {
        // get the UUID of a case or create one if needed
        Symbol& symbol = NPT_Ref(va_node.arguments[0]);

        if( symbol.IsA(SymbolType::Dictionary) )
        {
            uuid = CS2WS(assert_cast<EngineDictionary&>(symbol).GetEngineCase().GetCase().GetOrCreateUuid());
        }

        else
        {
            uuid = CS2WS(assert_cast<DICT&>(symbol).GetDicX()->GetCase().GetOrCreateUuid());
        }
    }

    return AssignAlphaValue(std::move(uuid));
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
    std::wstring command = EvalAlphaExpr(execsystem_node.m_iCommand);

    std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event = ExExecCommonBeforeExecute(FNEXECSYSTEM_CODE, command, execsystem_node.m_iOptions);

#ifdef WIN_DESKTOP
    success =  ExExecCommonExecute(command, execsystem_node.m_iOptions);

#else
    bool wait = ( ( execsystem_node.m_iOptions & EXECSYSTEM_WAIT ) != 0 );

    // for a couple actions, fully evaluate the path before passing the file paths to Android functions
    size_t colon_pos = command.find(':');

    if( colon_pos != std::wstring::npos )
    {
        constexpr const TCHAR* ActionsToFullyEvaluatePath[] = { _T("camera"), _T("signature"), _T("view") };
        wstring_view command_sv = command;
        wstring_view action_sv = command_sv.substr(0, colon_pos);

        for( size_t i = 0; i < _countof(ActionsToFullyEvaluatePath); ++i )
        {
            if( SO::EqualsNoCase(action_sv, ActionsToFullyEvaluatePath[i]) )
            {
                std::wstring filename = SO::Trim(command_sv.substr(colon_pos + 1));
                MakeFullPathFileName(filename);
                command = SO::Concatenate(action_sv, _T(":"), filename);
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
        std::wstring pff_filename = EvalFullPathFileName(execsystem_node.m_iCommand);
        return ExExecPFF(std::move(pff_filename), execsystem_node.m_iOptions);
    }
}


double CIntDriver::ExExecPFF(std::variant<LogicPff*, std::wstring> logic_pff_or_pff_filename, std::optional<int> flags/* = std::nullopt*/)
{
    LogicPff* logic_pff = nullptr;
    std::shared_ptr<const PFF> pff;
    std::shared_ptr<PffExecutor> pff_executor;
    bool success = true;

    if( std::holds_alternative<LogicPff*>(logic_pff_or_pff_filename) )
    {
        logic_pff = std::get<LogicPff*>(logic_pff_or_pff_filename);
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
        loaded_pff->SetPifFileName(WS2CS(std::get<std::wstring>(logic_pff_or_pff_filename)));
        success = loaded_pff->LoadPifFile(true);
        pff = std::move(loaded_pff);
    }

    ASSERT(pff != nullptr && flags.has_value());

    // when a PFF is launched in wait mode try to use the PFF executor
    bool use_pff_executor = ( ( ( *flags & EXECSYSTEM_WAIT ) != 0 ) && PffExecutor::CanExecute(pff->GetAppType()) );

    std::wstring pff_filename;

    if( logic_pff == nullptr || !logic_pff->IsModified() )
    {
        pff_filename = CS2WS(pff->GetPifFileName());
    }

    else if( !use_pff_executor )
    {
        pff_filename = logic_pff->GetRunnableFilename();
    }

    else
    {
        pff_filename = logic_pff->GetName();
    }

    std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event = ExExecCommonBeforeExecute(FNEXECPFF_CODE, pff_filename, *flags);

    if( success )
    {
        if( use_pff_executor )
        {
            try
            {
                if( pff_executor == nullptr )
                    pff_executor = std::make_shared<PffExecutor>();

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
                issaerror(MessageType::Error, 47195, PortableFunctions::PathGetFilename(pff_filename), exception.GetErrorMessage().c_str());
                success = false;
            }
        }

        // otherwise launch the application using the executable (as in execsystem)
        else
        {
#ifdef WIN_DESKTOP
            std::optional<std::wstring> exe_filename = pff->GetExecutableProgram();

            success = exe_filename.has_value() &&
                      ExExecCommonExecute(FormatTextCS2WS(_T("%s \"%s\""), exe_filename->c_str(), pff_filename.c_str()), *flags);
#else
            if( pff->GetAppType() == ENTRY_TYPE || PffExecutor::CanExecute(pff->GetAppType()) )
            {
                // 20140213 a temporary kludge ... we'll set a parameter concerning the next application to run, which will be run when this application ends
                success = (bool)PlatformInterface::GetInstance()->GetApplicationInterface()->ExecPff(pff_filename);
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


std::unique_ptr<Paradata::ExternalApplicationEvent> CIntDriver::ExExecCommonBeforeExecute(FunctionCode source, const std::wstring& command, int flags)
{
    try
    {
        TransactionManager::CommitTransactions();
    }

    catch( const DataRepositoryException::Error& exception )
    {
        issaerror(MessageType::Warning, 10104, exception.GetErrorMessage().c_str());
    }

    std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event;

    if( Paradata::Logger::IsOpen() )
    {
        bool stop = ( ( flags & EXECSYSTEM_STOP ) != 0 );

        external_application_event = std::make_unique<Paradata::ExternalApplicationEvent>(
            ( source == FNEXECSYSTEM_CODE ) ? Paradata::ExternalApplicationEvent::Source::ExecSystem : Paradata::ExternalApplicationEvent::Source::ExecPff,
            command,
            stop);
    }

    return external_application_event;
}


bool CIntDriver::ExExecCommonExecute(std::wstring command, int flags)
{
#ifdef WIN_DESKTOP
    int show_window = ( ( flags & EXECSYSTEM_MAXIMIZED ) != 0 ) ? SW_MAXIMIZE :
                      ( ( flags & EXECSYSTEM_MINIMIZED ) != 0 ) ? SW_MINIMIZE :
                      ( ( flags & EXECSYSTEM_NORMAL ) != 0 )    ? SW_SHOWNA :
                                                                  SW_SHOWNA;

    bool focus =      ( ( flags & EXECSYSTEM_FOCUS ) != 0 )     ? true :
                      ( ( flags & EXECSYSTEM_NOFOCUS ) != 0 )   ? false :
                                                                  true;

    bool wait =       ( ( flags & EXECSYSTEM_WAIT ) != 0 )      ? true :
                      ( ( flags & EXECSYSTEM_NOWAIT ) != 0 )    ? false :
                                                                  false;

    int return_code = 0;
    return RunProgram(std::move(command), &return_code, show_window, focus, wait);

#else
    return false;

#endif
}


double CIntDriver::ExExecCommonAfterExecute(FunctionCode source, int flags, bool success, std::unique_ptr<Paradata::ExternalApplicationEvent> external_application_event)
{
    bool stop = ( ( flags & EXECSYSTEM_STOP ) != 0 );

    if( stop )
    {
        m_bStopProc = true;
        m_pEngineDriver->SetStopCode(1);

        // clear any OnExit command for the current application
        if( source == FNEXECPFF_CODE )
            m_pEngineDriver->m_pPifFile->SetOnExitFilename(_T(""));
    }

    if( external_application_event != nullptr )
    {
        bool wait = ( ( flags & EXECSYSTEM_WAIT ) != 0 );
        external_application_event->SetPostExecutionValues(success, wait);
        m_pParadataDriver->RegisterAndLogEvent(std::move(external_application_event));
    }

    return success ? 1 : 0;
}


double CIntDriver::exview(const int program_index)
{
    const auto& view_node = GetNode<Nodes::View>(program_index);

    const int symbol_index_or_source_expression =
        Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_7_7_000_1) ? view_node.symbol_index_or_source_expression :
                                                                                 GetNode<FNN_NODE>(program_index).fn_expr[0];

    const int subscript_compilation =
        Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1)  ? view_node.subscript_compilation :
                                                                                  -1;

    const int viewer_options_node_index =
        Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1)  ? view_node.viewer_options_node_index :
        Versioning::MeetsCompiledLogicVersion(Serializer:: Iteration_7_7_000_2) ? view_node.subscript_compilation :
                                                                                  -1;
    std::unique_ptr<const ViewerOptions> viewer_options = EvaluateViewerOptions(viewer_options_node_index);

    // viewing files or URLs
    if( symbol_index_or_source_expression >= 0 )
    {
        std::wstring filename = EvalAlphaExpr(symbol_index_or_source_expression);
        bool success = false;

        Viewer viewer;
        viewer.UseEmbeddedViewer()
              .UseSharedHtmlLocalFileServer()
              .SetOptions(viewer_options.get());

        if( filename.find(_T("://")) != std::wstring::npos )
        {
            success = viewer.ViewHtmlUrl(filename);
        }

        else
        {
            MakeFullPathFileName(filename);
            success = viewer.ViewFile(filename);
        }

        // handle any program control exceptions that may have resulted from JavaScript
        // calls into CSPro logic
        RethrowProgramControlExceptions();

        return success ? 1 : 0;
    }

    // viewing objects
    else
    {
        Symbol* symbol = GetFromSymbolOrEngineItem(-1 * symbol_index_or_source_expression, subscript_compilation);

        if( symbol == nullptr )
            return 0;

        if( symbol->IsA(SymbolType::Pre80Dictionary) )
        {
            return exCase_view(assert_cast<DICT&>(*symbol), viewer_options.get());
        }

        else if( symbol->IsA(SymbolType::Document) )
        {
            return exDocument_view(assert_cast<const LogicDocument&>(*symbol), viewer_options.get());
        }

        else if( symbol->IsA(SymbolType::Image) )
        {
            return exImage_view(assert_cast<const LogicImage&>(*symbol), viewer_options.get());
        }

        else if( symbol->IsA(SymbolType::NamedFrequency) )
        {
            return exFreq_view(assert_cast<const NamedFrequency&>(*symbol), viewer_options.get(), -1);
        }

        else if( symbol->IsA(SymbolType::Report) )
        {
            return exReport_view(assert_cast<Report&>(*symbol), viewer_options.get());
        }

        else
        {
            return ReturnProgrammingError(DEFAULT);
        }
    }
}


double CIntDriver::exsavesetting(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    bool success = false;

    std::shared_ptr<CommonStore> common_store = m_pEngineDriver->GetCommonStore();

    if( common_store != nullptr )
    {
        common_store->SwitchTable(CommonStore::TableType::UserSettings);

        // clear the database
        if( va_node.arguments[0] == -1 )
        {
            success = common_store->Clear();
        }

        else
        {
            std::wstring key = EvalAlphaExpr(va_node.arguments[0]);
            std::wstring value = EvaluateExpressionAsString(static_cast<DataType>(va_node.arguments[1]), va_node.arguments[2]);

            if( value.empty() )
            {
                success = common_store->Delete(key);
            }

            else
            {
                success = common_store->PutString(key, value);
            }
        }
    }

    return success;
}


double CIntDriver::exloadsetting(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    std::wstring key = EvalAlphaExpr(va_node.arguments[0]);
    std::optional<std::wstring> value;

    std::shared_ptr<CommonStore> common_store = m_pEngineDriver->GetCommonStore();

    if( common_store != nullptr )
    {
        common_store->SwitchTable(CommonStore::TableType::UserSettings);

        value = common_store->GetString(key);

        // if they gave a default value, put that in the database and return it
        if( !value.has_value() && va_node.arguments[1] != -1 )
        {
            value = EvaluateExpressionAsString(static_cast<DataType>(va_node.arguments[1]), va_node.arguments[2]);
            common_store->PutString(key, *value);
        }
    }

    return AssignAlphaValue(std::move(value));
}


double CIntDriver::exgetcaselabel(int iExpr)
{
    const auto& fn8_node = GetNode<FN8_NODE>(iExpr);
    const Symbol* symbol = NPT(fn8_node.symbol_index);

    if( symbol->IsA(SymbolType::Dictionary) )
    {
        const EngineDictionary* engine_dictionary = assert_cast<const EngineDictionary*>(symbol);
        const Case& data_case = engine_dictionary->GetEngineCase().GetCase();

        return AssignAlphaValue(data_case.GetCaseLabel());
    }

    else
    {
        const DICX* pDicX = DPX(fn8_node.symbol_index);
        return AssignAlphaValue(pDicX->GetCase().GetCaseLabel());
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

        data_case.SetCaseLabel(EvalAlphaExpr<CString>(fn8_node.extra_parameter));

#ifdef WIN_DESKTOP
        // refresh the case listing
        if( engine_dictionary->GetSubType() == SymbolSubType::Input )
            WindowsDesktopMessage::Send(WM_IMSA_KEY_CHANGED, &data_case);
#endif
    }

    else
    {
        DICX* pDicX = DPX(fn8_node.symbol_index);
        Case& data_case = pDicX->GetCase();

        data_case.SetCaseLabel(EvalAlphaExpr<CString>(fn8_node.extra_parameter));

#ifdef WIN_DESKTOP
        if( symbol->GetSubType() == SymbolSubType::Input ) // refresh the case listing
            WindowsDesktopMessage::Send(WM_IMSA_KEY_CHANGED, &data_case);
#endif
    }

    return 1;
}


double CIntDriver::exdecryptstring(int iExpr)
{
    // currently this is only used to decrypt locally-declared config variables
    const auto& encryption_node = GetNode<Nodes::Encryption>(iExpr);
    ASSERT(encryption_node.function_code == FunctionCode::DECRYPT_STRING_CODE);

    std::wstring encrypted_string = EvalAlphaExpr(encryption_node.string_expression);
    std::wstring decrypted_string = Encryptor(encryption_node.encryption_type).Decrypt(encrypted_string);

    return AssignAlphaValue(std::move(decrypted_string));
}


double CIntDriver::exencode(int iExpr)
{
    const auto& encode_node = GetNode<Nodes::Encode>(iExpr);
    ASSERT(encode_node.encoding_type != Nodes::EncodeType::Default || encode_node.string_expression >= 0);

    // change the default encoding type
    if( encode_node.string_expression < 0 )
    {
        m_currentEncodeType = encode_node.encoding_type;
        return AssignBlankAlphaValue();
    }

    // or encode a string
    else
    {
        Nodes::EncodeType encoding_type = ( encode_node.encoding_type == Nodes::EncodeType::Default ) ? m_currentEncodeType :
                                                                                                        encode_node.encoding_type;
        std::wstring text = EvalAlphaExpr(encode_node.string_expression);

        return AssignAlphaValue(
            ( encoding_type == Nodes::EncodeType::Html )            ? Encoders::ToHtml(text) :
            ( encoding_type == Nodes::EncodeType::Csv )             ? Encoders::ToCsv(std::move(text)) :
            ( encoding_type == Nodes::EncodeType::PercentEncoding ) ? Encoders::ToPercentEncoding(text) :
            ( encoding_type == Nodes::EncodeType::Uri )             ? Encoders::ToUri(text) :
            ( encoding_type == Nodes::EncodeType::UriComponent )    ? Encoders::ToUriComponent(text) :
            ( encoding_type == Nodes::EncodeType::Slashes )         ? Encoders::ToEscapedString(std::move(text)) :
            ( encoding_type == Nodes::EncodeType::JsonString )      ? Encoders::ToJsonString(text) :
                                                                      ReturnProgrammingError(text));
    }
}


std::wstring CIntDriver::EvaluateTextFill(int program_index)
{
    const auto& text_fill_node = GetNode<Nodes::TextFill>(program_index);

    if( IsBinary(text_fill_node.data_type) )
    {
        const BinarySymbol* binary_symbol = GetFromSymbolOrEngineItem<BinarySymbol*>(text_fill_node.symbol_index_or_expression, text_fill_node.subscript_compilation);

        return ( binary_symbol != nullptr ) ? LocalhostCreateMappingForBinarySymbol(*binary_symbol) :
                                              std::wstring();
    }

    else
    {
        return EvaluateExpressionAsString(text_fill_node.data_type, text_fill_node.symbol_index_or_expression);
    }
}
