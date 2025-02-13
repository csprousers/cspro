#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "EngineStringComparer.h"
#include <zEngineO/Array.h>
#include <zEngineO/List.h>
#include <zToolsO/RaiiHelpers.h>
#include <zToolsO/Utf8Convert.h>
#include <regex>


// --------------------------------------------------------------------------
// exstring_...: evaluate string operators: =, <>, <, <=, >=, >
// --------------------------------------------------------------------------

template<TokenCode token_code>
double CIntDriver::exstring_operators(int program_index)
{
    const auto& oper_node = GetNode<Nodes::Operator>(program_index);
    const std::wstring lhs = EvalAlphaExpr(oper_node.left_expr);
    const std::wstring rhs = EvalAlphaExpr(oper_node.right_expr);

    ASSERT(token_code == EngineStringComparer::StringFunctionCodeToTokenCode(static_cast<FunctionCode>(oper_node.oper)));

    if( m_usingLogicSettingsV0 )
        return EngineStringComparer::V0::Evaluate(lhs, rhs, token_code);

    return EngineStringComparer::V8::Evaluate<token_code>(lhs, rhs);
}


double CIntDriver::exstring_eq(int program_index) { return exstring_operators<TokenCode::TOKEQOP>(program_index); }
double CIntDriver::exstring_ne(int program_index) { return exstring_operators<TokenCode::TOKNEOP>(program_index); }
double CIntDriver::exstring_lt(int program_index) { return exstring_operators<TokenCode::TOKLTOP>(program_index); }
double CIntDriver::exstring_le(int program_index) { return exstring_operators<TokenCode::TOKLEOP>(program_index); }
double CIntDriver::exstring_ge(int program_index) { return exstring_operators<TokenCode::TOKGEOP>(program_index); }
double CIntDriver::exstring_gt(int program_index) { return exstring_operators<TokenCode::TOKGTOP>(program_index); }


// --------------------------------------------------------------------------
// excompare: execute COMPARE function
// --------------------------------------------------------------------------
double CIntDriver::excompare(int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const std::wstring lhs = EvalAlphaExpr(fnn_node.fn_expr[0]);
    const std::wstring rhs = EvalAlphaExpr(fnn_node.fn_expr[1]);

    if( m_usingLogicSettingsV0 )
        return EngineStringComparer::V0::Compare(lhs, rhs);

    const int comparison = lhs.compare(rhs);

    return ( comparison == 0 ) ?  0 :
           ( comparison < 0 )  ? -1 :
                                  1;
}


double CIntDriver::excompareNoCase(int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    std::wstring lhs = EvalAlphaExpr(va_node.arguments[0]);
    std::wstring rhs = EvalAlphaExpr(va_node.arguments[1]);

    // modify the locale (as is done in extolower_toupper)
    const auto& old_locale = std::locale::global(std::locale(""));
    const RAII::RunOnDestruction run_on_destruction([&]() { std::locale::global(old_locale); });    

    if( m_usingLogicSettingsV0 )
    {
        SO::MakeLower(lhs);
        SO::MakeLower(rhs);
        return EngineStringComparer::V0::Compare(lhs, rhs);
    }

    const int comparison = SO::CompareNoCase(lhs, rhs);

    return ( comparison == 0 ) ?  0 :
           ( comparison < 0 )  ? -1 :
                                  1;
}


// --------------------------------------------------------------------------
// exconcat: execute CONCAT function
// --------------------------------------------------------------------------
double CIntDriver::exconcat(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    ASSERT(fnn_node.fn_nargs >= 1);

    std::wstring result = EvalAlphaExpr(fnn_node.fn_expr[0]);

    for( int i = 1; i < fnn_node.fn_nargs; ++i )
        result.append(EvalAlphaExpr(fnn_node.fn_expr[i]));

    return AssignAlphaValue(std::move(result));
}


// --------------------------------------------------------------------------
// exischecked: execute ISCHECKED function
// --------------------------------------------------------------------------
double CIntDriver::exischecked(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    const std::wstring code = EvalAlphaExpr(fnn_node.fn_expr[0]);
    const std::wstring checkbox_field_value = EvalAlphaExpr(fnn_node.fn_expr[1]);

    // the checkbox field has to be a multiple of the code length
    const size_t code_length = code.length();

    if( code_length > 0 && ( checkbox_field_value.length() % code_length ) == 0 )
    {
        for( const TCHAR* checkbox_itr = checkbox_field_value.c_str(); *checkbox_itr != 0; checkbox_itr += code_length )
        {
            if( _tmemcmp(checkbox_itr, code.c_str(), code_length) == 0 )
                return 1;
        }
    }

    return 0;
}


// --------------------------------------------------------------------------
// exlength: execute LENGTH function
// --------------------------------------------------------------------------
double CIntDriver::exlength(int iExpr)
{
    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(iExpr);

    // a string variable (the original use of the length function)
    if( va_with_size_node.arguments[0] >= 0 )
    {
        return EvalAlphaExpr(va_with_size_node.arguments[0]).length();
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
            const LogicArray& logic_array = assert_cast<const LogicArray&>(symbol);
            const size_t dimension = evalexpr<size_t>(va_with_size_node.arguments[1]);

            if( dimension < 1 || dimension > logic_array.GetNumberDimensions() )
            {
                issaerror(MessageType::Error, 19041, logic_array.GetName().c_str(), dimension);
                return DEFAULT;
            }

            // don't count the 0th element in the dimension size
            return logic_array.GetDimension(dimension - 1) - 1;
        }
    }
}


// --------------------------------------------------------------------------
// expos_poschar: execute POS and POSCHAR functions
// --------------------------------------------------------------------------
double CIntDriver::expos_poschar(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);

    // 1st arg - pattern
    const std::wstring pattern = EvalAlphaExpr(fnn_node.fn_expr[0]);

    // 2nd arg - string to be searched
    const std::wstring str = EvalAlphaExpr(fnn_node.fn_expr[1]);

    const size_t pos = ( fnn_node.fn_code == FunctionCode::FNPOS_CODE ) ? str.find(pattern) :
                                                                          str.find_first_of(pattern);

    // add 1 because strings are 1-indexed
    static_assert(( std::wstring::npos ) + 1 == 0);
    return pos + 1;
}


// --------------------------------------------------------------------------
// exregexmatch: execute REGEXMATCH function
// --------------------------------------------------------------------------
double CIntDriver::exregexmatch(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    const std::wstring target = EvalAlphaExpr(va_node.arguments[0]);
    const std::wstring regex = EvalAlphaExpr(va_node.arguments[1]);

    try
    {
        if( std::regex_match(UTF8Convert::WideToUTF8(SO::Trim(target)),
                             std::regex(UTF8Convert::WideToUTF8(SO::Trim(regex))) ) )
        {
            return 1;
        }
    }

    catch( const std::regex_error& )
    {
        issaerror(MessageType::Error, 100260, regex.c_str());
    }

    return 0;
}


// --------------------------------------------------------------------------
// exreplace: execute REPLACE function
// --------------------------------------------------------------------------
double CIntDriver::exreplace(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    std::wstring source = EvalAlphaExpr(fnn_node.fn_expr[0]);
    const std::wstring replacement_text = EvalAlphaExpr(fnn_node.fn_expr[1]);
    const std::wstring new_text = EvalAlphaExpr(fnn_node.fn_expr[2]);

    SO::Replace(source, replacement_text, new_text);

    return AssignAlphaValue(std::move(source));
}


// --------------------------------------------------------------------------
// exstartswith: execute STARTSWITH function
// --------------------------------------------------------------------------
double CIntDriver::exstartswith(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    const std::wstring starts_with_text = EvalAlphaExpr(fnn_node.fn_expr[0]);
    const std::wstring source_text = EvalAlphaExpr(fnn_node.fn_expr[1]);

    return SO::StartsWith(source_text, starts_with_text);
}


// --------------------------------------------------------------------------
// exstrip: execute STRIP function
// --------------------------------------------------------------------------
double CIntDriver::exstrip(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    std::wstring text = EvalAlphaExpr(fnn_node.fn_expr[0]);

    SO::MakeTrimRight(text);

    return AssignAlphaValue(std::move(text));
}


// --------------------------------------------------------------------------
// extolower_toupper: execute TOLOWER and TOUPPER functions
// --------------------------------------------------------------------------
double CIntDriver::extolower_toupper(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    std::wstring text = EvalAlphaExpr(fnn_node.fn_expr[0]);

    // modifying the case using CString depends on the locale, and prior to
    // CSPro 8.0, the locale would always be modified and reset, which led to
    // performance problems; now only do that if non-ASCII characters are used

    // CString has been replaced with std::wstring, but the locale code has been kept
    // the same; sometime it should be tested to see if it is necessary
    constexpr TCHAR TopAsciiCharacterInUniqueRange = 0x7F;
    const auto& conversion_function = ( fnn_node.fn_code == FunctionCode::FNTOUPPER_CODE ) ? &std::towupper : &std::towlower;

    auto text_itr = text.begin();
    const auto& text_end = text.cend();

    for( ; text_itr != text_end; ++text_itr )
    {
        TCHAR& ch = *text_itr;

        if( ch > TopAsciiCharacterInUniqueRange )
            break;

        ch = (*conversion_function)(ch);
    }

    if( text_itr != text_end )
    {
        // this makes toupper correctly handle characters with accents
        // by switching from C locale which only knows ASCII chars to
        // to default locale
        const auto& old_locale = std::locale::global(std::locale(""));

        for( ; text_itr != text_end; ++text_itr )
        {
            TCHAR& ch = *text_itr;
            ch = (*conversion_function)(ch);
        }

        // set the locale back since there are some functions that rely on it
        std::locale::global(old_locale);
    }

    return AssignAlphaValue(std::move(text));
}
