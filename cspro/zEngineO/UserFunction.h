#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/EngineValue.h>
#include <zEngineO/UserFunctionLocalSymbolsManager.h>

struct EngineData;
class UserFunctionArgumentEvaluator;


// --------------------------------------------------------------------------
// UserFunction
// --------------------------------------------------------------------------

class ZENGINEO_API UserFunction : public Symbol
{
    friend UserFunctionLocalSymbolsManager;

public:
    UserFunction(std::string user_function_name, EngineData& engine_data);

    void SetProgramIndex(int program_index) { m_programIndex = program_index; }
    int GetProgramIndex() const             { return m_programIndex; }

    void SetReturnType(SymbolType return_type) { m_returnType = return_type; }
    SymbolType GetReturnType() const           { return m_returnType; }
    DataType GetReturnDataType() const         { return ( m_returnType == SymbolType::WorkVariable ) ? DataType::Numeric : DataType::String; }

    void SetReturnPaddingStringLength(int return_string_length) { m_returnPaddingStringLength = return_string_length; }
    int GetReturnPaddingStringLength() const                    { return m_returnPaddingStringLength; }

    void SetSqlCallbackFunction(bool bIsSqlCallbackFunction) { m_sqlCallbackFunction = bIsSqlCallbackFunction; }
    bool IsSqlCallbackFunction() const                       { return m_sqlCallbackFunction; }

    void SetParameters(std::vector<int> parameter_symbol_indices, std::vector<int> parameter_default_values);

    int GetParameterSymbolIndex(size_t parameter_number) const { return m_parameterSymbols[parameter_number]; }
    const std::vector<int>& GetParameterSymbolIndices() const  { return m_parameterSymbols; }
    size_t GetNumberParameters() const                         { return m_parameterSymbols.size(); }

    Symbol& GetParameterSymbol(size_t parameter_number);
    const Symbol& GetParameterSymbol(size_t parameter_number) const { return const_cast<UserFunction*>(this)->GetParameterSymbol(parameter_number); }

    std::vector<SymbolType> GetParameterSymbolTypes() const;

    int GetParameterDefaultValue(size_t parameter_number) const { return m_parameterDefaultValues[parameter_number - GetNumberRequiredParameters()]; }
    size_t GetNumberRequiredParameters() const                  { return m_parameterSymbols.size() - m_parameterDefaultValues.size(); };

    void SetFunctionBodySymbols(std::vector<int> function_body_symbols) { m_functionBodySymbols = std::move(function_body_symbols); }

    // runtime methods
    UserFunctionLocalSymbolsManager GetLocalSymbolsManager() { return UserFunctionLocalSymbolsManager(*this); }

    void SetReturnValue(Engine::Value return_value);
    Engine::Value GetReturnValue() const;

    // Symbol overrides
    void CompareDeclarationAttributes(const Symbol& symbol) const override;
    void CompareDeclarationAttributes(SymbolType return_type, int return_padding_string_length, bool sql_callback_function) const;

    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

private:
    Engine::Value PreprocessReturnValue(Engine::Value return_value) const;

private:
    EngineData& m_engineData;
    int m_programIndex;

    SymbolType m_returnType;
    int m_returnPaddingStringLength;

    bool m_sqlCallbackFunction;

    std::vector<int> m_parameterSymbols;
    std::vector<int> m_parameterDefaultValues;

    std::vector<int> m_functionBodySymbols;

    // runtime only
    std::optional<Engine::Value> m_returnValue;

    std::vector<std::shared_ptr<UserFunctionLocalSymbolsManager::Data>> m_localSymbolsManagerData;
    size_t m_functionCallCount;
};
