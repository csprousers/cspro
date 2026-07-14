#include "stdafx.h"
#include "IncludesCC.h"
#include "File.h"


void LogicCompiler::SetCompilationSymbol(const Symbol* const symbol) noexcept
{
    m_compilationSymbol = symbol;
}


SymbolType LogicCompiler::GetCompilationSymbolType() const noexcept
{
    if( m_compilationSymbol != nullptr )
        return m_compilationSymbol->GetType();

    return SymbolType::None;
}


int LogicCompiler::GetCompilationLevelNumber_base1() const
{
    if( m_compilationSymbol != nullptr )
        return SymbolCalculator::GetLevelNumber_base1(*m_compilationSymbol);

    return SymbolCalculator::NoLevelNumber;
}


bool LogicCompiler::IsCompiling(const Symbol& symbol) const noexcept
{
    return ( m_compilationSymbol == &symbol );
}


bool LogicCompiler::IsCompiling(const SymbolType symbol_type) const noexcept
{
    return ( GetCompilationSymbolType() == symbol_type );
}


bool LogicCompiler::IsGlobalCompilation() const noexcept
{
    return IsCompiling(SymbolType::Application);
}


bool LogicCompiler::IsNoLevelCompilation() const noexcept
{
    if( m_compilationSymbol == nullptr )
        return true;

    return m_compilationSymbol->IsOneOf(
        SymbolType::Application,
        SymbolType::Report,
        SymbolType::UserFunction
    );
}


EngineAppType LogicCompiler::GetEngineAppType() const
{
    return ( m_engineData->application != nullptr ) ? m_engineData->application->GetEngineAppType() :
                                                      EngineAppType::Invalid;
}


void LogicCompiler::SetCompilationProcType(const ProcType proc_type, const ExtendedProcType extended_proc_type/* = ExtendedProcType::None*/)
{
    m_procType = proc_type;
    m_extendedProcType = extended_proc_type;
}


void LogicCompiler::CompileExternalCode()
{
    ASSERT(m_engineData->application != nullptr);

    for( const CodeFile& code_file : m_engineData->application->GetCodeFiles() )
    {
        if( code_file.GetCodeType() != CodeType::LogicMain )
            CompileExternalCode(code_file);
    }
}


void LogicCompiler::CompileExternalCode(const CodeFile& code_file)
{
    if( code_file.GetCodeType() == CodeType::LogicExternal )
    {
        CompileExternalCodeLogic(code_file);
    }

    else
    {
        ASSERT(code_file.IsJavaScript());
        CompileExternalCodeJavaScript(code_file);
    }
}


void LogicCompiler::RunPostCompilationChecks()
{
    // report errors for any function declarations that were never defined;
    // TODO: ReportError is used instead of IssueError because this is called in places where the
    //       IssueError infrastructure is no longer in place, but this should eventually be changed
    for( const int symbol_index : m_declaredSymbolIndices )
    {
        const Symbol& symbol = NPT_Ref(symbol_index);
        ASSERT(symbol.IsA(SymbolType::UserFunction));

        ReportError(MGF::UserFunction_declared_but_never_defined_50009, symbol.GetName().c_str());
    }

    // issue warnings for File handlers that are not used
    for( const LogicFile* const logic_file : m_engineData->files_global_visibility )
    {
        if( !logic_file->IsUsed() )
            IssueWarning(MGF::File_handler_not_used_505, logic_file->GetName().c_str());
    }
}
