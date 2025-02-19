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
    std::optional<std::string> evaluated_formatter;
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
    virtual SharableString GetString() = 0;
    virtual std::variant<int, SharableString> GetChar() = 0;
    virtual SharableString GetProc() = 0;
    virtual SharableString GetVariable() = 0;
    virtual SharableString GetVariableLabel() = 0;

    CREATE_CSPRO_EXCEPTION(EvaluationException);
};
