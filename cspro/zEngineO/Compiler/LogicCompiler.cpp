#include "stdafx.h"
#include "IncludesCC.h"
#include "LogicCompiler.h"
#include <zToolsO/ConstantConserver.h>


LogicCompiler::LogicCompiler(std::shared_ptr<EngineData> engine_data)
    :   Logic::BaseCompiler(engine_data->symbol_table),
        m_engineData(std::move(engine_data)),
        m_compilationSymbol(nullptr),
        m_numericConstantConserver(std::make_unique<ConstantConserver<double>>(m_engineData->numeric_constants)),
        m_stringLiteralConserver(std::make_unique<ConstantConserver<std::wstring>>(m_engineData->string_literals)),
        m_tracingLogic(false)
{
}


LogicCompiler::~LogicCompiler()
{
}


void LogicCompiler::SetCompilationSymbol(const Symbol& symbol)
{
    m_compilationSymbol = &symbol;
}


int LogicCompiler::GetCompilationLevelNumber_base1() const
{
    return SymbolCalculator::GetLevelNumber_base1(*m_compilationSymbol);
}


bool LogicCompiler::IsNoLevelCompilation() const
{
    return m_compilationSymbol->IsOneOf(SymbolType::Application,
                                        SymbolType::Report,
                                        SymbolType::UserFunction);
}


const std::wstring& LogicCompiler::GetCurrentProcName() const
{
    return ( m_compilationSymbol != nullptr ) ? m_compilationSymbol->GetName() : 
                                                SO::EmptyString;
}


EngineAppType LogicCompiler::GetEngineAppType() const
{
    return ( m_engineData->application != nullptr ) ? m_engineData->application->GetEngineAppType() :
                                                      EngineAppType::Invalid;
}


int LogicCompiler::ConserveConstant(double numeric_constant)
{
    return m_numericConstantConserver->Add(numeric_constant);
}


int LogicCompiler::ConserveConstant(const std::wstring& string_literal)
{
    return m_stringLiteralConserver->Add(string_literal);
}


int LogicCompiler::ConserveConstant(std::wstring&& string_literal)
{
    return m_stringLiteralConserver->Add(std::move(string_literal));
}
