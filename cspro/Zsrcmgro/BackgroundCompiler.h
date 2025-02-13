#pragma once

#include <Zsrcmgro/zSrcMgrO.h>

class Application;
class CCompIFaz;
class CEngineArea;
class CompilerCreator;
class DesignerCompilerMessageProcessor;

namespace Logic
{
    class SourceBuffer;
    class SymbolTable;
}


class CLASS_DECL_ZSRCMGR BackgroundCompiler
{
public:
    BackgroundCompiler(Application& application, DesignerCompilerMessageProcessor* designer_compiler_message_processor = nullptr,
                       CompilerCreator* compiler_creator = nullptr);
    virtual ~BackgroundCompiler();

    void Compile(const TCHAR* buffer_text);
    void Compile(std::shared_ptr<Logic::SourceBuffer> source_buffer);

    const Logic::SymbolTable& GetCompiledSymbolTable() const;

    CEngineArea* GetEngineArea() const;

protected:
    const Logic::SymbolTable& GetSymbolTable() const;

protected:
    std::unique_ptr<CCompIFaz> m_compIFaz;
};
