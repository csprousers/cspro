#include "stdafx.h"
#include "IncludesRT.h"
#include "WorkVariable.h"
#include <engine/Nodes.h>
#include <zToolsO/FloatingPointMath.h>
#include <zUtilO/Randomizer.h>


// --------------------------------------------------------------------------
// numeric routines
// --------------------------------------------------------------------------

Engine::Value LogicInterpreter::ex_numeric_constant(const int program_index)
{
    const auto& const_node = GetNode<CONST_NODE>(program_index);
    return GetNumericConstant(const_node.const_index);
}



// --------------------------------------------------------------------------
// WorkVariable
// --------------------------------------------------------------------------

Engine::Value LogicInterpreter::ex_WorkVariable_evaluate(const int program_index)
{
    const auto& element_reference_single_node = GetNode<Nodes::ElementReferenceSingle>(program_index);
    const WorkVariable& work_variable = GetSymbolWorkVariable(element_reference_single_node.symbol_index);

    return work_variable.GetValue();
}


Engine::Value LogicInterpreter::ex_WorkVariable_compute(const int program_index)
{
    const auto& symbol_compute_expression_node = GetNode<Nodes::SymbolComputeExpression>(program_index);
    WorkVariable& work_variable = GetSymbolWorkVariable(symbol_compute_expression_node.lhs_symbol_index);

    const double rhs_value = Evaluate<double>(symbol_compute_expression_node.rhs_expression);

    work_variable.SetValue(rhs_value);

    return rhs_value;
}



// --------------------------------------------------------------------------
// math functions
// --------------------------------------------------------------------------

bool LogicInterpreter::PreprocessSpecialValues(double& v1, double &v2, double& result) const
{
    const bool v1_is_special = IsSpecial(v1);
    const bool v2_is_special = IsSpecial(v2);

    // if neither value is special, no preprocessing required
    if( !v1_is_special && !v2_is_special )
        return false;

    // if treating special values as zero, convert the values
    if( m_engineData->engine_settings.GetTreatSpecialValuesAsZero() )
    {
        if( v1_is_special )
            v1 = 0;

        if( v2_is_special )
            v2 = 0;

        return false;
    }

    // otherwise, return the value using the following priority: default, notappl, missing, refused
    if( v1 == DEFAULT || v2 == DEFAULT )
    {
        result = DEFAULT;
    }

    else if( v1 == NOTAPPL || v2 == NOTAPPL )
    {
        result = NOTAPPL;
    }

    else if( v1 == MISSING || v2 == MISSING )
    {
        result = MISSING;
    }

    else if( v1 == REFUSED || v2 == REFUSED )
    {
        result = REFUSED;
    }

    else
    {
        ASSERT(false);
        result = DEFAULT;
    }

    return true;
}


// --------------------------------------------------------------------------
// ex_add: add two values
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_add(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate<double>(operator_node.left_expr);
    double v2 = Evaluate<double>(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    return v1 + v2;
}


// --------------------------------------------------------------------------
// ex_sub: subtract two values
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_sub(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate<double>(operator_node.left_expr);
    double v2 = Evaluate<double>(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    return v1 - v2;
}


// --------------------------------------------------------------------------
// ex_minus: execute unary minus operator
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_minus(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    const double v1 = Evaluate<double>(operator_node.left_expr);

    if( IsSpecial(v1) )
        return v1;

    return -1 * v1;
}


// --------------------------------------------------------------------------
// ex_mult: multiply two values
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_mult(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate<double>(operator_node.left_expr);
    double v2 = Evaluate<double>(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    return v1 * v2;
}


// --------------------------------------------------------------------------
// ex_div: divide two values
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_div(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate<double>(operator_node.left_expr);
    double v2 = Evaluate<double>(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    if( v2 == 0 )
        return Engine::Value::Invalid<double>();

    return v1 / v2;
}


// --------------------------------------------------------------------------
// ex_mod: modulo: v1 % v2
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_mod(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate<double>(operator_node.left_expr);
    double v2 = Evaluate<double>(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    if( v2 == 0 )
        return Engine::Value::Invalid<double>();

    return fmod(v1, v2);
}


// --------------------------------------------------------------------------
// ex_exp: expr ** expr
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_exp(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate<double>(operator_node.left_expr);
    double v2 = Evaluate<double>(operator_node.right_expr);
    double result;

    // even if treating special values as 0, it doesn't make sense to treat v2 as 0
    // because then something like 3^DEFAULT would equal 1
    if( IsSpecial(v2) )
    {
        if( m_engineData->engine_settings.GetTreatSpecialValuesAsZero() )
            return Engine::Value::Invalid<double>();

        return v2;
    }

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    result = pow(v1, v2);

    if( isnan(result) || !std::isfinite(result) ) // 20131208 a way to check for NaN and infinity
        return Engine::Value::Invalid<double>();

    return result;
}


// --------------------------------------------------------------------------
// ex_eq: compare two values for EQual
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_eq(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return Engine::Value::Bool(
        FloatingPointMath::Evaluate<TokenCode::TOKEQOP>(
            Evaluate<double>(operator_node.left_expr),
            Evaluate<double>(operator_node.right_expr)
        )
    );
}


// --------------------------------------------------------------------------
// ex_ne: compare two values for Not Equal
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_ne(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return Engine::Value::Bool(
        FloatingPointMath::Evaluate<TokenCode::TOKNEOP>(
            Evaluate<double>(operator_node.left_expr),
            Evaluate<double>(operator_node.right_expr)
        )
    );
}


// --------------------------------------------------------------------------
// ex_le: compare two values for Less or Equal
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_le(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return Engine::Value::Bool(
        FloatingPointMath::Evaluate<TokenCode::TOKLEOP>(
            Evaluate<double>(operator_node.left_expr),
            Evaluate<double>(operator_node.right_expr)
        )
    );
}


// --------------------------------------------------------------------------
// ex_lt: compare two values for Less Than
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_lt(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return Engine::Value::Bool(
        FloatingPointMath::Evaluate<TokenCode::TOKLTOP>(
            Evaluate<double>(operator_node.left_expr),
            Evaluate<double>(operator_node.right_expr)
        )
    );
}


// --------------------------------------------------------------------------
// ex_ge: compare two values for Greater or Equal
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_ge(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return Engine::Value::Bool(
        FloatingPointMath::Evaluate<TokenCode::TOKGEOP>(
            Evaluate<double>(operator_node.left_expr),
            Evaluate<double>(operator_node.right_expr)
        )
    );
}


// --------------------------------------------------------------------------
// ex_gt: compare two values for Greater Than
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_gt(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return Engine::Value::Bool(
        FloatingPointMath::Evaluate<TokenCode::TOKGTOP>(
            Evaluate<double>(operator_node.left_expr),
            Evaluate<double>(operator_node.right_expr)
        )
    );
}


// --------------------------------------------------------------------------
// ex_equ: compare two values for EQual using <=>
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_equ(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    const double v1 = Evaluate<double>(operator_node.left_expr);
    const double v2 = Evaluate<double>(operator_node.right_expr);

    if( IsSpecial(v1) || IsSpecial(v2) )
        return Engine::Value::Bool(false); // TNC Nov 16, 2001

    return Engine::Value::Bool(
        FloatingPointMath::Equals(v1, v2)
    );
}


// --------------------------------------------------------------------------
// ex_or: OR two values
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_or(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return Engine::Value::Bool(
        ( EvaluateConditional(operator_node.left_expr) ||
          EvaluateConditional(operator_node.right_expr) )
    );
}


// --------------------------------------------------------------------------
// ex_not: NOT (complement) a logical value
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_not(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    // special values are treated like false, NOT special value is true
    return Engine::Value::Bool(!EvaluateConditional(operator_node.left_expr));
}


// --------------------------------------------------------------------------
// ex_and: AND two values
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_and(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return Engine::Value::Bool(
        ( EvaluateConditional(operator_node.left_expr) &&
          EvaluateConditional(operator_node.right_expr) )
    );
}


// --------------------------------------------------------------------------
// ex_abs: absolute value
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_abs(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate<double>(fnn_node.fn_expr[0]);

    if( value < 0 )
    {
        ASSERT(!IsSpecial(value));
        return -1 * value;
    }

    return value;
}


// --------------------------------------------------------------------------
// ex_ex: exponential (with 2.71828)
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_ex(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    double value = Evaluate<double>(fnn_node.fn_expr[0]);

    if( !IsSpecial(value) )
    {
        value = exp(value);

        // convert NaN and infinity to default
        if( isnan(value) || !std::isfinite(value) )
            return Engine::Value::Invalid<double>();
    }

    return value;
}


// --------------------------------------------------------------------------
// ex_inc: increment a value
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_inc(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(va_node.arguments[0]);
    double increment_value = EvaluateOptional<double>(va_node.arguments[1], 1);

    return ModifySymbolValue<double>(symbol_value_node,
        [&](double& value)
        {
            if( !PreprocessSpecialValues(value, increment_value, value) )
                value += increment_value;

            return value;
        });
}


// --------------------------------------------------------------------------
// ex_int: return a value's integer portion
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_int(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate<double>(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? value :
                              floor(value);
}


// --------------------------------------------------------------------------
// ex_log: logarithm
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_log(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate<double>(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? Engine::Value(value) :
           ( value >= 0 )   ? Engine::Value(log10(value)) :
                              Engine::Value::Invalid<double>();
}


// --------------------------------------------------------------------------
// ex_low_high: low and high functions
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_low_high(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const bool want_minimum = ( fnn_node.fn_code == FunctionCode::FNLOW_CODE );
    std::optional<double> value;

    for( int i = 0; i < fnn_node.fn_nargs; ++i )
    {
        const double this_value = Evaluate<double>(fnn_node.fn_expr[i]);

        if( !IsSpecial(this_value) )
        {
            value = !value.has_value() ? this_value :
                    want_minimum       ? std::min(this_value, *value) :
                                         std::max(this_value, *value);
        }
    }

    return value.has_value() ? Engine::Value(*value) :
                               Engine::Value::Invalid<double>();
}


// --------------------------------------------------------------------------
// ex_round: round a value
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_round(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate<double>(fnn_node.fn_expr[0]);

    return ( value < 0 )     ? ceil(value - MAGICROUND) :
           !IsSpecial(value) ? floor(value + MAGICROUND) :
                               value;
}


// --------------------------------------------------------------------------
// ex_special: check is a value is special
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_special(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);

    return Engine::Value::Bool(
        IsSpecial(Evaluate<double>(fnn_node.fn_expr[0]))
    );
}


// --------------------------------------------------------------------------
// ex_sqrt: square root
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_sqrt(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate<double>(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? Engine::Value(value) :
           ( value >= 0 )   ? Engine::Value(sqrt(value)) :
                              Engine::Value::Invalid<double>();
}


// --------------------------------------------------------------------------
// ex_seed: seed the random number generator
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_seed(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double seed_value = Evaluate<double>(fnn_node.fn_expr[0]);

    if( IsSpecial(seed_value) )
        return Engine::Value::Bool(false);

    Randomizer::Seed(static_cast<uint32_t>(seed_value));

    return Engine::Value::Bool(true);
}


// --------------------------------------------------------------------------
// ex_random: generate a random number
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_random(const int program_index)
{
    const auto& function_node = GetNode<FNN_NODE>(program_index);
    const double low_value = Evaluate<double>(function_node.fn_expr[0]);
    const double high_value = Evaluate<double>(function_node.fn_expr[1]);

    if( low_value > high_value || IsSpecial(low_value) || IsSpecial(high_value) )
        return Engine::Value::Invalid<double>();

    int64_t int_low_value = static_cast<int64_t>(low_value);
    const int64_t difference = static_cast<int64_t>(high_value) - int_low_value;

    if( difference != 0 )
        int_low_value += static_cast<int64_t>(Randomizer::Next() * ( difference + 1 ) );

    return Engine::Value::Integer(int_low_value);
}


// --------------------------------------------------------------------------
// ex_tonumber: convert a string to a number
// --------------------------------------------------------------------------
Engine::Value LogicInterpreter::ex_tonumber(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString number_string = Evaluate<SharableString>(fnn_node.fn_expr[0]);
    const std::string_view trimmed_number_string_sv = SO::Trim(*number_string);

    constexpr char DecimalSeparator = '.';

    enum class Sign { None, Negative, Positive };
    Sign sign = Sign::None;
    const char* start_number_section = nullptr;
    bool reached_decimal_area = false;
    size_t length_number_section = 0;

    for( const char& ch : trimmed_number_string_sv )
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

        else if( start_number_section == nullptr && ch == ' ' )
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
        return SO::EqualsNoCase(trimmed_number_string_sv, "true")  ? Engine::Value(1) :
               SO::EqualsNoCase(trimmed_number_string_sv, "false") ? Engine::Value(0) :
                                                                     Engine::Value::Invalid<double>();
    }

    // get just the string portion
    const double value = atod(std::string_view(start_number_section, length_number_section));

    return ( value == IMSA_BAD_DOUBLE || IsSpecial(value) ) ? Engine::Value::Invalid<double>() :
           ( sign == Sign::Negative )                       ? Engine::Value(-1 * value) :
                                                              Engine::Value(value);
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


Engine::Value CIntDriver::exedit(const int iExpr)
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

    return UTF8_TODO::GetUtf8(edit_result);
}
