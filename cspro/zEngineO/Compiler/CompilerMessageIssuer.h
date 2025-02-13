#pragma once

#include <zEngineO/Messages/MessageIssuer.h>
#include <zLogicO/BasicTokenCompiler.h>


class CompilerMessageIssuer : public MessageIssuer
{
public:
    CompilerMessageIssuer(Logic::BasicTokenCompiler& logic_compiler)
        :   m_compiler(logic_compiler)
    {
    }

    [[noreturn]] void IssueError(int message_number, ...) override
    {
        Logic::ParserError parser_error;

        va_list parg;
        va_start(parg, message_number);

        m_compiler.IssueMessage(parser_error, message_number, parg);

        va_end(parg);

        throw parser_error;
    }

private:
    Logic::BasicTokenCompiler& m_compiler;
};
