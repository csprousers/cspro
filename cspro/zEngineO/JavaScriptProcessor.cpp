#include "stdafx.h"
#include "JavaScriptProcessor.h"
#include "AllSymbols.h"
#include "EngineAccessor.h"
#include "EngineCaseConstructionReporter.h"
#include "UserFunctionArgumentChecker.h"
#include "UserFunctionArgumentEvaluator.h"
#include "Interpreter/LogicInterpreter.h"
#include <engine/InterpreterAccessor.h>
#include <zToolsO/ObjectTransporter.h>
#include <zMessageO/SystemMessageIssuer.h>
#include <zListingO/Lister.h>
#include <zLogicO/ActionInvoker.h>
#include <zJavaScript/Executor.h>


// --------------------------------------------------------------------------
// EngineJavaScriptProcessor
// --------------------------------------------------------------------------

EngineJavaScriptProcessor::EngineJavaScriptProcessor(EngineData& engine_data)
    :   m_engineData(engine_data),
        m_executorEvaluationState(ExecutorEvaluationState::None)
{
    ASSERT(m_engineData.engine_accessor != nullptr);

    const std::string& application_file_path = ( m_engineData.application != nullptr ) ? m_engineData.application->GetApplicationFilePath() :
                                                                                         SO::Empty_string;

    m_executor = std::make_unique<JavaScript::Executor>(PortableFunctions::PathGetDirectory(application_file_path));
}


EngineJavaScriptProcessor::~EngineJavaScriptProcessor()
{
}


size_t EngineJavaScriptProcessor::CompileScript(SharableString script, const JavaScript::ModuleType module_type, const std::string& file_path,
                                                const bool evaluate_at_application_startup)
{
    ASSERT(script.IsSet());

    JavaScript::Bytecode bytecode = m_executor->CompileScript(script.GetString(), module_type, file_path);

    m_bytecodeData.emplace_back(
        BytecodeData
        {
            file_path,
            std::move(script),
            module_type,
            evaluate_at_application_startup,
            std::move(bytecode)
        });

    return m_bytecodeData.size() - 1;
}


size_t EngineJavaScriptProcessor::CompileScript(SharableString script, const JavaScript::ModuleType module_type)
{
    return CompileScript(std::move(script), module_type, SO::Empty_string, false);
}


void EngineJavaScriptProcessor::CompileCodeFile(const CodeFile& code_file)
{
    ASSERT(code_file.IsJavaScript());

    const JavaScript::ModuleType module_type =
        ( code_file.GetCodeType() == CodeType::JavaScriptGlobal ) ? JavaScript::ModuleType::Global :
                                                                    JavaScript::ModuleType::Module;

    CompileScript(code_file.GetTextSource().GetTextAsSharableString(), module_type, code_file.GetTextSource().GetFilePath(), true);
}


std::string EngineJavaScriptProcessor::EvaluateBytecode(const BytecodeData& bytecode_data)
{
    ASSERT(m_executorEvaluationState != ExecutorEvaluationState::None);
    return m_executor->EvaluateBytecode(bytecode_data.bytecode);
}


std::string EngineJavaScriptProcessor::EvaluateBytecode(const size_t bytecode_index)
{
    if( bytecode_index >= m_bytecodeData.size() )
        throw ProgrammingErrorException();

    EnsureExecutorEvaluationState(ExecutorEvaluationState::FullRuntimeAccess);

    return EvaluateBytecode(m_bytecodeData[bytecode_index]);
}


void EngineJavaScriptProcessor::EvaluateApplicationStartupBytecode()
{
    EnsureExecutorEvaluationState(ExecutorEvaluationState::PrinterOnly);

    auto module_loading_errors = std::make_shared<std::vector<std::tuple<std::string, std::string>>>();
    m_executor->SetModuleLoaderErrorTracker(module_loading_errors);

    try
    {
        for( const BytecodeData& bytecode_data : m_bytecodeData )
        {
            if( bytecode_data.evaluate_at_application_startup )
                EvaluateBytecode(bytecode_data);
        }
    }

    catch( const CSProException& exception )
    {
        throw CSProException(SO::Concatenate("The application cannot be started due to JavaScript evaluation errors:\n\n", exception.what()));
    }

    if( ( !module_loading_errors->empty() ) &&
        ( m_engineData.application == nullptr ||
          m_engineData.application->GetApplicationProperties().GetJavaScriptProperties().GetAbortOnModuleLoadError() ) )
    {
        std::string message = "The application cannot be started due to JavaScript module loading errors:\n";

        for( const auto& [module_name, error] : *module_loading_errors )
            message.append(FormatText("\n'%s': %s", module_name.c_str(), error.c_str()));

        throw CSProException(message);
    }
}


std::string EngineJavaScriptProcessor::EvaluateScript(const std::string& script, bool& exception_is_from_compilation)
{
    ASSERT(exception_is_from_compilation);

    const JavaScript::Bytecode bytecode = m_executor->CompileScript(script, JavaScript::ModuleType::Global);

    exception_is_from_compilation = false;

    EnsureExecutorEvaluationState(ExecutorEvaluationState::FullRuntimeAccess);

    return m_executor->EvaluateBytecode(bytecode);
}


JavaScript::Value EngineJavaScriptProcessor::InvokeFunction(const std::string& function_name,
                                                            const size_t number_function_arguments, const JavaScript::Value* const js_function_arguments)
{
    EnsureExecutorEvaluationState(ExecutorEvaluationState::FullRuntimeAccess);

    return m_executor->InvokeFunction(function_name, number_function_arguments, js_function_arguments);
}


bool EngineJavaScriptProcessor::HasPropertyValue(const std::string& name) noexcept
{
    return m_executor->HasPropertyValue(name);
}


JavaScript::Value EngineJavaScriptProcessor::GetValue(const std::string& name)
{
    return m_executor->GetPropertyValue(name);
}


void EngineJavaScriptProcessor::SetValue(const std::string& name, JavaScript::Value js_value)
{
    ASSERT(!js_value.IsException());

    m_executor->SetPropertyValue(name, std::move(js_value));
}


std::string EngineJavaScriptProcessor::GetValueJson(const std::string& name)
{
    const JavaScript::Value js_value = GetValue(name);
    return m_executor->GetJsonForValue(js_value);
}


void EngineJavaScriptProcessor::SetValueFromJson(const std::string& name, const std::string& json_text, bool& exception_is_from_json_parsing)
{
    ASSERT(exception_is_from_json_parsing);

    JavaScript::Value js_value = m_executor->CreateValueFromJson(json_text);

    exception_is_from_json_parsing = false;

    SetValue(name, std::move(js_value));
}


JavaScript::Value EngineJavaScriptProcessor::CreateValue(const double value)
{
    return m_executor->CreateEngineValue(value);
}


double EngineJavaScriptProcessor::ConvertNumeric(const JavaScript::Value& js_value)
{
    return m_executor->ConvertEngineValue<double>(js_value);
}


JavaScript::Value EngineJavaScriptProcessor::CreateValue(const std::string_view value_sv)
{
    return m_executor->CreateEngineValue(value_sv);
}


JavaScript::Value EngineJavaScriptProcessor::CreateValue(const std::string& value)
{
    return m_executor->CreateEngineValue(value);
}


JavaScript::Value EngineJavaScriptProcessor::CreateValue(const SharableString& value)
{
    return m_executor->CreateEngineValue(value.GetString());
}


SharableString EngineJavaScriptProcessor::ConvertString(const JavaScript::Value& js_value)
{
    return m_executor->ConvertEngineValue<SharableString>(js_value);
}


JavaScript::Value EngineJavaScriptProcessor::CreateValue(const std::variant<double, SharableString>& value)
{
    return std::holds_alternative<double>(value) ? CreateValue(std::get<double>(value)) :
                                                   CreateValue(std::get<SharableString>(value));
}


JavaScript::Value EngineJavaScriptProcessor::CreateValue(const Engine::Value& value)
{
    if( value.is<double>() )
        return CreateValue(value.get<double>());

    ASSERT(value.is<SharableString>());
    return CreateValue(value.as<SharableString>());
}


JavaScript::Value EngineJavaScriptProcessor::CreateValue(const Symbol& symbol)
{
    switch( symbol.GetType() )
    {
        case SymbolType::UserFunction: return CreateValue(assert_cast<const UserFunction&>(symbol));
        default:                       return symbol.GetJavaScriptValue(*m_executor);
    }
}


void EngineJavaScriptProcessor::ConvertSymbol(const JavaScript::Value& js_value, Symbol& symbol)
{
    symbol.SetValueFromJavaScript(*m_executor, js_value);
}


JavaScript::Value EngineJavaScriptProcessor::CreateValue(const UserFunction& user_function)
{
#ifdef _DEBUG
    const UserFunctionArgumentChecker argument_checker(nullptr, user_function);
    ASSERT(!argument_checker.FindFirstInvalidParameter(EngineJavaScriptProcessor::SymbolTypesAllowedAsArguments, false).has_value());
#endif

    return m_executor->CreateFunction(
        [this, symbol_index = user_function.GetSymbolIndex(), interpreter_accessor = std::shared_ptr<InterpreterAccessor>()]
        (const int argc, const JavaScript::Value* const argv) mutable
        {
            if( interpreter_accessor == nullptr )
                interpreter_accessor = ObjectTransporter::GetInterpreterAccessor();

            return ExecuteLogicUserFunction(*interpreter_accessor, symbol_index, argc, argv);
        });
}


void EngineJavaScriptProcessor::serialize(Serializer& ar)
{
    static_assert(sizeof(JavaScript::Bytecode::value_type) == 1);

    // the bytecode generated on Windows cannot be read on Android because the size of JSValue differs,
    // so such bytecode will be recompiled on loading (if the script has been serialized)
    size_t serialized_JSValue_size = JavaScript::Value::GetSizeofJSValue();
    ar & serialized_JSValue_size;
    const bool serialized_bytecode_is_usable = ( serialized_JSValue_size == JavaScript::Value::GetSizeofJSValue() );

    JavaScriptProperties::BytecodeSerialization bytecode_serialization;

    if( ar.IsSaving() )
    {
        bytecode_serialization = ( m_engineData.application != nullptr ) ? m_engineData.application->GetApplicationProperties().GetJavaScriptProperties().GetBytecodeSerialization() :
                                                                           JavaScriptProperties::DefaultBytecodeSerialization;
    }

    ar.SerializeEnum(bytecode_serialization);

    vector_serialize(ar, m_bytecodeData,
        [&](BytecodeData& bytecode_data)
        {
            ar.SerializePath(bytecode_data.file_path);

            if( bytecode_serialization != JavaScriptProperties::BytecodeSerialization::BytecodeOnly )
                ar & bytecode_data.script;

            ar.SerializeEnum(bytecode_data.module_type);
            ar & bytecode_data.evaluate_at_application_startup;

            if( bytecode_serialization != JavaScriptProperties::BytecodeSerialization::ScriptOnly )
            {
                if( ar.IsSaving() )
                {
                    ar.Write<size_t>(bytecode_data.bytecode.size());
                    ar.Write(bytecode_data.bytecode.data(), bytecode_data.bytecode.size());
                }

                else
                {
                    const size_t bytecode_size = ar.Read<size_t>();

                    if( serialized_bytecode_is_usable )
                    {
                        bytecode_data.bytecode.resize(bytecode_size);
                        ar.Read(bytecode_data.bytecode.data(), bytecode_data.bytecode.size());
                    }

                    else
                    {
                        ar.IgnoreUnusedBytes(bytecode_size);
                    }
                }
            }
        });

    if( ar.IsLoading() && !m_bytecodeData.empty() )
        AddCompiledApplicationModuleLoaderHelper();
}



// --------------------------------------------------------------------------
// EngineJavaScriptProcessor::CompiledApplicationModuleLoaderHelper
// --------------------------------------------------------------------------

class EngineJavaScriptProcessor::CompiledApplicationModuleLoaderHelper : public JavaScript::ModuleLoaderHelper
{
public:
    CompiledApplicationModuleLoaderHelper(EngineJavaScriptProcessor& javascript_processor);

    void CompileAllBytecode();

protected:
    const JavaScript::Bytecode* GetBytecode(const std::string& file_path) override;

private:
    void CompileScript(BytecodeData& bytecode_data);

private:
    EngineJavaScriptProcessor& m_javascriptProcessor;
};


EngineJavaScriptProcessor::CompiledApplicationModuleLoaderHelper::CompiledApplicationModuleLoaderHelper(EngineJavaScriptProcessor& javascript_processor)
    :   m_javascriptProcessor(javascript_processor)
{
    ASSERT(!m_javascriptProcessor.m_bytecodeData.empty());
}


void EngineJavaScriptProcessor::CompiledApplicationModuleLoaderHelper::CompileAllBytecode()
{
#ifdef _DEBUG
    const size_t empty_bytecode_count = std::count_if(m_javascriptProcessor.m_bytecodeData.cbegin(), m_javascriptProcessor.m_bytecodeData.cend(),
                                                      [&](const BytecodeData& bytecode_data) { return bytecode_data.bytecode.empty(); });
    ASSERT(empty_bytecode_count == 0 ||
           empty_bytecode_count == m_javascriptProcessor.m_bytecodeData.size());
#endif

    if( !m_javascriptProcessor.m_bytecodeData.front().bytecode.empty() )
        return;

    for( BytecodeData& bytecode_data : m_javascriptProcessor.m_bytecodeData )
    {
        // GetBytecode may have already been called on a yet-to-be-processed bytecode_data,
        // so ensure that we actually need to compile the script
        if( bytecode_data.bytecode.empty() )
            CompileScript(bytecode_data);
    }
}


const JavaScript::Bytecode* EngineJavaScriptProcessor::CompiledApplicationModuleLoaderHelper::GetBytecode(const std::string& file_path)
{
    const auto& lookup = std::find_if(m_javascriptProcessor.m_bytecodeData.begin(), m_javascriptProcessor.m_bytecodeData.end(),
                                      [&](const BytecodeData& bytecode_data) { return SO::EqualsNoCase(file_path, bytecode_data.file_path); });

    if( lookup == m_javascriptProcessor.m_bytecodeData.cend() )
        return nullptr;

    if( lookup->bytecode.empty() )
    {
        CompileScript(*lookup);
        ASSERT(!lookup->bytecode.empty());
    }

    return &lookup->bytecode;
}


void EngineJavaScriptProcessor::CompiledApplicationModuleLoaderHelper::CompileScript(BytecodeData& bytecode_data)
{
    ASSERT(bytecode_data.bytecode.empty());

    if( !bytecode_data.script.IsSet() )
    {
        throw ApplicationLoadException("The compiled application could not be loaded because the "
                                       "JavaScript bytecode is not compatible with this operation system.");
    }

    try
    {
        bytecode_data.bytecode = m_javascriptProcessor.m_executor->CompileScript(bytecode_data.script.GetString(), bytecode_data.module_type,
                                                                                 bytecode_data.file_path);
    }

    catch( const CSProException& exception )
    {
        throw ApplicationLoadException("The compiled application could not be loaded due to an error recompiling JavaScript "
                                       "for this platform: %s", exception.what());
    }
}


void EngineJavaScriptProcessor::AddCompiledApplicationModuleLoaderHelper()
{
    const auto module_loader_helper = std::make_shared<CompiledApplicationModuleLoaderHelper>(*this);
    m_executor->SetModuleLoaderHelper(module_loader_helper);

    // ensure that all bytecode is compiled during the deserialization of the compiled application
    module_loader_helper->CompileAllBytecode();
}



// --------------------------------------------------------------------------
// EngineJavaScriptProcessor::Printer
// --------------------------------------------------------------------------

class EngineJavaScriptProcessor::Printer : public JavaScript::Printer
{
public:
    Printer(EngineAccessor& engine_accessor)
        :   m_engineAccessor(engine_accessor)
    {
    }

private:
    void OnPrint(const SharableString text) override
    {
        OnPrintOrConsoleLog(MGF::JavaScript_print_100461, text);
    }

    void OnConsoleLog(const SharableString text) override
    {
        OnPrintOrConsoleLog(MGF::JavaScript_console_log_100462, text);
    }

    void OnPrintOrConsoleLog(const int message_number, const SharableString& text) const
    {
        Listing::Lister* const lister = m_engineAccessor.ea_GetLister();

        if( lister != nullptr )
        {
            lister->Write(MessageType::Warning,
                          message_number,
                          m_engineAccessor.ea_GetSharedSystemMessageIssuer()->GetFormattedMessage(message_number, text.GetString().c_str()));
        }
    }

private:
    EngineAccessor& m_engineAccessor;
};



// --------------------------------------------------------------------------
// EngineJavaScriptProcessor::EnsureExecutorEvaluationState
// --------------------------------------------------------------------------

void EngineJavaScriptProcessor::EnsureExecutorEvaluationState(const ExecutorEvaluationState state)
{
    ASSERT(state != ExecutorEvaluationState::None);

    if( state <= m_executorEvaluationState )
        return;

    ASSERT(state >= ExecutorEvaluationState::PrinterOnly);

    if( m_executorEvaluationState < ExecutorEvaluationState::PrinterOnly )
        m_executor->SetPrinter(std::make_unique<Printer>(*m_engineData.engine_accessor));

    if( state == ExecutorEvaluationState::FullRuntimeAccess )
    {
        if( m_engineData.application != nullptr )
        {
            const JavaScriptProperties& javascript_properties = m_engineData.application->GetApplicationProperties().GetJavaScriptProperties();

            if( javascript_properties.GetUseActionInvoker() )
            {
                m_executor->UseActionInvoker(
                    ActionInvoker::GetFunctions(),
                    ActionInvoker::GetNamespaceNames(),
                    std::make_unique<EngineCaseConstructionReporter>(
                        m_engineData.engine_accessor->ea_GetSharedSystemMessageIssuer(),
                        nullptr
                    ),
                    javascript_properties.GetActionInvokerObjectNameOverride()
               );
            }
        }
    }

    m_executorEvaluationState = state;
}



// --------------------------------------------------------------------------
// EngineJavaScriptProcessor functionality to call logic functions
// --------------------------------------------------------------------------

class EngineJavaScriptProcessor::ArgumentEvaluator : public UserFunctionArgumentEvaluator
{
public:
    ArgumentEvaluator(EngineJavaScriptProcessor& javascript_processor, const UserFunction& user_function,
                      size_t number_function_arguments, const JavaScript::Value* js_function_arguments);

protected:
    std::optional<size_t> GetNumberArguments() override { return m_numberFunctionArguments; }

    double GetNumeric(size_t parameter_number) override;
    SharableString GetString(size_t parameter_number) override;
    bool ConstructSymbolInPlace(size_t parameter_number, Symbol& parameter_symbol) override;
    std::shared_ptr<Symbol> GetSymbol(size_t parameter_number) override;

private:
    template<typename CF>
    auto ConvertValueWorker(size_t parameter_number, const CF& callback_function);

private:
    EngineJavaScriptProcessor& m_javascriptProcessor;
    const UserFunction& m_userFunction;
    size_t m_numberFunctionArguments;
    const JavaScript::Value* m_jsFunctionArguments;
};


EngineJavaScriptProcessor::ArgumentEvaluator::ArgumentEvaluator(EngineJavaScriptProcessor& javascript_processor, const UserFunction& user_function,
                                                                const size_t number_function_arguments, const JavaScript::Value* const js_function_arguments)
    :   m_javascriptProcessor(javascript_processor),
        m_userFunction(user_function),
        m_numberFunctionArguments(number_function_arguments),
        m_jsFunctionArguments(js_function_arguments)
{
    ASSERT(m_numberFunctionArguments >= m_userFunction.GetNumberRequiredParameters());
    ASSERT(m_jsFunctionArguments != nullptr || m_numberFunctionArguments == 0);
}


template<typename CF>
auto EngineJavaScriptProcessor::ArgumentEvaluator::ConvertValueWorker(const size_t parameter_number, const CF& callback_function)
{
    ASSERT(parameter_number < m_numberFunctionArguments);
    const JavaScript::Value& js_value = m_jsFunctionArguments[parameter_number];

    try
    {
        return callback_function(js_value);
    }

    catch( const CSProException& exception )
    {
        const std::shared_ptr<SystemMessageIssuer> system_message_issuer =
            m_javascriptProcessor.m_engineData.engine_accessor->ea_GetSharedSystemMessageIssuer();

        throw CSProException(system_message_issuer->GetFormattedMessage(
            MGF::JavaScript_value_conversion_error_100467,
            ToDisplayString(m_userFunction.GetParameterSymbol(parameter_number).GetType()),
            exception.what()
        ));
    }
}


double EngineJavaScriptProcessor::ArgumentEvaluator::GetNumeric(const size_t parameter_number)
{
    return ConvertValueWorker(parameter_number,
        [&](const JavaScript::Value& js_value)
        {
            return m_javascriptProcessor.ConvertNumeric(js_value);
        });
}


SharableString EngineJavaScriptProcessor::ArgumentEvaluator::GetString(const size_t parameter_number)
{
    return ConvertValueWorker(parameter_number,
        [&](const JavaScript::Value& js_value)
        {
            return m_javascriptProcessor.ConvertString(js_value);
        });
}


bool EngineJavaScriptProcessor::ArgumentEvaluator::ConstructSymbolInPlace(const size_t parameter_number, Symbol& parameter_symbol)
{
    // because Array parameters do not have a fixed size with allocated memory, we cannot construct
    // an Array in place and instead must create a new one (in GetSymbol)
    if( parameter_symbol.IsA(SymbolType::Array) )
        return false;

    ConvertValueWorker(parameter_number,
        [&](const JavaScript::Value& js_value)
        {
            m_javascriptProcessor.ConvertSymbol(js_value, parameter_symbol);
        });

    return true;
}


std::shared_ptr<Symbol> EngineJavaScriptProcessor::ArgumentEvaluator::GetSymbol(const size_t parameter_number)
{
    // for parameter symbols that cannot be used (in ConstructSymbolInPlace), we construct new symbols here
    const Symbol& base_parameter_symbol = m_userFunction.GetParameterSymbol(parameter_number);
    std::unique_ptr<Symbol> argument_symbol = base_parameter_symbol.CloneInInitialState();

    // Array
    {
        ASSERT(argument_symbol->IsA(SymbolType::Array));
        // the dimension size calculations of will be handled in LogicArray::SetValueFromJavaScriptWorker
    }

    ConvertValueWorker(parameter_number,
        [&](const JavaScript::Value& js_value)
        {
            m_javascriptProcessor.ConvertSymbol(js_value, *argument_symbol);
        });

    return argument_symbol;
}


JavaScript::Value EngineJavaScriptProcessor::ExecuteLogicUserFunction(InterpreterAccessor& interpreter_accessor, const int user_function_symbol_index,
                                                                      const size_t argc, const JavaScript::Value* const js_argv)
{
    UserFunction& user_function = assert_cast<UserFunction&>(m_engineData.symbol_table.GetAt(user_function_symbol_index));

    try
    {
        if( interpreter_accessor.GetInterpreter().IsExecutionInterrupted() )
            throw CSProException("CSPro logic cannot be executed due to a pending program control statement");

        // check the number of arguments;
        // too many arguments will be allowed, with the additional arguments ignored
        if( argc < user_function.GetNumberParameters() )
        {
            try
            {
                const UserFunctionArgumentChecker argument_checker(nullptr, user_function);
                argument_checker.CheckNumberArguments(argc);
            }

            catch( const UserFunctionArgumentChecker::CheckError& error )
            {
                throw CSProException("the user-defined function expects %s", error.what());
            }
        }

        // execute the function
        ArgumentEvaluator argument_evaluator(*this, user_function, argc, js_argv);
        const InterpreterExecuteResult execute_result = interpreter_accessor.CallUserFunction(user_function, argument_evaluator);

        if( execute_result.program_control_executed )
            throw CSProException("the function terminated prior to completion due to a program control statement");

        return CreateValue(execute_result.result);
    }

    catch( const CSProException& exception )
    {
        throw CSProException("Error executing '%s': %s", user_function.GetName().c_str(), exception.what());
    }
}
