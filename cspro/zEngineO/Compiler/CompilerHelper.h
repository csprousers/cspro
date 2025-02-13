#pragma once

class LogicCompiler;


// --------------------------------------------------------------------------
// CompilerHelper
// --------------------------------------------------------------------------

class CompilerHelper
{
    friend class LogicCompiler;

protected:
    CompilerHelper(LogicCompiler& logic_compiler)
        : m_compiler(&logic_compiler)
    {
    }

public:
    virtual ~CompilerHelper() { }

protected:
    // cacheable compiler helpers will be stored at the application level, separate from the compiler
    virtual bool IsCacheable() const = 0;

protected:
    LogicCompiler* m_compiler;
};
