#pragma once

#include <engine/LogicStackSaver.h>
#include <stdexcept>


class ProgramControlException : public std::runtime_error
{
public:
    enum class Type { EndCase, SkipCase, Exit, Next, Break, EnterFlow };

    Type GetType() const { return m_type; }

protected:
    ProgramControlException(Type type)
        :   std::runtime_error("Program Control"),
            m_type(type)
    {
    }

private:
    Type m_type;
};


class EndCaseProgramControlException : public ProgramControlException
{
public:
    EndCaseProgramControlException() : ProgramControlException(Type::EndCase) { }
};


class SkipCaseProgramControlException : public ProgramControlException
{
public:
    SkipCaseProgramControlException() : ProgramControlException(Type::SkipCase) { }
};


class ExitProgramControlException : public ProgramControlException
{
public:
    ExitProgramControlException() : ProgramControlException(Type::Exit) { }
};


class NextProgramControlException : public ProgramControlException
{
public:
    NextProgramControlException() : ProgramControlException(Type::Next) { }
};


class BreakProgramControlException : public ProgramControlException
{
public:
    BreakProgramControlException() : ProgramControlException(Type::Break) { }
};


class EnterFlowProgramControlException : public ProgramControlException, public LogicStackSaver
{
public:
    EnterFlowProgramControlException() : ProgramControlException(Type::EnterFlow) { }
};
