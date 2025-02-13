#include "stdafx.h"
#include "UserFunction.h"


// --------------------------------------------------------------------------
// UserFunction
// --------------------------------------------------------------------------

UserFunction::UserFunction(std::wstring user_function_name, EngineData& engine_data)
    :   Symbol(std::move(user_function_name), SymbolType::UserFunction),
        m_engineData(engine_data),
        m_programIndex(-1),
        m_returnType(SymbolType::WorkVariable),
        m_returnPaddingStringLength(0),
        m_sqlCallbackFunction(false),
        m_returnValue(0.0),
        m_functionCallCount(0)
{
}


void UserFunction::AddParameterSymbol(const Symbol& parameter_symbol)
{
    m_parameterSymbols.emplace_back(parameter_symbol.GetSymbolIndex());
}


Symbol& UserFunction::GetParameterSymbol(const size_t parameter_number)
{
    return m_engineData.symbol_table.GetAt(m_parameterSymbols[parameter_number]);
}


void UserFunction::Reset()
{
    if( m_returnType == SymbolType::WorkVariable )
    {
        m_returnValue = DEFAULT;
    }

    else
    {
        if( !std::holds_alternative<std::wstring>(m_returnValue) )
        {
            m_returnValue.emplace<std::wstring>();
        }

        else
        {
            std::get<std::wstring>(m_returnValue).clear();
        }        
    }
}


void UserFunction::SetReturnValue(std::variant<double, std::wstring> return_value)
{
    m_returnValue = std::move(return_value);

    if( std::holds_alternative<std::wstring>(m_returnValue) && m_returnPaddingStringLength != 0 )
        SO::MakeExactLength(std::get<std::wstring>(m_returnValue), m_returnPaddingStringLength);
}


void UserFunction::serialize_subclass(Serializer& ar)
{
    ar & m_programIndex;

    ar.SerializeEnum(m_returnType);

    if( ar.PredatesVersionIteration(Serializer::Iteration_7_6_000_1) )
    {
        if( m_returnType == SymbolType::Variable )
            m_returnType = SymbolType::WorkString;
    }

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
