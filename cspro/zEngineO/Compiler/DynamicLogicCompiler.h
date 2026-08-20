#pragma once

#include <zEngineO/Compiler/LogicCompiler.h>


// --------------------------------------------------------------------------
// DynamicLogicCompiler_COMPILER_DLL_TODO
//
// This is a temporary class for dynamic logic compilation that throws
// exceptions for all virtual methods that are not implemented.
// --------------------------------------------------------------------------

class DynamicLogicCompiler_COMPILER_DLL_TODO : public LogicCompiler
{
public:
    using LogicCompiler::LogicCompiler;

protected:
    const LogicSettings& GetLogicSettings() const override
    {
        if( m_engineData->application == nullptr )
            throw CreateCompilationException();

        return m_engineData->application->GetLogicSettings();
    }

    void ProcessSymbol() override
    {
        // COMPILER_DLL_TODO LogicCompiler::ProcessSymbol's implementation sets Tokstindex,
        // which is currently only in CEngineCompFunc, so only use the base version
        BaseCompiler::ProcessSymbol();
    }

    virtual CSProException CreateCompilationException(const char* const missing_method = nullptr) const
    {
        return CSProException("There was an error compiling the logic. CSPro currently supports the dynamic compilation "
                             "of only some logic statements and functions. Missing method: %s",
                             ( missing_method != nullptr ) ? missing_method : ReturnProgrammingError("<unknown>"));
    }

private:
#pragma warning(push)
#pragma warning(disable:4100)

#define NOT_IMPLEMENTED_COMPILER_DLL_TODO(func) override final { throw CreateCompilationException(#func); }

    void CompileExternalCodeLogic(const CodeFile& code_file) NOT_IMPLEMENTED_COMPILER_DLL_TODO(CompileExternalCodeLogic)
    void CompileExternalCodeJavaScript(const CodeFile& code_file) NOT_IMPLEMENTED_COMPILER_DLL_TODO(CompileExternalCodeJavaScript)
    MessageManager& GetUserMessageManager() NOT_IMPLEMENTED_COMPILER_DLL_TODO(GetUserMessageManager)
    MessageEvaluator& GetUserMessageEvaluator() NOT_IMPLEMENTED_COMPILER_DLL_TODO(GetUserMessageEvaluator)
    int& get_COMPILER_DLL_TODO_Tokstindex() NOT_IMPLEMENTED_COMPILER_DLL_TODO(get_COMPILER_DLL_TODO_Tokstindex)
    int& get_COMPILER_DLL_TODO_InCompIdx() NOT_IMPLEMENTED_COMPILER_DLL_TODO(get_COMPILER_DLL_TODO_InCompIdx)
    std::tuple<int, bool>& get_COMPILER_DLL_TODO_m_loneAlphaFunctionCallTester() NOT_IMPLEMENTED_COMPILER_DLL_TODO(get_COMPILER_DLL_TODO_m_loneAlphaFunctionCallTester)
    int CompileHas_COMPILER_DLL_TODO(int iVarNode) NOT_IMPLEMENTED_COMPILER_DLL_TODO(CompileHas_COMPILER_DLL_TODO)
    int crelalpha_COMPILER_DLL_TODO() NOT_IMPLEMENTED_COMPILER_DLL_TODO(crelalpha_COMPILER_DLL_TODO)
    int varsanal_COMPILER_DLL_TODO(int fmt) NOT_IMPLEMENTED_COMPILER_DLL_TODO(varsanal_COMPILER_DLL_TODO)
    int tvarsanal_COMPILER_DLL_TODO() NOT_IMPLEMENTED_COMPILER_DLL_TODO(tvarsanal_COMPILER_DLL_TODO)
    int rutfunc_COMPILER_DLL_TODO(Logic::FunctionCompilationType compilation_type) NOT_IMPLEMENTED_COMPILER_DLL_TODO(rutfunc_COMPILER_DLL_TODO)
    int instruc_COMPILER_DLL_TODO(bool allow_multiple_statements = true) NOT_IMPLEMENTED_COMPILER_DLL_TODO(instruc_COMPILER_DLL_TODO)
    DICT* GetInputDictionary(bool issue_error_if_no_input_dictionary) NOT_IMPLEMENTED_COMPILER_DLL_TODO(GetInputDictionary)
    void MarkAllDictionaryItemsAsUsed() NOT_IMPLEMENTED_COMPILER_DLL_TODO(MarkAllDictionaryItemsAsUsed)
    void MarkAllInSectionUsed(SECT* pSecT) NOT_IMPLEMENTED_COMPILER_DLL_TODO(MarkAllInSectionUsed)
    void SetCaseAccessSetRequiresFullAccess_COMPILER_DLL_TODO(Symbol& symbol) NOT_IMPLEMENTED_COMPILER_DLL_TODO(SetCaseAccessSetRequiresFullAccess_COMPILER_DLL_TODO)
    int CompileReenterStatement_COMPILER_DLL_TODO(bool bNextTkn = true) NOT_IMPLEMENTED_COMPILER_DLL_TODO(CompileReenterStatement_COMPILER_DLL_TODO)
    int CompileMoveStatement_COMPILER_DLL_TODO(bool bFromSelectStatement = false) NOT_IMPLEMENTED_COMPILER_DLL_TODO(CompileMoveStatement_COMPILER_DLL_TODO)
    void rutasync_as_global_compilation_COMPILER_DLL_TODO(const Symbol& compilation_symbol, const std::function<void()>& compilation_function) NOT_IMPLEMENTED_COMPILER_DLL_TODO(rutasync_as_global_compilation_COMPILER_DLL_TODO)
    void CheckIdChanger(const VART* pVarT) NOT_IMPLEMENTED_COMPILER_DLL_TODO(CheckIdChanger)
    std::vector<const DictNamedBase*> GetImplicitSubscriptCalculationStack(const EngineItem& engine_item) const NOT_IMPLEMENTED_COMPILER_DLL_TODO(GetImplicitSubscriptCalculationStack)

#undef NOT_IMPLEMENTED_COMPILER_DLL_TODO

#pragma warning(pop)
};
