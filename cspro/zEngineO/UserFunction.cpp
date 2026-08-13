#include "stdafx.h"
#include "UserFunction.h"


// --------------------------------------------------------------------------
// UserFunction
// --------------------------------------------------------------------------

UserFunction::UserFunction(std::string user_function_name, EngineData& engine_data)
    :   Symbol(std::move(user_function_name), SymbolType::UserFunction),
        m_engineData(engine_data),
        m_programIndex(-1),
        m_returnType(SymbolType::WorkVariable),
        m_returnPaddingStringLength(0),
        m_sqlCallbackFunction(false),
        m_functionCallCount(0)
{
}


void UserFunction::SetParameters(std::vector<int> parameter_symbol_indices, std::vector<int> parameter_default_values)
{
    ASSERT(parameter_default_values.size() <= parameter_symbol_indices.size());

    m_parameterSymbols = std::move(parameter_symbol_indices);

    // whena a function is declared but not defined, the default values should be kept from the declaration
    if( m_parameterDefaultValues.empty() )
        m_parameterDefaultValues = std::move(parameter_default_values);

    ASSERT(m_parameterDefaultValues.size() <= m_parameterSymbols.size());
}


Symbol& UserFunction::GetParameterSymbol(const size_t parameter_number)
{
    return m_engineData.symbol_table.GetAt(m_parameterSymbols[parameter_number]);
}


std::vector<SymbolType> UserFunction::GetParameterSymbolTypes() const
{
    std::vector<SymbolType> symbol_types;

    for( const int symbol_index : m_parameterSymbols )
        symbol_types.emplace_back(m_engineData.symbol_table.GetAt(symbol_index).GetType());

    return symbol_types;
}


void UserFunction::CompareDeclarationAttributes(const Symbol& symbol) const
{
    const UserFunction& user_function = assert_cast<const UserFunction&>(symbol);

    CompareDeclarationAttributes(user_function.m_returnType, user_function.m_returnPaddingStringLength,
                                 user_function.m_sqlCallbackFunction);
}


void UserFunction::CompareDeclarationAttributes(const SymbolType return_type, const int return_padding_string_length,
                                                const bool sql_callback_function) const
{
    if( m_returnType != return_type )
    {
        throw CompareDeclarationAttributesException("return type: %s vs. %s", ToDisplayString(m_returnType),
                                                                              ToDisplayString(return_type));
    }

    if( m_returnPaddingStringLength != return_padding_string_length )
    {
        const bool this_is_alpha = ( m_returnPaddingStringLength == 0 );

        if( this_is_alpha || return_padding_string_length == 0 )
        {
            throw CompareDeclarationAttributesException("return type: %s vs. %s", this_is_alpha ? "alpha" : "string",
                                                                                  this_is_alpha ? "string" : "alpha");
        }

        throw CompareDeclarationAttributesException("return type alpha length: %d vs. %d", static_cast<int>(m_returnPaddingStringLength),
                                                                                           static_cast<int>(return_padding_string_length));
    }

    if( m_sqlCallbackFunction != sql_callback_function )
        throw CompareDeclarationAttributesException("SQL callback flag");

    // the parameters are compared in LogicCompiler::CompileUserFunctionParameters
}


void UserFunction::Reset()
{
    m_returnValue.reset();
}


Engine::Value UserFunction::PreprocessReturnValue(Engine::Value return_value) const
{
    ASSERT(( m_returnType == SymbolType::WorkVariable && return_value.is<double>() ) ||
           ( m_returnType == SymbolType::WorkString && return_value.is<SharableString>() ));

    if( return_value.is<SharableString>() && m_returnPaddingStringLength != 0 )
        return_value.get<SharableString>().WideMakeExactLength(m_returnPaddingStringLength);

    return return_value;
}


void UserFunction::SetReturnValue(Engine::Value return_value)
{
    m_returnValue = PreprocessReturnValue(std::move(return_value));
}


Engine::Value UserFunction::GetReturnValue() const
{
    if( m_returnValue.has_value() )
        return *m_returnValue;

    return PreprocessReturnValue(Engine::Value::Invalid(GetReturnDataType()));
}


void UserFunction::serialize_subclass(Serializer& ar)
{
    ar & m_programIndex;

    ar.SerializeEnum(m_returnType);

    ar & m_returnPaddingStringLength
       & m_sqlCallbackFunction
       & m_parameterSymbols
       & m_parameterDefaultValues;

    ar.IgnoreUnusedVariable<std::vector<int>>(Serializer::Iteration_7_7_000_2); // m_functionParameterTypes

    ar.IgnoreUnusedVariable<bool>(Serializer::Iteration_7_7_000_2); // m_requiresLocalVariablesReset

    if( ar.MeetsVersionIteration(Serializer::Iteration_8_0_000_1) )
    {
        ar & m_functionBodySymbols;
    }

    else
    {
        size_t m_numberSymbolsDeclaredInFunctionScope = ar.Read<size_t>();
        int symbol_index = GetSymbolIndex() + m_parameterSymbols.size();

        for( ; m_numberSymbolsDeclaredInFunctionScope > 0; --m_numberSymbolsDeclaredInFunctionScope )
            m_functionBodySymbols.emplace_back(++symbol_index);
    }
}


void UserFunction::WriteJsonMetadata_subclass(JsonWriter& json_writer) const
{
    json_writer.Write(JK::sql, m_sqlCallbackFunction);

    json_writer.Key(JK::returnType).WriteObject(
        [&]()
        {
            json_writer.Write(JK::type, m_returnType);

            if( m_returnPaddingStringLength != 0 )
            {
                json_writer.Write(JK::subtype, SymbolSubType::WorkAlpha)
                           .Write(JK::length, m_returnPaddingStringLength);
            }
        });

    // parameters
    {
        json_writer.BeginArray(JK::parameters);

        for( size_t i = 0; i < m_parameterSymbols.size(); ++i )
        {
            const Symbol& symbol = m_engineData.symbol_table.GetAt(m_parameterSymbols[i]);

            json_writer.BeginObject();

            json_writer.Key(JK::symbol);
            symbol.WriteJson(json_writer, SymbolJsonOutput::Metadata);

            json_writer.Write(JK::optional, ( i >= GetNumberRequiredParameters() ));

            json_writer.EndObject();
        }

        json_writer.EndArray();
    }
}
