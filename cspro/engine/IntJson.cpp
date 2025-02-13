#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/SymbolSerializerHelper.h>
#include <zJson/Json.h>
#include <zAppO/Properties/ApplicationProperties.h>
#include <ZBRIDGEO/npff.h>


namespace
{
    class EngineJsonReaderInterface : public JsonReaderInterface
    {
    public:
        EngineJsonReaderInterface(CIntDriver* pIntDriver)
            :   m_pEngineDriver(pIntDriver->m_pEngineDriver)
        {
            m_directory = GetWorkingFolder(m_pEngineDriver->m_pPifFile->GetAppFName());
        }

    protected:
        void OnLogWarning(std::wstring message) override
        {
            issaerror(MessageType::Warning, message);
        }

    private:
        CEngineDriver* m_pEngineDriver;
    };
}


JsonReaderInterface* CIntDriver::GetEngineJsonReaderInterface()
{
    if( m_engineJsonReaderInterface == nullptr )
        m_engineJsonReaderInterface = std::make_unique<EngineJsonReaderInterface>(this);

    return m_engineJsonReaderInterface.get();
}


namespace
{
    class EngineSymbolSerializerHelper : public SymbolSerializerHelper
    {
    public:
        EngineSymbolSerializerHelper(CIntDriver& interpreter, const JsonProperties& json_properties)
            :   SymbolSerializerHelper(json_properties),
                m_interpreter(interpreter)
        {
        }

        std::wstring LocalhostCreateMappingForBinarySymbol(const BinarySymbol& binary_symbol) override
        {
            return m_interpreter.LocalhostCreateMappingForBinarySymbol(binary_symbol);
        }

    private:
        CIntDriver& m_interpreter;
    };
}


std::wstring CIntDriver::GetSymbolJson(const Symbol& symbol, const Symbol::SymbolJsonOutput symbol_json_output, const JsonNode<wchar_t>* serialization_options_node)
{
    cs::shared_or_raw_ptr<JsonProperties> json_properties;

    // parse any serialization properties specified (on top of the application's default properties)...
    if( serialization_options_node != nullptr )
    {
        json_properties = std::make_shared<JsonProperties>(m_pEngineDriver->m_pPifFile->GetApplication()->GetApplicationProperties().GetJsonProperties());
        json_properties->UpdateFromJson(*serialization_options_node);
    }

    // ...or use the application's properties
    else
    {
        json_properties = &m_pEngineDriver->m_pPifFile->GetApplication()->GetApplicationProperties().GetJsonProperties();
    }

    auto json_writer = Json::CreateStringWriter(( json_properties->GetJsonFormat() == JsonProperties::JsonFormat::Compact ) ? JsonFormattingOptions::Compact :
                                                                                                                              JsonFormattingOptions::PrettySpacing);

    EngineSymbolSerializerHelper engine_symbol_serializer_helper(*this, *json_properties);
    auto symbol_serializer_holder = json_writer->GetSerializerHelper().Register(&engine_symbol_serializer_helper);

    symbol.WriteJson(*json_writer, symbol_json_output);
        
    return json_writer->GetString();
}


double CIntDriver::exSymbol_getJson_getValueJson(const int program_index)
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
            std::unique_ptr<const JsonNode<wchar_t>> serialization_options_node;

            if( symbol_va_with_subscript_node.arguments[0] != -1 )
            {
                const std::wstring json_text = EvalAlphaExpr(symbol_va_with_subscript_node.arguments[0]);
                serialization_options_node = std::make_unique<JsonNode<wchar_t>>(Json::Parse(json_text, GetEngineJsonReaderInterface()));
            }

            return AssignAlphaValue(GetSymbolJson(*symbol, symbol_json_output, serialization_options_node.get()));
        }

        catch( const CSProException& exception )
        {
            issaerror(MessageType::Error, 100441, symbol->GetName().c_str(),
                                                  exception.GetErrorMessage().c_str());
        }
    }

    return AssignBlankAlphaValue();
}


void CIntDriver::UpdateSymbolValueFromJson(Symbol& symbol, const JsonNode<wchar_t>& json_node)
{
    symbol.UpdateValueFromJson(json_node);
}


double CIntDriver::exSymbol_updateValueFromJson(const int program_index)
{
    const auto& symbol_va_with_subscript_node = GetNode<Nodes::SymbolVariableArgumentsWithSubscript>(program_index);
    const std::wstring json_text = EvalAlphaExpr(symbol_va_with_subscript_node.arguments[0]);

    Symbol* symbol = GetFromSymbolOrEngineItem(symbol_va_with_subscript_node.symbol_index, symbol_va_with_subscript_node.subscript_compilation);

    if( symbol != nullptr )
    {
        try
        {
            const JsonNode<wchar_t> json_node = Json::Parse(json_text, GetEngineJsonReaderInterface());

            UpdateSymbolValueFromJson(*symbol, json_node);

            return 1;
        }

        catch( const CSProException& exception )
        {
            issaerror(MessageType::Error, 100442, symbol->GetName().c_str(),
                                                  exception.GetErrorMessage().c_str());
        }
    }

    return 0;
}
