#include "stdafx.h"
#include "IncludesRT.h"
#include "Array.h"
#include "List.h"
#include "StringComparer.h"
#include "WorkString.h"
#include "Nodes/Encryption.h"
#include "Nodes/Strings.h"
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
        case DataType::Numeric: return DoubleToString(Evaluate(program_index));
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
    return m_workingStrings.size() - 1;
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


double LogicInterpreter::ex_WorkString_assign(const int program_index)
{
    const auto& symbol_reset_node = GetNode<Nodes::SymbolReset>(program_index);
    WorkString& work_string = GetSymbolWorkString(symbol_reset_node.symbol_index);

    work_string.SetString(EvaluateSharableString(symbol_reset_node.initialize_value));

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
        return SO::WideLength(*text);
    }

    // symbols
    else
    {
        const Symbol& symbol = NPT_Ref(-1 * va_with_size_node.arguments[0]);

        // lists
        if( symbol.IsA(SymbolType::List) )
        {
            const LogicList& logic_list = assert_cast<const LogicList&>(symbol);
            return logic_list.GetCount();
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
    return 1 + SO::WideLength(std::string_view(str->data(), pos));
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
    ASSERT(encode_node.encoding_type != Nodes::EncodeType::Default || encode_node.string_expression >= 0);

    // change the default encoding type
    if( encode_node.string_expression < 0 )
    {
        m_currentEncodeType = encode_node.encoding_type;
        return AssignStringNull();
    }

    // or encode a string
    const Nodes::EncodeType encoding_type = ( encode_node.encoding_type == Nodes::EncodeType::Default ) ? m_currentEncodeType :
                                                                                                          encode_node.encoding_type;
    SharableString text = EvaluateSharableString(encode_node.string_expression);
    std::unique_ptr<std::string> encoded_text;

    switch( encoding_type )
    {
        case Nodes::EncodeType::Html:
            encoded_text = Encoders::ToHtmlWorker(*text);
            break;

        case Nodes::EncodeType::Csv:
            encoded_text = Encoders::ToCsvWorker(*text);
            break;

        case Nodes::EncodeType::PercentEncoding:
            encoded_text = Encoders::ToPercentEncodingWorker(*text);
            break;

        case Nodes::EncodeType::Uri:
            encoded_text = Encoders::ToUriWorker(*text);
            break;

        case Nodes::EncodeType::UriComponent:
            encoded_text = Encoders::ToUriComponentWorker(*text);
            break;

        case Nodes::EncodeType::Slashes:
            return AssignString(Encoders::ToEscapedString(text.Release()));

        case Nodes::EncodeType::JsonString:
            return AssignString(Encoders::ToJsonString(*text));

        case Nodes::EncodeType::Markdown:
            encoded_text = Encoders::ToMarkdownWorker(*text);
            break;

        default:
            ASSERT(false);
            break;
    }

    return ( encoded_text != nullptr ) ? AssignString(std::move(encoded_text)) :
                                         AssignString(std::move(text));
}
