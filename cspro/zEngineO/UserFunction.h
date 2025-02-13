#pragma once

#include <zEngineO/zEngineO.h>
#include <zEngineO/UserFunctionLocalSymbolsManager.h>

struct EngineData;


// --------------------------------------------------------------------------
// UserFunction
// --------------------------------------------------------------------------

class ZENGINEO_API UserFunction : public Symbol
{
    friend UserFunctionLocalSymbolsManager;

public:
    UserFunction(std::wstring user_function_name, EngineData& engine_data);

    void SetProgramIndex(int program_index) { m_programIndex = program_index; }
    int GetProgramIndex() const             { return m_programIndex; }

    void SetReturnType(SymbolType return_type) { m_returnType = return_type; }
    SymbolType GetReturnType() const           { return m_returnType; }
    DataType GetReturnDataType() const         { return ( m_returnType == SymbolType::WorkVariable ) ? DataType::Numeric : DataType::String; }

    void SetReturnPaddingStringLength(int return_string_length) { m_returnPaddingStringLength = return_string_length; }
    int GetReturnPaddingStringLength() const                    { return m_returnPaddingStringLength; }

    void SetSqlCallbackFunction(bool bIsSqlCallbackFunction) { m_sqlCallbackFunction = bIsSqlCallbackFunction; }
    bool IsSqlCallbackFunction() const                       { return m_sqlCallbackFunction; }

    void AddParameterSymbol(const Symbol& parameter_symbol);
    int GetParameterSymbolIndex(size_t parameter_number) const { return m_parameterSymbols[parameter_number]; }
    const std::vector<int>& GetParameterSymbolIndices() const  { return m_parameterSymbols; }
    size_t GetNumberParameters() const                         { return m_parameterSymbols.size(); }

    Symbol& GetParameterSymbol(size_t parameter_number);
    const Symbol& GetParameterSymbol(size_t parameter_number) const { return const_cast<UserFunction*>(this)->GetParameterSymbol(parameter_number); }

    void AddParameterDefaultValue(int default_value_expression) { m_parameterDefaultValues.emplace_back(default_value_expression); }
    int GetParameterDefaultValue(size_t parameter_number) const { return m_parameterDefaultValues[parameter_number - GetNumberRequiredParameters()]; }
    size_t GetNumberRequiredParameters() const                  { return m_parameterSymbols.size() - m_parameterDefaultValues.size(); };

    void SetFunctionBodySymbols(std::vector<int> function_body_symbols) { m_functionBodySymbols = std::move(function_body_symbols); }

    // runtime methods
    UserFunctionLocalSymbolsManager GetLocalSymbolsManager() { return UserFunctionLocalSymbolsManager(*this); }

    void SetReturnValue(std::variant<double, std::wstring> return_value);
    template<typename T> const T& GetReturnValue() const { return std::get<T>(m_returnValue); }    

    // Symbol overrides
    void Reset() override;

    void serialize_subclass(Serializer& ar) override;

protected:
    void WriteJsonMetadata_subclass(JsonWriter& json_writer) const override;

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
    std::variant<double, std::wstring> m_returnValue;

    std::vector<std::shared_ptr<UserFunctionLocalSymbolsManager::Data>> m_localSymbolsManagerData;
    size_t m_functionCallCount;
};



// --------------------------------------------------------------------------
// UserFunctionArgumentEvaluator
//
// for evaluating the arguments to a function
// --------------------------------------------------------------------------

class UserFunctionArgumentEvaluator
{
public:
    virtual ~UserFunctionArgumentEvaluator() { }

    // return the evaluated numeric expression
    virtual double GetNumeric(int parameter_number) = 0;

    // return the evaluated string expression
    virtual std::wstring GetString(int parameter_number) = 0;

    // return true if the symbol's argument was constructed in place (using the parameter symbol)
    virtual bool ConstructSymbolInPlace(int /*parameter_number*/, Symbol& /*parameter_symbol*/) { return false; }

    // return the evaluated symbol if the symbol was not constructed in place, or null to use the parameter symbol;
    // if the symbol is not valid (e.g., has a invalid subscript), throw an InvalidSubscript exception
    virtual std::shared_ptr<Symbol> GetSymbol(int /*parameter_number*/) { return ReturnProgrammingError(nullptr); }

    CREATE_CSPRO_EXCEPTION_WITH_MESSAGE(InvalidSubscript, "")
};
