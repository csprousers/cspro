#include "stdafx.h"
#include <zMessageO/MessageEvaluator.h>
#include <zMessageO/MessageFile.h>
#include <zMessageO/SystemMessages.h>


std::wstring ActionInvoker::Runtime::GetMessageText(const JsonNode<wchar_t>& json_node, const Action action)
{
    ASSERT(action == Action::Message_formatText || action == Action::Message_getText);

    const std::optional<int> message_number =
        ( action == Action::Message_getText || json_node.Contains(JK::number) ) ? std::make_optional(json_node.Get<int>(JK::number)) :
                                                                                  std::nullopt;

    if( message_number.has_value() )
    {
        const MessageFile* message_file;

        // the message can come from system messages...
        if( json_node.Contains(JK::type) &&
            json_node.GetFromStringOptions(JK::type, std::initializer_list<const TCHAR*>({ _T("user"), _T("system") })) == 1 )
        {
            message_file = &SystemMessages::GetMessageFile();
        }

        // ...or user messages (the default)
        else
        {
            try
            {
                message_file = &GetInterpreterAccessor().GetUserMessageFile();
            }

            catch(...)
            {
                throw CSProException("No user messages exist in the current environment.");
            }
        }

        const std::wstring* message_text = message_file->GetMessageTextWithNoDefaultMessage(*message_number);

        if( message_text != nullptr )
            return *message_text;
    }

    if( json_node.Contains(JK::text) )
        return json_node.Get<std::wstring>(JK::text);

    if( message_number.has_value() )
        throw CSProException(_T("No message number '%d' exists."), *message_number);

    throw CSProException("No message text specified.");
}


ActionInvoker::Result ActionInvoker::Runtime::Message_getText(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    return Result::String(GetMessageText(json_node, Action::Message_getText));
}


namespace
{
    class ActionInvokerMessageParameterEvaluator : public MessageParameterEvaluator
    {
    public:
        ActionInvokerMessageParameterEvaluator(std::optional<JsonNodeArray<wchar_t>> arguments_array);

    protected:
        MessageFormat::Type GetMessageFormatType(const MessageFormat& message_format) const override;
        int GetInteger() override;
        double GetDouble() override;
        std::wstring GetString() override;
        wchar_t GetChar() override;
        std::wstring GetProc() override;
        std::wstring GetVariable() override;
        std::wstring GetVariableLabel() override;

    private:
        template<typename CF>
        auto EvaluateArgument(CF callback_function);

    private:
        std::optional<JsonNodeArray<wchar_t>> m_argumentsArray;
        const size_t m_numberArguments;
        size_t m_nextArgumentIndex;
    };


    ActionInvokerMessageParameterEvaluator::ActionInvokerMessageParameterEvaluator(std::optional<JsonNodeArray<wchar_t>> arguments_array)
        :   m_argumentsArray(std::move(arguments_array)),
            m_numberArguments(m_argumentsArray.has_value() ? m_argumentsArray->size() : 0),
            m_nextArgumentIndex(0)
    {
    }


    template<typename CF>
    auto ActionInvokerMessageParameterEvaluator::EvaluateArgument(CF callback_function)
    {
        const size_t current_argument_index = m_nextArgumentIndex++;

        if( current_argument_index >= m_numberArguments )
        {
            throw EvaluationException(_T("<no argument provided for replacement #%d>"),
                                      static_cast<int>(current_argument_index) + 1);
        }

        try
        {
            return callback_function((*m_argumentsArray)[current_argument_index]);
        }

        catch( const CSProException& exception )
        {
            throw EvaluationException(_T("<invalid argument for replacement #%d: %s>"),
                                      static_cast<int>(current_argument_index) + 1,
                                      exception.GetErrorMessage().c_str());
        }
    }


    MessageFormat::Type ActionInvokerMessageParameterEvaluator::GetMessageFormatType(const MessageFormat& message_format) const
    {
        // process integers as doubles so that special values can be formatted properly
        return ( message_format.type == MessageFormat::Type::Integer ) ? MessageFormat::Type::Double :
                                                                         message_format.type;
    }


    int ActionInvokerMessageParameterEvaluator::GetInteger()
    {
        throw ProgrammingErrorException();
    }


    double ActionInvokerMessageParameterEvaluator::GetDouble()
    {
        return EvaluateArgument(
            [](const JsonNode<wchar_t>& json_node)
            {
                return json_node.GetEngineValue<double>();
            });
    }


    std::wstring ActionInvokerMessageParameterEvaluator::GetString()
    {
        return EvaluateArgument(
            [](const JsonNode<wchar_t>& json_node)
            {
                return json_node.GetEngineValue<std::wstring>();
            });
    }


    wchar_t ActionInvokerMessageParameterEvaluator::GetChar()
    {
        const std::wstring text = GetString();
        return !text.empty() ? text.front() : ' ';
    }


    std::wstring ActionInvokerMessageParameterEvaluator::GetProc()
    {
        return std::wstring();
    }


    std::wstring ActionInvokerMessageParameterEvaluator::GetVariable()
    {
        return GetString();
    }


    std::wstring ActionInvokerMessageParameterEvaluator::GetVariableLabel()
    {
        return GetString();
    }
}


ActionInvoker::Result ActionInvoker::Runtime::Message_formatText(const JsonNode<wchar_t>& json_node, Caller& /*caller*/)
{
    std::wstring unformatted_message_text = GetMessageText(json_node, Action::Message_formatText);

    // we can return the string directly if there are no formats in the message
    if( unformatted_message_text.find('%') == std::wstring::npos )
        return Result::String(std::move(unformatted_message_text));

    // use a dummy message file since we are not using message numbers at this point
    if( m_messageEvaluator == nullptr )
        m_messageEvaluator = std::make_unique<MessageEvaluator>(std::make_unique<MessageFile>());

    ActionInvokerMessageParameterEvaluator message_parameter_evaluator(json_node.Contains(JK::arguments) ? std::make_optional(json_node.GetArray(JK::arguments)) :
                                                                                                           std::nullopt);

    return Result::String(m_messageEvaluator->GetFormattedMessage(message_parameter_evaluator, unformatted_message_text));
}
