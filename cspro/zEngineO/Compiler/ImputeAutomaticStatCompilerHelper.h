#pragma once

#include <zEngineO/Compiler/CompilerHelper.h>


class ImputeAutomaticStatCompilerHelper : public CompilerHelper
{
public:
    ImputeAutomaticStatCompilerHelper(LogicCompiler& logic_compiler)
        :   CompilerHelper(logic_compiler)
    {
    }

    const std::optional<bool>& GetAutomaticStatFlag() const { return m_automaticStat; }
    void SetAutomaticStatFlag(std::optional<bool> flag)     { m_automaticStat = std::move(flag); }

protected:
    bool IsCacheable() const override { return false; }

private:
    std::optional<bool> m_automaticStat;
};
