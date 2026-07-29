#include "stdafx.h"
#include "IncludesRT.h"
#include "Array.h"
#include "List.h"
#include "StringComparer.h"
#include "WorkString.h"
#include "Nodes/Encryption.h"
#include "Nodes/Strings.h"
#include "Nodes/TextTemplate.h"
#include "Nodes/Various.h"
#include <regex>


// --------------------------------------------------------------------------
// string routines
// --------------------------------------------------------------------------

SharableString LogicInterpreter::EvaluateSharableString(const int program_index)
{
    return GetWorkingSharableString(Evaluate<size_t>(program_index));
}


SharableString LogicInterpreter::EvaluateSharableString(const DataType value_data_type, const int program_index)
{
    switch( value_data_type )
    {
        case DataType::String:  return EvaluateSharableString(program_index);
        case DataType::Numeric: return DoubleToString(Evaluate<double>(program_index));
        default:                return ReturnProgrammingError(SharableString());
    }
}

SharableString LogicInterpreter::EvaluateNullableSharableString(const int program_index)
{
    return ( program_index != -1 ) ? EvaluateSharableString(program_index) :
                                     SharableString();
}


std::string LogicInterpreter::EvaluateString(const int program_index)
{
    return GetWorkingString(Evaluate<size_t>(program_index));
}


std::string LogicInterpreter::EvaluateString(const DataType value_data_type, const int program_index)
{
    return EvaluateSharableString(value_data_type, program_index).Release();
}


double LogicInterpreter::AssignStringNull()
{
    m_workingStrings.emplace_back();
    ASSERT81(!m_workingStrings.back().IsSet());
    return static_cast<double>(m_workingStrings.size() - 1);
}


SharableString LogicInterpreter::GetWorkingSharableString(const size_t index)
{
    // if the string is the last one in the array, which should almost always be the case, remove it
    if( ( index + 1 ) == m_workingStrings.size() )
    {
        SharableString sharable_string = std::move(m_workingStrings.back());
        m_workingStrings.pop_back();
        return sharable_string;
    }

    else if( index < m_workingStrings.size() )
    {
        return m_workingStrings[index];
    }

    else
    {
        return ReturnProgrammingError(SharableString());
    }
}


std::string LogicInterpreter::GetWorkingString(const size_t index)
{
    return GetWorkingSharableString(index).Release();
}


double LogicInterpreter::ex_string_literal(const int program_index)
{
    const auto& string_literal_node = GetNode<Nodes::StringLiteral>(program_index);
    return AssignString(m_engineData->string_literals[string_literal_node.string_literal_index]);
}


// this function is templated so that it can handle both std::string and std::wstring objects
template<auto memset_T, typename T>
void ex_string_compute_handle_subscripts(T& lhs_value, const T& rhs_value, const size_t starting_position, const std::optional<size_t> chars_to_copy)
{
    size_t rhs_chars_to_copy;
    size_t padding;

    // if no length is specified, copy the the entire RHS string
    if( !chars_to_copy.has_value() )
    {
        rhs_chars_to_copy = rhs_value.length();
        padding = 0;
    }

    // otherwise copy the number of characters requested
    else
    {
        rhs_chars_to_copy = std::min(*chars_to_copy, rhs_value.length());
        padding = *chars_to_copy - rhs_chars_to_copy;
    }

    // increase the LHS string length as necessary
    const size_t max_string_length = starting_position + rhs_chars_to_copy + padding;

    if( max_string_length > lhs_value.length() )
        lhs_value.resize(max_string_length, ' ');

    // copy all of some of the RHS string
    typename T::pointer const lhs_value_starting_position = lhs_value.data() + starting_position;
    memcpy(lhs_value_starting_position, rhs_value.c_str(), rhs_chars_to_copy * sizeof(typename T::value_type));

    // if more characters were requested to copy than exist in the RHS string, pad the LHS string with spaces
    if( padding != 0 )
        memset_T(lhs_value_starting_position + rhs_chars_to_copy, ' ', padding);
}


double LogicInterpreter::ex_string_compute(const int program_index)
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
            case MoveType::UserFunction:
            case MoveType::Variable:
                simulated_symbol_value_node.symbol_index = GetNode<Nodes::ElementReference>(move_node.move_expr).symbol_index;
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
    SharableString rhs_value = EvaluateSharableString(string_compute_node->string_expression);

    // if there are no subscripts used, we can set the value directly
    if( string_compute_node->substring_index_expression == -1 )
    {
        AssignValueToSymbol(*symbol_value_node, std::move(rhs_value));
    }

    // otherwise get the variable's current value and apply the new value on top of it
    else
    {
        const int starting_position = Evaluate<int>(string_compute_node->substring_index_expression) - 1;

        // return if the starting position is invalid
        if( starting_position < 0 )
            return DEFAULT;

        // return if the number of characters to copy is invalid or would result in nothing to copy
        const std::optional<int> chars_to_copy = EvaluateOptional<int>(string_compute_node->substring_length_expression);

        if( chars_to_copy.has_value() && *chars_to_copy <= 0 )
            return DEFAULT;

        ModifySymbolValue<SharableString>(*symbol_value_node,
            [&](SharableString& lhs_value)
            {
                // the subscript handling routine could be converted to only use UTF-8 strings,
                // using SO::WideGetOffset to determine where to modify the LHS string, but for
                // simplicity, for now strings will be converted to wide strings as necessary
                if( TC::UsesOnlyUtf8SingleByteChars(*lhs_value) &&
                    TC::UsesOnlyUtf8SingleByteChars(*rhs_value) )
                {
                    ex_string_compute_handle_subscripts<&memset>(
                        lhs_value.MakeModifiable(),
                        *rhs_value,
                        starting_position,
                        chars_to_copy
                    );
                }

                else
                {
                    std::wstring wide_lhs_value = TC::ToWide(*lhs_value);

                    ex_string_compute_handle_subscripts<&wmemset>(
                        wide_lhs_value,
                        TC::ToWide(*rhs_value),
                        starting_position,
                        chars_to_copy
                    );

                    lhs_value = TC::ToUtf8(wide_lhs_value);
                }
            });
    }

    return 0;
}



// --------------------------------------------------------------------------
// string escaping routines
// --------------------------------------------------------------------------

namespace
{
    struct EscapeTypeDetails
    {
        std::string_view newline_chars_actual_sv;
        bool escape_backslashes;
    };

    constexpr EscapeTypeDetails EscapeTypeDetailsMap[] =
    {
        { "\n",   false },
        { "\r\n", false },
        { "\n",   true  },
        { "\r\n", true  },
    };

    static_assert(_countof(EscapeTypeDetailsMap) == ( static_cast<size_t>(LogicInterpreter::V0_EscapeType::NewlinesToSlashRN_Backslashes) + 1 ));
}


SharableString LogicInterpreter::ConvertV0Escapes(SharableString text, const V0_EscapeType v0_escape_type/* = V0_EscapeType::NewlinesToSlashN*/)
{
    if( m_usingLogicSettingsV0 )
        text = ConvertV0Escapes(*text, v0_escape_type);

    return text;
}


std::string LogicInterpreter::ConvertV0Escapes(std::string text, const V0_EscapeType v0_escape_type/* = V0_EscapeType::NewlinesToSlashN*/)
{
    if( !m_usingLogicSettingsV0 )
        return text;

    const EscapeTypeDetails& escape_type_details = EscapeTypeDetailsMap[static_cast<size_t>(v0_escape_type)];

    size_t backslash_pos = 0;

    while( ( backslash_pos = text.find('\\', backslash_pos) ) != std::string::npos )
    {
        if( ( backslash_pos + 1 ) == text.length() )
            break;

        const char escape_ch = text[backslash_pos + 1];

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

    return text;
}


SharableString LogicInterpreter::ApplyV0Escapes(SharableString text, const V0_EscapeType v0_escape_type/* = V0_EscapeType::NewlinesToSlashN*/)
{
    if( m_usingLogicSettingsV0 )
        text = ConvertV0Escapes(*text, v0_escape_type);

    return text;
}


std::string LogicInterpreter::ApplyV0Escapes(std::string text, const V0_EscapeType v0_escape_type/* = V0_EscapeType::NewlinesToSlashN*/)
{
    if( !m_usingLogicSettingsV0 )
        return text;

    const EscapeTypeDetails& escape_type_details = EscapeTypeDetailsMap[static_cast<size_t>(v0_escape_type)];

    ASSERT(text.find('\r') == std::string::npos);

    size_t escape_pos = 0;

    while( ( escape_pos = text.find_first_of("\n\\", escape_pos) ) != std::string::npos )
    {
        if( text[escape_pos] == '\n' )
        {
            text.replace(escape_pos, 1, "\\n");
            escape_pos += 2;
        }

        else if( escape_type_details.escape_backslashes )
        {
            text.replace(escape_pos, 1, "\\\\");
            escape_pos += 2;
        }

        else
        {
            ++escape_pos;
        }
    }

    return text;
}



// --------------------------------------------------------------------------
// WorkString
// --------------------------------------------------------------------------

double LogicInterpreter::ex_WorkString_evaluate(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const WorkString& work_string = GetSymbolWorkString(va_node.arguments[0]);

    return AssignString(work_string.GetSharableString());
}


double LogicInterpreter::ex_WorkString_compute(const int program_index)
{
    const auto& symbol_compute_expression_node = GetNode<Nodes::SymbolComputeExpression>(program_index);
    WorkString& work_string = GetSymbolWorkString(symbol_compute_expression_node.lhs_symbol_index);

    work_string.SetString(EvaluateSharableString(symbol_compute_expression_node.rhs_expression));

    return 0;
}



// --------------------------------------------------------------------------
// string functions
// --------------------------------------------------------------------------

template<TokenCode token_code>
double LogicInterpreter::ex_string_operators(const int program_index)
{
    // string operator evaluation: =, <>, <, <=, >=, >
    const auto& oper_node = GetNode<Nodes::Operator>(program_index);
    const SharableString lhs = EvaluateSharableString(oper_node.left_expr);
    const SharableString rhs = EvaluateSharableString(oper_node.right_expr);

    ASSERT(token_code == EngineStringComparer::StringFunctionCodeToTokenCode(static_cast<FunctionCode>(oper_node.oper)));

    if( m_usingLogicSettingsV0 )
        return EngineStringComparer::V0::Evaluate(*lhs, *rhs, token_code);

    return EngineStringComparer::V8::Evaluate<token_code>(*lhs, *rhs);
}


double LogicInterpreter::ex_string_eq(const int program_index) { return ex_string_operators<TokenCode::TOKEQOP>(program_index); }
double LogicInterpreter::ex_string_ne(const int program_index) { return ex_string_operators<TokenCode::TOKNEOP>(program_index); }
double LogicInterpreter::ex_string_lt(const int program_index) { return ex_string_operators<TokenCode::TOKLTOP>(program_index); }
double LogicInterpreter::ex_string_le(const int program_index) { return ex_string_operators<TokenCode::TOKLEOP>(program_index); }
double LogicInterpreter::ex_string_ge(const int program_index) { return ex_string_operators<TokenCode::TOKGEOP>(program_index); }
double LogicInterpreter::ex_string_gt(const int program_index) { return ex_string_operators<TokenCode::TOKGTOP>(program_index); }


double LogicInterpreter::ex_compare(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString lhs = EvaluateSharableString(fnn_node.fn_expr[0]);
    const SharableString rhs = EvaluateSharableString(fnn_node.fn_expr[1]);

    if( m_usingLogicSettingsV0 )
        return EngineStringComparer::V0::Compare(*lhs, *rhs);

    const int comparison = lhs->compare(*rhs);

    return ( comparison == 0 ) ?  0 :
           ( comparison < 0 )  ? -1 :
                                  1;
}


double LogicInterpreter::ex_compareNoCase(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    SharableString lhs = EvaluateSharableString(va_node.arguments[0]);
    SharableString rhs = EvaluateSharableString(va_node.arguments[1]);

    if( m_usingLogicSettingsV0 )
    {
        lhs.MakeLower();
        rhs.MakeLower();
        return EngineStringComparer::V0::Compare(*lhs, *rhs);
    }

    const int comparison = SO::CompareNoCase(*lhs, *rhs);

    return ( comparison == 0 ) ?  0 :
           ( comparison < 0 )  ? -1 :
                                  1;
}


double LogicInterpreter::ex_concat(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    ASSERT(fnn_node.fn_nargs >= 1);

    // short-circuit concatenating a single value
    if( fnn_node.fn_nargs == 1 )
        return AssignString(EvaluateSharableString(fnn_node.fn_expr[0]));

    std::string result = EvaluateString(fnn_node.fn_expr[0]);

    // evaluate all additional strings to determine the concatenated length
    auto additional_strings = std::make_unique<SharableString[]>(fnn_node.fn_nargs - 1);

    const size_t initial_result_length = result.length();
    size_t concatenated_length = initial_result_length;

    for( int i = 1; i < fnn_node.fn_nargs; ++i )
    {
        SharableString& this_string = additional_strings[i - 1];
        this_string = EvaluateSharableString(fnn_node.fn_expr[i]);
        concatenated_length += this_string->length();
    }

    result.resize(concatenated_length);

    char* result_data = result.data() + initial_result_length;

    for( int i = 1; i < fnn_node.fn_nargs; ++i )
    {
        SharableString& this_string = additional_strings[i - 1];
        memcpy(result_data, this_string->data(), this_string->length());
        result_data += this_string->length();
    }

    ASSERT(result_data == ( result.data() + concatenated_length ));

    return AssignString(std::move(result));
}


double LogicInterpreter::ex_ischecked(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString code = EvaluateSharableString(fnn_node.fn_expr[0]);
    const SharableString checkbox_field_value = EvaluateSharableString(fnn_node.fn_expr[1]);

    // the checkbox field has to be a multiple of the code length
    const size_t wide_code_length = SO::WideLength(*code);

    if( wide_code_length == 0 || ( SO::WideLength(*checkbox_field_value) % wide_code_length ) != 0 )
        return 0;

    std::string_view checkbox_field_value_sv = *checkbox_field_value;

    while( true )
    {
        const std::string_view this_code_sv = SO::WideSubstring(checkbox_field_value_sv, 0, wide_code_length);

        if( this_code_sv.empty() )
            return 0;

        if( *code == this_code_sv )
            return 1;

        checkbox_field_value_sv = checkbox_field_value_sv.substr(this_code_sv.length());
    }

    return 0;
}


double LogicInterpreter::ex_length(const int program_index)
{
    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(program_index);

    // a string variable (the original use of the length function)
    if( va_with_size_node.arguments[0] >= 0 )
    {
        const SharableString text = EvaluateSharableString(va_with_size_node.arguments[0]);
        return static_cast<double>(SO::WideLength(*text));
    }

    // symbols
    else
    {
        const Symbol& symbol = NPT_Ref(-1 * va_with_size_node.arguments[0]);

        // lists
        if( symbol.IsA(SymbolType::List) )
        {
            const LogicList& logic_list = assert_cast<const LogicList&>(symbol);
            return static_cast<double>(logic_list.GetCount());
        }

        // arrays
        else
        {
            return ex_Array_length(assert_cast<const LogicArray&>(symbol),
                                   Evaluate<size_t>(va_with_size_node.arguments[1]));
        }
    }
}


double LogicInterpreter::ex_pos_poschar(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);

    // 1st arg - pattern
    const SharableString pattern = EvaluateSharableString(fnn_node.fn_expr[0]);

    // 2nd arg - string to be searched
    const SharableString str = EvaluateSharableString(fnn_node.fn_expr[1]);

    const size_t pos = ( fnn_node.fn_code == FunctionCode::FNPOS_CODE ) ? str->find(*pattern) :
                                                                          str->find_first_of(*pattern);

    if( pos == std::string::npos )
        return 0;

    // return the position in wide characters, adding 1 because strings are 1-indexed
    return static_cast<double>(1 + SO::WideLength(std::string_view(str->data(), pos)));
}


double LogicInterpreter::ex_regexmatch(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    SharableString target = EvaluateSharableString(va_node.arguments[0]);
    SharableString regex = EvaluateSharableString(va_node.arguments[1]);

    target.MakeTrim();
    regex.MakeTrim();

    try
    {
        if( std::regex_match(*target, std::regex(*regex)) )
            return 1;
    }

    catch( const std::regex_error& )
    {
        IssueMessage(MessageType::Error, 100260, regex->c_str());
    }

    return 0;
}


double LogicInterpreter::ex_replace(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    std::string source = EvaluateString(fnn_node.fn_expr[0]);
    const SharableString replacement_text = EvaluateSharableString(fnn_node.fn_expr[1]);
    const SharableString new_text = EvaluateSharableString(fnn_node.fn_expr[2]);

    SO::Replace(source, *replacement_text, *new_text);

    return AssignString(std::move(source));
}


double LogicInterpreter::ex_startswith(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString starts_with_text = EvaluateSharableString(fnn_node.fn_expr[0]);
    const SharableString source_text = EvaluateSharableString(fnn_node.fn_expr[1]);

    return SO::StartsWith(*source_text, *starts_with_text);
}


double LogicInterpreter::ex_strip(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    SharableString text = EvaluateSharableString(fnn_node.fn_expr[0]);

    text.MakeTrimRight();

    return AssignString(std::move(text));
}


double LogicInterpreter::ex_tolower_toupper(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    SharableString text = EvaluateSharableString(fnn_node.fn_expr[0]);

    if( fnn_node.fn_code == FunctionCode::FNTOUPPER_CODE )
    {
        text.MakeUpper();
    }

    else
    {
        ASSERT(fnn_node.fn_code == FunctionCode::FNTOLOWER_CODE);
        text.MakeLower();
    }

    return AssignString(std::move(text));
}


double LogicInterpreter::ex_decryptstring(const int program_index)
{
    // currently this is only used to decrypt locally-declared config variables
    const auto& encryption_node = GetNode<Nodes::Encryption>(program_index);
    ASSERT(encryption_node.function_code == FunctionCode::DECRYPT_STRING_CODE);

    const SharableString encrypted_string = EvaluateSharableString(encryption_node.string_expression);

    Encryptor encryptor(encryption_node.encryption_type);
    std::string decrypted_string = encryptor.Decrypt(*encrypted_string);

    return AssignString(std::move(decrypted_string));
}


double LogicInterpreter::ex_encode(const int program_index)
{
    const auto& encode_node = GetNode<Nodes::Encode>(program_index);
    ASSERT(encode_node.encode_type != EncodeType::Default || encode_node.string_expression >= 0);

    // change the default encoding type
    if( encode_node.string_expression < 0 )
    {
        m_currentEncodeType = encode_node.encode_type;
        return AssignStringNull();
    }

    // or encode a string
    else
    {
        return AssignString(EncodeText(EvaluateSharableString(encode_node.string_expression),
                                       encode_node.encode_type));
    }
}
