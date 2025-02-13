#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include "EngineFloatingPointMath.h"


namespace
{
    inline bool PreprocessSpecialValues(const CSettings* engine_settings, double& v1, double &v2, double& result)
    {
        bool v1_is_special = IsSpecial(v1);
        bool v2_is_special = IsSpecial(v2);

        // if neither value is special, no preprocessing required
        if( !v1_is_special && !v2_is_special )
            return false;

        // if treating special values as zero, convert the values
        if( engine_settings->GetTreatSpecialValuesAsZero() )
        {
            if( v1_is_special )
                v1 = 0;

            if( v2_is_special )
                v2 = 0;

            return false;
        }

        // otherwise, return the value using the following priority: default, notappl, missing, refused
        if( v1 == DEFAULT || v2 == DEFAULT )
            result = DEFAULT;

        else if( v1 == NOTAPPL || v2 == NOTAPPL )
            result = NOTAPPL;

        else if( v1 == MISSING || v2 == MISSING )
            result = MISSING;

        else if( v1 == REFUSED || v2 == REFUSED )
            result = REFUSED;

        else
        {
            ASSERT(false);
            result = DEFAULT;
        }

        return true;
    }
}



// --------------------------------------------------------------------------
// exadd   : add two values
// --------------------------------------------------------------------------
double CIntDriver::exadd(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    double v1 = evalexpr(operator_node.left_expr);
    double v2 = evalexpr(operator_node.right_expr);
    double result;

    if( !PreprocessSpecialValues(m_pEngineSettings, v1, v2, result) )
        result = v1 + v2;

    return result;
}


// --------------------------------------------------------------------------
// exsub   : subtract two variables
// --------------------------------------------------------------------------
double CIntDriver::exsub(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    double v1 = evalexpr(operator_node.left_expr);
    double v2 = evalexpr(operator_node.right_expr);
    double result;

    if( !PreprocessSpecialValues(m_pEngineSettings, v1, v2, result) )
        result = v1 - v2;

    return result;
}


// --------------------------------------------------------------------------
// exminus : execute unary minus operator
// --------------------------------------------------------------------------
double CIntDriver::exminus(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    double v1 = evalexpr(operator_node.left_expr);

    if( !IsSpecial(v1) )
        v1 = -v1;

    return v1;
}


// --------------------------------------------------------------------------
// exmult  : multiply two variables
// --------------------------------------------------------------------------
double CIntDriver::exmult(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    double v1 = evalexpr(operator_node.left_expr);
    double v2 = evalexpr(operator_node.right_expr);
    double result;

    if( !PreprocessSpecialValues(m_pEngineSettings, v1, v2, result) )
        result = v1 * v2;

    return result;
}


// --------------------------------------------------------------------------
// exdiv   : divide two variables
// --------------------------------------------------------------------------
double CIntDriver::exdiv(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    double v1 = evalexpr(operator_node.left_expr);
    double v2 = evalexpr(operator_node.right_expr);
    double result;

    if( !PreprocessSpecialValues(m_pEngineSettings, v1, v2, result) )
        result = ( v2 == 0 ) ? DEFAULT : ( v1 / v2 );

    return result;
}


// --------------------------------------------------------------------------
// exmod   : module: v1 % v2
// --------------------------------------------------------------------------
double CIntDriver::exmod(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    double v1 = evalexpr(operator_node.left_expr);
    double v2 = evalexpr(operator_node.right_expr);
    double result;

    if( !PreprocessSpecialValues(m_pEngineSettings, v1, v2, result) )
        result = ( v2 == 0 ) ? DEFAULT : fmod(v1, v2);

    return result;
}


// --------------------------------------------------------------------------
// exexp   : execute expr ** expr
// --------------------------------------------------------------------------
double CIntDriver::exexp(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    double v1 = evalexpr(operator_node.left_expr);
    double v2 = evalexpr(operator_node.right_expr);
    double result;

    // even if treating special values as 0, it doesn't make sense to treat v2 as 0
    // because then something like 3^DEFAULT would equal 1
    if( IsSpecial(v2) )
    {
        result = m_pEngineSettings->GetTreatSpecialValuesAsZero() ? DEFAULT : v2;
    }

    else if( !PreprocessSpecialValues(m_pEngineSettings, v1, v2, result) )
    {
        result = pow(v1, v2);

        if( isnan(result) || !std::isfinite(result) ) // GHM 20131208 a way to check for NaN and infinity
            result = DEFAULT;
    }

    return result;
}


// --------------------------------------------------------------------------
// exeq    : compare two values for EQual
// --------------------------------------------------------------------------
double CIntDriver::exeq(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    return FloatingPointMath::Equals(evalexpr(operator_node.left_expr),
                                     evalexpr(operator_node.right_expr)) ? 1 : 0;
}


// --------------------------------------------------------------------------
// exne    : compare two values for Not Equal
// --------------------------------------------------------------------------
double CIntDriver::exne(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    return FloatingPointMath::Equals(evalexpr(operator_node.left_expr),
                                     evalexpr(operator_node.right_expr)) ? 0 : 1;
}


// --------------------------------------------------------------------------
// exle    : compare two values for Less or Equal
// --------------------------------------------------------------------------
double CIntDriver::exle(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    return EngineFloatingPointMath::comparison<TokenCode::TOKLEOP>(
        evalexpr(operator_node.left_expr), evalexpr(operator_node.right_expr)) ? 1 : 0;
}


// --------------------------------------------------------------------------
// exlt    : compare two values for Less Than
// --------------------------------------------------------------------------
double CIntDriver::exlt(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    return EngineFloatingPointMath::comparison<TokenCode::TOKLTOP>(
        evalexpr(operator_node.left_expr), evalexpr(operator_node.right_expr)) ? 1 : 0;
}


// --------------------------------------------------------------------------
// exge    : compare two values for Greater or Equal
// --------------------------------------------------------------------------
double CIntDriver::exge(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    return EngineFloatingPointMath::comparison<TokenCode::TOKGEOP>(
        evalexpr(operator_node.left_expr), evalexpr(operator_node.right_expr)) ? 1 : 0;
}


// --------------------------------------------------------------------------
// exgt    : compare two values for Greater Than
// --------------------------------------------------------------------------
double CIntDriver::exgt(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    return EngineFloatingPointMath::comparison<TokenCode::TOKGTOP>(
        evalexpr(operator_node.left_expr), evalexpr(operator_node.right_expr)) ? 1 : 0;
}


// --------------------------------------------------------------------------
// exequ   : compare two values for EQual
// --------------------------------------------------------------------------
double CIntDriver::exequ(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);
    double v1 = evalexpr(operator_node.left_expr);
    double v2 = evalexpr(operator_node.right_expr);

    if( IsSpecial(v1) || IsSpecial(v2) )
        return 0; // TNC Nov 16, 2001

    return FloatingPointMath::Equals(v1, v2) ? 1 : 0;
}


// --------------------------------------------------------------------------
// exor    : OR two values
// --------------------------------------------------------------------------
double CIntDriver::exor(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);

    return ( ConditionalValueIsTrue(evalexpr(operator_node.left_expr)) ||
             ConditionalValueIsTrue(evalexpr(operator_node.right_expr)) ) ? 1 : 0;
}


// --------------------------------------------------------------------------
// exnot   : NOT (complement) a logical value
// --------------------------------------------------------------------------
double CIntDriver::exnot(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);

    // special values are treated like false, NOT special value is true
    return ConditionalValueIsTrue(evalexpr(operator_node.left_expr)) ? 0 : 1;
}


// --------------------------------------------------------------------------
// exand   : AND two values
// --------------------------------------------------------------------------
double CIntDriver::exand(int iExpr)
{
    const auto& operator_node = GetNode<Nodes::Operator>(iExpr);

    return ( ConditionalValueIsTrue(evalexpr(operator_node.left_expr)) &&
             ConditionalValueIsTrue(evalexpr(operator_node.right_expr)) ) ? 1 : 0;
}


// --------------------------------------------------------------------------
// exabs: absolute value
// --------------------------------------------------------------------------
double CIntDriver::exabs(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double value = evalexpr(fnn_node.fn_expr[0]);

    return ( value >= 0 ) ? value :
                            ( -1 * value );
}


// --------------------------------------------------------------------------
// exex: exponential (with 2.71828)
// --------------------------------------------------------------------------
double CIntDriver::exex(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double value = evalexpr(fnn_node.fn_expr[0]);

    if( !IsSpecial(value) )
    {
        value = exp(value);

        // convert NaN and infinity to default
        if( isnan(value) || !std::isfinite(value) ) 
            value = DEFAULT;
    }

    return value;
}


// --------------------------------------------------------------------------
// exinc: increment a value
// --------------------------------------------------------------------------
double CIntDriver::exinc(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(va_node.arguments[0]);
    double increment_value = EvaluateOptionalNumericExpression<double>(va_node.arguments[1], 1);
    std::optional<double> return_value;

    ModifySymbolValue<double>(symbol_value_node,
        [&](double& value)
        {
            if( !PreprocessSpecialValues(m_pEngineSettings, value, increment_value, value) )
                value += increment_value;

            return_value = value;
        });

    return return_value.value_or(DEFAULT);
}


// --------------------------------------------------------------------------
// exint: return a value's integer portion
// --------------------------------------------------------------------------
double CIntDriver::exint(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double value = evalexpr(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? value :
                              floor(value);
}


// --------------------------------------------------------------------------
// exlog: logarithm
// --------------------------------------------------------------------------
double CIntDriver::exlog(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double value = evalexpr(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? value :
           ( value >= 0 )   ? log10(value) :
                              DEFAULT;
}


// --------------------------------------------------------------------------
// exlowhigh: low and high functions
// --------------------------------------------------------------------------
double CIntDriver::exlowhigh(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    bool want_minimum = ( fnn_node.fn_code == FunctionCode::FNLOW_CODE );
    std::optional<double> value;

    for( int i = 0; i < fnn_node.fn_nargs; ++i )
    {
        double this_value = evalexpr(fnn_node.fn_expr[i]);

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
// exround: round a value
// --------------------------------------------------------------------------
double CIntDriver::exround(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double value = evalexpr(fnn_node.fn_expr[0]);

    return ( value < 0 )     ? ceil(value - MAGICROUND) :
           !IsSpecial(value) ? floor(value + MAGICROUND) :
                               value;
}


// --------------------------------------------------------------------------
// exspecial: check is a value is special
// --------------------------------------------------------------------------
double CIntDriver::exspecial(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double value = evalexpr(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? 1 : 0;
}


// --------------------------------------------------------------------------
// exsqrt: square root
// --------------------------------------------------------------------------
double CIntDriver::exsqrt(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    double value = evalexpr(fnn_node.fn_expr[0]);

    return IsSpecial(value) ? value :
           ( value >= 0 )   ? sqrt(value) :
                              DEFAULT;
}
