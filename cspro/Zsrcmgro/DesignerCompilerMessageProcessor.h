#pragma once

#include <zLogicO/ParserMessage.h>

class CEngineDriver;


// a base class for CCompiler and DesignerCapiLogicCompiler message processing
class DesignerCompilerMessageProcessor
{
public:
    virtual ~DesignerCompilerMessageProcessor() { }

    virtual CEngineDriver* GetEngineDriver() = 0;    
    virtual CString GetProcName() const = 0;
    virtual int GetLineNumberOfCurrentCompile() const = 0;

    const std::vector<Logic::ParserMessage>& GetParserMessages() const { return m_parserMessages; }

    void ClearParserMessages();

    void AddParserMessage(Logic::ParserMessage parser_message);

private:
    std::vector<Logic::ParserMessage> m_parserMessages;
};



inline void DesignerCompilerMessageProcessor::ClearParserMessages()
{
    m_parserMessages.clear();
}


inline void DesignerCompilerMessageProcessor::AddParserMessage(Logic::ParserMessage parser_message)
{
    // no line number for CAPI logic
    if( std::holds_alternative<CapiLogicLocation>(parser_message.extended_location) )
    {
        parser_message.line_number = 0;
    }

    // if the line number is not set, assign it to the first line number for the current PROC;
    // this will generally occur during a full compilation with errors from CEngineCompFunc::CreateProcDirectory
    else if( parser_message.line_number == 0 )
    {
        ASSERT(std::holds_alternative<std::monostate>(parser_message.extended_location));

        parser_message.line_number = GetLineNumberOfCurrentCompile();
        parser_message.proc_name.clear();
    }

    m_parserMessages.emplace_back(std::move(parser_message));
}
