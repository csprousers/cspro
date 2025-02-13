#pragma once

#include <zMessageO/zMessageO.h>
#include <zToolsO/CSProException.h>


// message formats
struct MessageFormat
{
    enum class Type
    {
        EscapedPercent,     // %%
        Integer,            // %d
        Double,             // %f
        String,             // %s
        Char,               // %c
        Proc,               // %p
        Variable,           // %v
        VariableLabel       // %l
    };

    Type type;
    std::optional<std::wstring> evaluated_formatter;
    size_t formatter_start_position;
    size_t formatter_end_position;
};


// message parameter evaluator
class MessageParameterEvaluator
{
public:
    virtual ~MessageParameterEvaluator() { }

    virtual MessageFormat::Type GetMessageFormatType(const MessageFormat& message_format) const { return message_format.type; }
    
    virtual bool ReplaceSpecialValuesWithSpaces() const { return false; }

    virtual int GetInteger() = 0;
    virtual double GetDouble() = 0;
    virtual std::wstring GetString() = 0;
    virtual wchar_t GetChar() = 0;
    virtual std::wstring GetProc() = 0;
    virtual std::wstring GetVariable() = 0;
    virtual std::wstring GetVariableLabel() = 0;

    CREATE_CSPRO_EXCEPTION(EvaluationException)
};
