#pragma once

#include <zEngineO/LoopStack.h>
#include <zEngineO/Compiler/CompilerHelper.h>
#include <zEngineO/Compiler/CompilerMessageIssuer.h>


class LoopStackCompilerHelper : public CompilerHelper, public LoopStack
{
public:
    LoopStackCompilerHelper(LogicCompiler& logic_compiler)
        :   CompilerHelper(logic_compiler),
            m_compilerMessageIssuer(logic_compiler)
    {
    }

protected:
    // CompilerHelper overrides
    bool IsCacheable() const override { return false; }

    // LoopStack overrides
    MessageIssuer& GetMessageIssuer() override { return m_compilerMessageIssuer; }

private:
    CompilerMessageIssuer m_compilerMessageIssuer;
};
