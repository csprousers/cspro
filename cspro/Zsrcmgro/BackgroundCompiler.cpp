#include "StdAfx.h"
#include "BackgroundCompiler.h"
#include "DesignerApplicationLoader.h"
#include <zAppO/Application.h>
#include <engine/Comp.h>
#include <Wcompile/Wcompile.h>


BackgroundCompiler::BackgroundCompiler(Application& application, DesignerCompilerMessageProcessor* designer_compiler_message_processor/* = nullptr*/,
                                       CompilerCreator* compiler_creator/* = nullptr*/)
    :   m_compIFaz(std::make_unique<CCompIFaz>(&application, compiler_creator))
{
    application.SetApplicationLoader(std::make_shared<DesignerApplicationLoader>(&application, designer_compiler_message_processor));
    
    bool compiler_initialization_errors = false;
    m_compIFaz->C_CompilerInit(nullptr, compiler_initialization_errors);
}


BackgroundCompiler::~BackgroundCompiler()
{
    m_compIFaz->C_CompilerEnd();
}


const Logic::SymbolTable& BackgroundCompiler::GetSymbolTable() const
{
    return m_compIFaz->GetSymbolTable();
}


void BackgroundCompiler::Compile(const TCHAR* buffer_text)
{
    m_compIFaz->C_CompilerCompile(buffer_text);
}


void BackgroundCompiler::Compile(std::shared_ptr<Logic::SourceBuffer> source_buffer)
{
    m_compIFaz->C_CompilerCompile(source_buffer);
}


const Logic::SymbolTable& BackgroundCompiler::GetCompiledSymbolTable() const
{
    return m_compIFaz->m_pEngineArea->GetSymbolTable();
}


CEngineArea* BackgroundCompiler::GetEngineArea() const
{
    return m_compIFaz->m_pEngineArea;
}
