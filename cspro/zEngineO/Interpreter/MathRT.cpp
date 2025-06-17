#include "stdafx.h"
#include "IncludesRT.h"
#include "WorkVariable.h"
#include <engine/Nodes.h>
#include <zToolsO/FloatingPointMath.h>
#include <zUtilO/Randomizer.h>


// --------------------------------------------------------------------------
// numeric routines
// --------------------------------------------------------------------------

double LogicInterpreter::ex_numeric_constant(const int program_index)
{
    const auto& const_node = GetNode<CONST_NODE>(program_index);
    return GetNumericConstant(const_node.const_index);
}



// --------------------------------------------------------------------------
// WorkVariable
// --------------------------------------------------------------------------

double LogicInterpreter::ex_WorkVariable_evaluate(const int program_index)
{
    const auto& svar_node = GetNode<SVAR_NODE>(program_index);
    return GetSymbolWorkVariable(svar_node.m_iVarIndex).GetValue();
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
double LogicInterpreter::ex_add(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate(operator_node.left_expr);
    double v2 = Evaluate(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    return v1 + v2;
}


// --------------------------------------------------------------------------
// ex_sub: subtract two values
// --------------------------------------------------------------------------
double LogicInterpreter::ex_sub(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate(operator_node.left_expr);
    double v2 = Evaluate(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    return v1 - v2;
}


// --------------------------------------------------------------------------
// ex_minus: execute unary minus operator
// --------------------------------------------------------------------------
double LogicInterpreter::ex_minus(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    const double v1 = Evaluate(operator_node.left_expr);

    if( IsSpecial(v1) )
        return v1;

    return -1 * v1;
}


// --------------------------------------------------------------------------
// ex_mult: multiply two values
// --------------------------------------------------------------------------
double LogicInterpreter::ex_mult(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate(operator_node.left_expr);
    double v2 = Evaluate(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    return v1 * v2;
}


// --------------------------------------------------------------------------
// ex_div: divide two values
// --------------------------------------------------------------------------
double LogicInterpreter::ex_div(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate(operator_node.left_expr);
    double v2 = Evaluate(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    if( v2 == 0 )
        return DEFAULT;

    return v1 / v2;
}


// --------------------------------------------------------------------------
// ex_mod: modulo: v1 % v2
// --------------------------------------------------------------------------
double LogicInterpreter::ex_mod(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate(operator_node.left_expr);
    double v2 = Evaluate(operator_node.right_expr);
    double result;

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    if( v2 == 0 )
        return DEFAULT;

    return fmod(v1, v2);
}


// --------------------------------------------------------------------------
// ex_exp: expr ** expr
// --------------------------------------------------------------------------
double LogicInterpreter::ex_exp(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    double v1 = Evaluate(operator_node.left_expr);
    double v2 = Evaluate(operator_node.right_expr);
    double result;

    // even if treating special values as 0, it doesn't make sense to treat v2 as 0
    // because then something like 3^DEFAULT would equal 1
    if( IsSpecial(v2) )
    {
        return m_engineData->engine_settings.GetTreatSpecialValuesAsZero() ? DEFAULT :
                                                                             v2;
    }

    if( PreprocessSpecialValues(v1, v2, result) )
        return result;

    result = pow(v1, v2);

    if( isnan(result) || !std::isfinite(result) ) // 20131208 a way to check for NaN and infinity
        return DEFAULT;

    return result;
}


// --------------------------------------------------------------------------
// ex_eq: compare two values for EQual
// --------------------------------------------------------------------------
double LogicInterpreter::ex_eq(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    return FloatingPointMath::Evaluate<TokenCode::TOKEQOP>(Evaluate(operator_node.left_expr),
                                                           Evaluate(operator_node.right_expr));
}


// --------------------------------------------------------------------------
// ex_ne: compare two values for Not Equal
// --------------------------------------------------------------------------
double LogicInterpreter::ex_ne(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    return FloatingPointMath::Evaluate<TokenCode::TOKNEOP>(Evaluate(operator_node.left_expr),
                                                           Evaluate(operator_node.right_expr));
}


// --------------------------------------------------------------------------
// ex_le: compare two values for Less or Equal
// --------------------------------------------------------------------------
double LogicInterpreter::ex_le(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    return FloatingPointMath::Evaluate<TokenCode::TOKLEOP>(Evaluate(operator_node.left_expr),
                                                           Evaluate(operator_node.right_expr));
}


// --------------------------------------------------------------------------
// ex_lt: compare two values for Less Than
// --------------------------------------------------------------------------
double LogicInterpreter::ex_lt(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    return FloatingPointMath::Evaluate<TokenCode::TOKLTOP>(Evaluate(operator_node.left_expr),
                                                           Evaluate(operator_node.right_expr));
}


// --------------------------------------------------------------------------
// ex_ge: compare two values for Greater or Equal
// --------------------------------------------------------------------------
double LogicInterpreter::ex_ge(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    return FloatingPointMath::Evaluate<TokenCode::TOKGEOP>(Evaluate(operator_node.left_expr),
                                                           Evaluate(operator_node.right_expr));
}


// --------------------------------------------------------------------------
// ex_gt: compare two values for Greater Than
// --------------------------------------------------------------------------
double LogicInterpreter::ex_gt(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    return FloatingPointMath::Evaluate<TokenCode::TOKGTOP>(Evaluate(operator_node.left_expr),
                                                           Evaluate(operator_node.right_expr));
}


// --------------------------------------------------------------------------
// ex_equ: compare two values for EQual using <=>
// --------------------------------------------------------------------------
double LogicInterpreter::ex_equ(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);
    const double v1 = Evaluate(operator_node.left_expr);
    const double v2 = Evaluate(operator_node.right_expr);

    if( IsSpecial(v1) || IsSpecial(v2) )
        return 0; // TNC Nov 16, 2001

    return FloatingPointMath::Equals(v1, v2);
}


// --------------------------------------------------------------------------
// ex_or: OR two values
// --------------------------------------------------------------------------
double LogicInterpreter::ex_or(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return ( EvaluateConditional(operator_node.left_expr) ||
             EvaluateConditional(operator_node.right_expr) );
}


// --------------------------------------------------------------------------
// ex_not: NOT (complement) a logical value
// --------------------------------------------------------------------------
double LogicInterpreter::ex_not(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    // special values are treated like false, NOT special value is true
    return !EvaluateConditional(operator_node.left_expr);
}


// --------------------------------------------------------------------------
// ex_and: AND two values
// --------------------------------------------------------------------------
double LogicInterpreter::ex_and(const int program_index)
{
    const auto& operator_node = GetNode<Nodes::Operator>(program_index);

    return ( EvaluateConditional(operator_node.left_expr) &&
             EvaluateConditional(operator_node.right_expr) );
}


// --------------------------------------------------------------------------
// ex_abs: absolute value
// --------------------------------------------------------------------------
double LogicInterpreter::ex_abs(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate(fnn_node.fn_expr[0]);

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
double LogicInterpreter::ex_ex(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    double value = Evaluate(fnn_node.fn_expr[0]);

    if( !IsSpecial(value) )
    {
        value = exp(value);

        // convert NaN and infinity to default
        if( isnan(value) || !std::isfinite(value) )
            return DEFAULT;
    }

    return value;
}


// --------------------------------------------------------------------------
// ex_inc: increment a value
// --------------------------------------------------------------------------
double LogicInterpreter::ex_inc(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(va_node.arguments[0]);
    double increment_value = EvaluateOptional(va_node.arguments[1], 1);
    double return_value = DEFAULT;

    // INTERPRETER_DLL_TODO change to: ModifySymbolValue<double>(symbol_value_node,
    ModifySymbolValue_double_INTERPRETER_DLL_TODO(symbol_value_node,
        [&](double& value)
        {
            if( !PreprocessSpecialValues(value, increment_value, value) )
                value += increment_value;

            return_value = value;
        });

    return return_value;
}


// --------------------------------------------------------------------------
// ex_int: return a value's integer portion
// --------------------------------------------------------------------------
double LogicInterpreter::ex_int(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? value :
                              floor(value);
}


// --------------------------------------------------------------------------
// ex_log: logarithm
// --------------------------------------------------------------------------
double LogicInterpreter::ex_log(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? value :
           ( value >= 0 )   ? log10(value) :
                              DEFAULT;
}


// --------------------------------------------------------------------------
// ex_low_high: low and high functions
// --------------------------------------------------------------------------
double LogicInterpreter::ex_low_high(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const bool want_minimum = ( fnn_node.fn_code == FunctionCode::FNLOW_CODE );
    std::optional<double> value;

    for( int i = 0; i < fnn_node.fn_nargs; ++i )
    {
        const double this_value = Evaluate(fnn_node.fn_expr[i]);

        if( !IsSpecial(this_value) )
        {
            value = !value.has_value() ? this_value :
                    want_minimum       ? std::min(this_value, *value) :
                                         std::max(this_value, *value);
        }
    }

    return value.value_or(DEFAULT);
}


// --------------------------------------------------------------------------
// ex_round: round a value
// --------------------------------------------------------------------------
double LogicInterpreter::ex_round(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate(fnn_node.fn_expr[0]);

    return ( value < 0 )     ? ceil(value - MAGICROUND) :
           !IsSpecial(value) ? floor(value + MAGICROUND) :
                               value;
}


// --------------------------------------------------------------------------
// ex_special: check is a value is special
// --------------------------------------------------------------------------
double LogicInterpreter::ex_special(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    return IsSpecial(Evaluate(fnn_node.fn_expr[0]));
}


// --------------------------------------------------------------------------
// ex_sqrt: square root
// --------------------------------------------------------------------------
double LogicInterpreter::ex_sqrt(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double value = Evaluate(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? value :
           ( value >= 0 )   ? sqrt(value) :
                              DEFAULT;
}


// --------------------------------------------------------------------------
// ex_seed: seed the random number generator
// --------------------------------------------------------------------------
double LogicInterpreter::ex_seed(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const double seed_value = Evaluate(fnn_node.fn_expr[0]);

    if( IsSpecial(seed_value) )
        return 0;

    Randomizer::Seed(static_cast<uint32_t>(seed_value));

    return 1;
}


// --------------------------------------------------------------------------
// ex_random: generate a random number
// --------------------------------------------------------------------------
double LogicInterpreter::ex_random(const int program_index)
{
    const auto& function_node = GetNode<FNN_NODE>(program_index);
    const double low_value = Evaluate(function_node.fn_expr[0]);
    const double high_value = Evaluate(function_node.fn_expr[1]);

    if( low_value > high_value || IsSpecial(low_value) || IsSpecial(high_value) )
        return DEFAULT;

    int64_t int_low_value = static_cast<int64_t>(low_value);
    const int64_t difference = static_cast<int64_t>(high_value) - int_low_value;

    if( difference != 0 )
        int_low_value += static_cast<int64_t>(Randomizer::Next() * ( difference + 1 ) );

    return static_cast<double>(int_low_value);
}


// --------------------------------------------------------------------------
// ex_tonumber: convert a string to a number
// --------------------------------------------------------------------------
double LogicInterpreter::ex_tonumber(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString number_string = EvaluateSharableString(fnn_node.fn_expr[0]);
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
        return SO::EqualsNoCase(trimmed_number_string_sv, "true")  ? 1 :
               SO::EqualsNoCase(trimmed_number_string_sv, "false") ? 0 :
                                                                     DEFAULT;
    }

    // get just the string portion
    const double value = atod(std::string_view(start_number_section, length_number_section));

    return ( value == IMSA_BAD_DOUBLE || IsSpecial(value) ) ? DEFAULT :
           ( sign == Sign::Negative )                       ? ( -1 * value ) :
                                                              value;
}
