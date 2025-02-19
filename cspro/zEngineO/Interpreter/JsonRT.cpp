#include "stdafx.h"
#include "IncludesRT.h"
#include "SymbolSerializerHelper.h"

namespace JsonRT { class EngineJsonReaderInterface; class EngineSymbolSerializerHelper; }


class JsonRT::EngineJsonReaderInterface : public JsonReaderInterface
{
public:
    EngineJsonReaderInterface(LogicInterpreter& interpreter)
        :   m_interpreter(interpreter)
    {
        m_directory = m_interpreter.GetCurrentWorkingDirectory();
    }

protected:
    void OnLogWarning(const std::string message) override
    {
        m_interpreter.IssueMessage(MessageType::Warning, MGF::OpenMessage_32001, message.c_str());
    }

private:
    LogicInterpreter& m_interpreter;
};


JsonReaderInterface* LogicInterpreter::GetEngineJsonReaderInterface()
{
    if( m_engineJsonReaderInterface == nullptr )
        m_engineJsonReaderInterface = std::make_unique<JsonRT::EngineJsonReaderInterface>(*this);

    return m_engineJsonReaderInterface.get();
}


class JsonRT::EngineSymbolSerializerHelper : public SymbolSerializerHelper
{
public:
    EngineSymbolSerializerHelper(LogicInterpreter& interpreter, const JsonProperties& json_properties)
        :   SymbolSerializerHelper(json_properties),
            m_interpreter(interpreter)
    {
    }

    std::string LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol) override
    {
        return m_interpreter.LocalhostCreateMappingForBinarySymbol(binary_symbol);
    }

private:
    LogicInterpreter& m_interpreter;
};


std::string LogicInterpreter::GetSymbolJson(const Symbol& symbol, const Symbol::SymbolJsonOutput symbol_json_output, const JsonNode* const serialization_options_node)
{
    cs::shared_or_raw_ptr<JsonProperties> json_properties;

    if( m_engineData->application != nullptr )
    {
        // parse any serialization properties specified (on top of the application's default properties)...
        if( serialization_options_node != nullptr )
        {
            json_properties = std::make_unique<JsonProperties>(m_engineData->application->GetApplicationProperties().GetJsonProperties());
            json_properties->UpdateFromJson(*serialization_options_node);
        }

        // ...or use the application's properties
        else
        {
            json_properties = &m_engineData->application->GetApplicationProperties().GetJsonProperties();
        }
    }

    else
    {
        ASSERT(false);
        json_properties = std::make_unique<JsonProperties>();
    }

    const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter(( json_properties->GetJsonFormat() == JsonProperties::JsonFormat::Compact ) ? JsonFormattingOptions::Compact :
                                                                                                                                                                 JsonFormattingOptions::PrettySpacing);

    JsonRT::EngineSymbolSerializerHelper engine_symbol_serializer_helper(*this, *json_properties);
    const auto symbol_serializer_holder = json_writer->GetSerializerHelper().Register(&engine_symbol_serializer_helper);

    symbol.WriteJson(*json_writer, symbol_json_output);

    return json_writer->ReleaseString();
}


double LogicInterpreter::ex_Symbol_getJson_getValueJson(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    const Symbol* symbol;
    Symbol::SymbolJsonOutput symbol_json_output;

    if( symbol_va_with_subscript_node.function_code == FunctionCode::SYMBOLFN_GETJSON_CODE)
    {
        symbol = &GetFromSymbolOrEngineItemForStaticFunction(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

        symbol_json_output = IsDataAccessible(*symbol, false) ? Symbol::SymbolJsonOutput::MetadataAndValue :
                                                                Symbol::SymbolJsonOutput::Metadata;
    }

    else
    {
        ASSERT(symbol_va_with_subscript_node.function_code == FunctionCode::SYMBOLFN_GETVALUEJSON_CODE );

        symbol = GetFromSymbolOrEngineItem(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

        symbol_json_output = Symbol::SymbolJsonOutput::Value;
    }

    if( symbol != nullptr )
    {
        try
        {
            // process any serialization properties specified
            std::unique_ptr<const JsonNode> serialization_options_node;

            if( symbol_va_with_subscript_node.arguments[0] != -1 )
            {
                const SharableString json_text = EvaluateSharableString(symbol_va_with_subscript_node.arguments[0]);
                serialization_options_node = std::make_unique<JsonNode>(Json::Parse(*json_text, GetEngineJsonReaderInterface()));
            }

            return AssignString(GetSymbolJson(*symbol, symbol_json_output, serialization_options_node.get()));
        }

        catch( const CSProException& exception )
        {
            IssueMessage(MessageType::Error, MGF::JSON_Symbol_get_error_100441, symbol->GetName().c_str(),
                                                                                exception.what());
        }
    }

    return AssignStringNull();
}


void LogicInterpreter::SetSymbolValueFromJson(Symbol& symbol, const JsonNode& json_node)
{
    symbol.SetValueFromJson(json_node);
}


double LogicInterpreter::ex_Symbol_setValueFromJson(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    const SharableString json_text = EvaluateSharableString(symbol_va_with_subscript_node.arguments[0]);

    Symbol* const symbol = GetFromSymbolOrEngineItem(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( symbol != nullptr )
    {
        try
        {
            const JsonNode json_node = Json::Parse(*json_text, GetEngineJsonReaderInterface());

            SetSymbolValueFromJson(*symbol, json_node);

            return 1;
        }

        catch( const CSProException& exception )
        {
            IssueMessage(MessageType::Error, MGF::JSON_Symbol_set_error_100442, symbol->GetName().c_str(),
                                                                                exception.what());
        }
    }

    return 0;
}
