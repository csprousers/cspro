#include "stdafx.h"
#include "IncludesCC.h"
#include "LogicCompiler.h"
#include "EnginePreprocessor.h"
#include <zToolsO/ConstantConserver.h>


LogicCompiler::LogicCompiler(cs::non_null_shared_or_raw_ptr<EngineData> engine_data)
    :   Logic::BaseCompiler(engine_data->symbol_table),
        m_engineData(std::move(engine_data)),
        m_preprocessor(std::make_unique<EnginePreprocessor>(*this, *m_engineData)),
        m_compilationSymbol(nullptr),
        m_procType(ProcType::None),
        m_extendedProcType(ExtendedProcType::None),
        m_numericConstantConserver(std::make_unique<ConstantConserver<double>>(m_engineData->numeric_constants)),
        m_stringLiteralConserver(std::make_unique<ConstantConserver<SharableString>>(m_engineData->string_literals)),
        m_tracingLogic(false)
{
}


LogicCompiler::~LogicCompiler()
{
}


std::string LogicCompiler::GetCurrentProcName() const
{
    return ( m_compilationSymbol != nullptr ) ? m_compilationSymbol->GetName() :
                                                std::string();
}


int LogicCompiler::ConserveConstant(const double numeric_constant)
{
    return m_numericConstantConserver->Add(numeric_constant);
}


int LogicCompiler::ConserveConstant(const std::string& string_literal)
{
    return m_stringLiteralConserver->Add(string_literal);
}


int LogicCompiler::ConserveConstant(std::string&& string_literal)
{
    return m_stringLiteralConserver->Add(std::move(string_literal));
}


int LogicCompiler::ConserveConstant(SharableString&& string_literal)
{
    return m_stringLiteralConserver->Add(std::move(string_literal));
}
