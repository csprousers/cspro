#include "stdafx.h"
#include "IncludesRT.h"
#include "Nodes/Messages.h"
#include <engine/Settings.h>
#include <engine/VarT.h>
#include <zMessageO/Messages.h>
#include <zMessageO/MessageEvaluator.h>
#include <zMessageO/MessageManager.h>
#include <zMessageO/RuntimeMessage.h>
#include <zListingO/WriteFile.h>


namespace MessagesRT
{
    struct MessageArgument;
    class MessageArgumentsMessageParameterEvaluator;
}


// --------------------------------------------------------------------------
// MessageArgument +
// MessageArgumentsMessageParameterEvaluator
// --------------------------------------------------------------------------

struct MessagesRT::MessageArgument
{
    std::variant<double, SharableString> value;
    int value_expression;
};


class MessagesRT::MessageArgumentsMessageParameterEvaluator : public MessageParameterEvaluator
{
public:
    MessageArgumentsMessageParameterEvaluator(LogicInterpreter& interpreter, const std::vector<MessageArgument>& arguments,
                                              FunctionCode function_code);

    MessageFormat::Type GetMessageFormatType(const MessageFormat& message_format) const override;
    bool ReplaceSpecialValuesWithSpaces() const override;
    int GetInteger() override;
    double GetDouble() override;
    SharableString GetString() override;
    std::variant<int, SharableString> GetChar() override;
    SharableString GetProc() override;
    SharableString GetVariable() override;
    SharableString GetVariableLabel() override;

private:
    enum class ArgumentType { Number, String, Either };

    const MessageArgument& GetArgument(ArgumentType argument_type);

    // Evaluates the %v variable formatter.
    SharableString EvaluateMessageVariableArgument(const std::variant<double, SharableString>& value,
                                                   int value_expression, bool request_label);

private:
    LogicInterpreter& m_interpreter;
    const std::vector<MessageArgument>& m_arguments;
    size_t m_nextArgumentIndex;
    FunctionCode m_functionCode;
};


MessagesRT::MessageArgumentsMessageParameterEvaluator::MessageArgumentsMessageParameterEvaluator(
    LogicInterpreter& interpreter, const std::vector<MessageArgument>& arguments, const FunctionCode function_code)
    :   m_interpreter(interpreter),
        m_arguments(arguments),
        m_nextArgumentIndex(0),
        m_functionCode(function_code)
{
}


MessageFormat::Type MessagesRT::MessageArgumentsMessageParameterEvaluator::GetMessageFormatType(const MessageFormat& message_format) const
{
    // process integers as doubles so that special values can be formatted properly
    return ( message_format.type == MessageFormat::Type::Integer ) ? MessageFormat::Type::Double :
                                                                     message_format.type;
}


bool MessagesRT::MessageArgumentsMessageParameterEvaluator::ReplaceSpecialValuesWithSpaces() const
{
    return ( m_functionCode == FNWRITE_CODE ||
             m_functionCode == FILEFN_WRITE_CODE );
}


int MessagesRT::MessageArgumentsMessageParameterEvaluator::GetInteger()
{
    throw ProgrammingErrorException();
}


double MessagesRT::MessageArgumentsMessageParameterEvaluator::GetDouble()
{
    const MessageArgument& argument = GetArgument(ArgumentType::Number);
    return std::get<double>(argument.value);
}


SharableString MessagesRT::MessageArgumentsMessageParameterEvaluator::GetString()
{
    const MessageArgument& argument = GetArgument(ArgumentType::String);
    return std::get<SharableString>(argument.value);
}


std::variant<int, SharableString> MessagesRT::MessageArgumentsMessageParameterEvaluator::GetChar()
{
    return MessageArgumentsMessageParameterEvaluator::GetString();
}


SharableString MessagesRT::MessageArgumentsMessageParameterEvaluator::GetProc()
{
    return m_interpreter.GetCurrentProcName();
}


SharableString MessagesRT::MessageArgumentsMessageParameterEvaluator::GetVariable()
{
    const MessageArgument& argument = GetArgument(ArgumentType::Either);
    return EvaluateMessageVariableArgument(argument.value, argument.value_expression, false);
}


SharableString MessagesRT::MessageArgumentsMessageParameterEvaluator::GetVariableLabel()
{
    const MessageArgument& argument = GetArgument(ArgumentType::Either);
    return EvaluateMessageVariableArgument(argument.value, argument.value_expression, true);
}


const MessagesRT::MessageArgument& MessagesRT::MessageArgumentsMessageParameterEvaluator::GetArgument(const ArgumentType argument_type)
{
    if( m_nextArgumentIndex >= m_arguments.size() )
        throw MessageParameterEvaluator::EvaluationException(MGF::GetMessageText(MGF::InvalidMessageParameterNumber).GetString());

    const MessageArgument& argument = m_arguments[m_nextArgumentIndex++];

    if( ( argument_type != ArgumentType::String && std::holds_alternative<double>(argument.value) ) ||
        ( argument_type != ArgumentType::Number && std::holds_alternative<SharableString>(argument.value) ) )
    {
        return argument;
    }

    // data type error
    constexpr const char* ParameterTypes[] = { "numeric", "string" };
    const size_t expected_parameter_type_index = ( argument_type == ArgumentType::Number ) ? 0 : 1;
    const SharableString formatter = MGF::GetMessageText(MGF::InvalidMessageParameterCount);

    throw MessageParameterEvaluator::EvaluationException(
        formatter->c_str(),
        ParameterTypes[expected_parameter_type_index],
        ParameterTypes[1 - expected_parameter_type_index]
    );
}


SharableString MessagesRT::MessageArgumentsMessageParameterEvaluator::EvaluateMessageVariableArgument(
    const std::variant<double, SharableString>& value, const int value_expression, const bool request_label)
{
    const bool is_numeric = std::holds_alternative<double>(value);
    const int expression_type = m_interpreter.GetNode<int>(value_expression);

    if( expression_type == FunctionCode::FN_VARIABLE_VALUE_CODE ) // CSPro 7.3+
    {
        const auto& variable_value_node = m_interpreter.GetNode<Nodes::VariableValue>(value_expression);
        const VART& vart = m_interpreter.GetSymbol<VART>(variable_value_node.symbol_index);
        const CDictItem* const dict_item = vart.GetDictItem();

        if( request_label && dict_item != nullptr )
        {
            return m_interpreter.GetItemValueLabel(vart, value);
        }

        else if( is_numeric )
        {
            return vart.dvaltochar(std::get<double>(value), true);
        }
    }

    return is_numeric ? SharableString(DoubleToString(std::get<double>(value))) :
                        std::get<SharableString>(value);
}



// --------------------------------------------------------------------------
// message routines
// --------------------------------------------------------------------------

SharableString LogicInterpreter::EvaluateUserMessage(const int message_node_index, const FunctionCode function_code,
                                                     int* const out_message_number/* = nullptr*/)
{
    const auto& message_node = GetNode<Nodes::Message>(message_node_index);
    MessageManager& user_message_manager = GetUserMessageManager_INTERPRETER_DLL_TODO();
    MessageEvaluator& user_message_evaluator = GetUserMessageEvaluator_INTERPRETER_DLL_TODO();

    // get the message number
    int message_number;
    SharableString unformatted_message_text;

    // a variable-numbered message
    if( message_node.message_number == -1 )
    {
        message_number = Evaluate<int>(message_node.message_expression);
    }

    // constant message number or a string-based message
    else
    {
        message_number = message_node.message_number;

        // for messages that are not in the message file, evaluate the message text
        if( message_node.message_expression != -1 )
        {
            unformatted_message_text = Evaluate<SharableString>(message_node.message_expression);
            user_message_manager.UpdateUnnumberedMessageText(message_number, unformatted_message_text);
        }
    }

    if( out_message_number != nullptr )
        *out_message_number = message_number;


    // evaluate any arguments passed along with the message
    const auto& argument_list_node = GetListNode(message_node.argument_list);
    std::vector<MessagesRT::MessageArgument> arguments;

    for( int i = 0; i < argument_list_node.number_elements; ++i )
    {
        DataType argument_data_type;

        if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        {
            argument_data_type = static_cast<DataType>(argument_list_node.elements[i]);
            ++i;
        }

        else
        {
            auto& variable_value_node = GetNode<Nodes::VariableValue>(argument_list_node.elements[i]);

            if( variable_value_node.function_code == FunctionCode::FN_VARIABLE_VALUE_CODE )
            {
                argument_data_type = VPT(variable_value_node.symbol_index)->GetDataType();
            }

            // added the second argument for strings combined with the + operator
            else if( variable_value_node.function_code == FunctionCode::CHOBJ_CODE ||
                     variable_value_node.function_code == FunctionCode::FNCONCAT_CODE )
            {
                argument_data_type = DataType::String;
            }

            else
            {
                argument_data_type = DataType::Numeric;
            }
        }

        arguments.emplace_back(MessagesRT::MessageArgument
            {
                EvaluateVariant<SharableString>(argument_data_type, argument_list_node.elements[i]),
                argument_list_node.elements[i]
            });
    }

    // format the message text
    MessagesRT::MessageArgumentsMessageParameterEvaluator message_parameter_evaluator(*this, arguments, function_code);

    return unformatted_message_text.IsSet()
        ? user_message_evaluator.GetFormattedMessage(message_parameter_evaluator, *unformatted_message_text)
        : user_message_evaluator.GetFormattedMessage(message_parameter_evaluator, message_number);
}



// --------------------------------------------------------------------------
// message functions
// --------------------------------------------------------------------------

Engine::Value LogicInterpreter::ex_errmsg(const int program_index)
{
    const CSettings* const settings = GetSettings_INTERPRETER_DLL_TODO();

    if( settings != nullptr && !settings->IsErrmsgMessageOn() )
        return Engine::Value::Integer(0);

    return DisplayUserMessage(program_index);
}


Engine::Value LogicInterpreter::ex_display(const int program_index)
{
    const CSettings* const settings = GetSettings_INTERPRETER_DLL_TODO();

    if( settings != nullptr && !settings->IsDisplayMessageOn() )
        return Engine::Value::Integer(0);

    return DisplayUserMessage(program_index);
}


Engine::Value LogicInterpreter::ex_write(const int program_index)
{
    Listing::WriteFile* const write_file = GetWriteFile_INTERPRETER_DLL_TODO();

    if( write_file == nullptr )
        return Engine::Value::Bool(false);

    write_file->WriteLine(EvaluateUserMessage(program_index, FunctionCode::FNWRITE_CODE));

    return Engine::Value::Bool(true);
}


Engine::Value LogicInterpreter::ex_maketext(const int program_index)
{
    return EvaluateUserMessage(program_index, FunctionCode::FNMAKETEXT_CODE);
}


Engine::Value LogicInterpreter::ex_logtext(const int program_index)
{
    if( !Paradata::Logger::IsOpen() )
        return Engine::Value::Bool(false);

    int message_number;
    SharableString message_text = EvaluateUserMessage(program_index, FunctionCode::FNLOGTEXT_CODE, &message_number);

    GetParadataDriver_INTERPRETER_DLL_TODO().RegisterAndLogEvent(
        GetParadataDriver_INTERPRETER_DLL_TODO().CreateMessageEvent(
            FunctionCode::FNLOGTEXT_CODE, message_number, std::move(message_text)
        )
    );

    return Engine::Value::Bool(true);
}


Engine::Value LogicInterpreter::ex_warning(const int program_index)
{
    // if advancing, don't display the message
    if( GetEngineAppType() == EngineAppType::Entry &&
        InAdvance_INTERPRETER_DLL_TODO() )
    {
        const auto& message_node = GetNode<Nodes::Message>(program_index);

        // return if there was no select statement
        if( message_node.extended_message_node_index == -1 )
            return Engine::Value::Integer(1);

        // otherwise follow the route that doesn't require operator intervention
        const auto& extended_message_node = GetNode<Nodes::ExtendedMessage>(message_node.extended_message_node_index);
        const auto& select_movements_list_node = GetListNode(extended_message_node.select_movements_list);

        if( select_movements_list_node.number_elements > 0 )
        {
            if( extended_message_node.select_default_button_expression != -1 )
            {
                const int default_button_number = Evaluate<int>(extended_message_node.select_default_button_expression);

                if( default_button_number >= 1 && default_button_number <= select_movements_list_node.number_elements )
                {
                    const int select_expression = select_movements_list_node.elements[default_button_number - 1];

                    if( select_expression != -1 )
                        ExecuteInstruction(select_expression);

                    return Engine::Value::Integer(default_button_number);
                }
            }

            // if no default button, set the return value to the first continue value
            else
            {
                for( int i = 0; i < select_movements_list_node.number_elements; ++i )
                {
                    if( select_movements_list_node.elements[i] == -1 )
                        return Engine::Value::Integer(i + 1);
                }
            }
        }
    }

    return DisplayUserMessage(program_index);
}


Engine::Value LogicInterpreter::ex_variablevalue(const int program_index)
{
    const auto& variable_value_node = GetNode<Nodes::VariableValue>(program_index);
    return Evaluate<Engine::Value>(variable_value_node.expression);
}


Engine::Value LogicInterpreter::DisplayUserMessage(const int message_node_index)
{
    const auto& message_node = GetNode<Nodes::Message>(message_node_index);
    const Nodes::ExtendedMessage* extended_message_node = nullptr;

    MessageManager& user_message_manager = GetUserMessageManager_INTERPRETER_DLL_TODO();

    // evaluate the message
    RuntimeMessage runtime_message;

    runtime_message.message_text = ConvertV0Escapes(
        EvaluateUserMessage(message_node_index, message_node.function_code, &runtime_message.message_number)
    );

    // display messages showing the line number for unnumbered messages
    runtime_message.message_number_for_display = user_message_manager.GetMessageNumberForDisplay(runtime_message.message_number);

    // process the extended options
    Nodes::ExtendedMessage::DisplayType display_type = Nodes::ExtendedMessage::DisplayType::Default;
    const EngineAppType engine_app_type = GetEngineAppType();

    if( message_node.extended_message_node_index != -1 )
    {
        extended_message_node = &GetNode<Nodes::ExtendedMessage>(message_node.extended_message_node_index);

        // with variable-numbered messages, we need to set the denominator information at runtime
        if( extended_message_node->denominator_symbol_index != -1 )
            user_message_manager.AddDenominator(runtime_message.message_number, extended_message_node->denominator_symbol_index);

        // modify the default display type
        display_type = extended_message_node->display_type;

        // process any select options
        if( engine_app_type == EngineAppType::Entry )
        {
            const Nodes::List& select_button_texts_list_node = GetListNode(extended_message_node->select_button_texts_list);

            if( select_button_texts_list_node.number_elements > 0 )
            {
                runtime_message.select_buttons = std::make_unique<RuntimeMessage::SelectButtons>();

                // add the button text
                for( int i = 0; i < select_button_texts_list_node.number_elements; ++i )
                {
                    runtime_message.select_buttons->button_texts.emplace_back(
                        Evaluate<SharableString>(select_button_texts_list_node.elements[i])
                    );
                }

                // check if there is a valid default button number
                if( extended_message_node->select_default_button_expression != -1 )
                {
                    // - 1 because the button index provided in logic is one-based
                    runtime_message.select_buttons->default_button_number =
                        Evaluate<size_t>(extended_message_node->select_default_button_expression) - 1;

                    if( runtime_message.select_buttons->default_button_number >= runtime_message.select_buttons->button_texts.size() )
                        runtime_message.select_buttons->default_button_number.reset();
                }
            }
        }
    }

    // it is possible to override the default display type in a batch application
    if( engine_app_type == EngineAppType::Batch &&
        display_type == Nodes::ExtendedMessage::DisplayType::Default &&
        m_engineData->pff != nullptr &&
        m_engineData->pff->GetErrMsgOverride() != ErrMsgOverride::No )
    {
        display_type = ( m_engineData->pff->GetErrMsgOverride() == ErrMsgOverride::Case )
            ? Nodes::ExtendedMessage::DisplayType::Case
            : Nodes::ExtendedMessage::DisplayType::Summary;
    }

    // increment the message count if this isn't a case message
    if( display_type != Nodes::ExtendedMessage::DisplayType::Case )
    {
        user_message_manager.IncrementMessageCount(runtime_message.message_number);

        // if this is a summary message, return without displaying the message
        if( display_type == Nodes::ExtendedMessage::DisplayType::Summary )
            return Engine::Value::Integer(1);
    }


    // display the message until a proper response is given
    while( true )
    {
        // create paradata events if necessary
        std::unique_ptr<Paradata::MessageEvent> message_event;
        std::unique_ptr<Paradata::OperatorSelectionEvent> operator_selection_event;

        if( Paradata::Logger::IsOpen() )
        {
            message_event = GetParadataDriver_INTERPRETER_DLL_TODO().CreateMessageEvent(
                message_node.function_code, runtime_message.message_number, runtime_message.message_text
            );

            if( runtime_message.select_buttons != nullptr )
            {
                operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(
                    Paradata::OperatorSelectionEvent::Source::Errmsg
                );
            }
        }

        // display the message
        const int selected_button_number = DisplayMessage_INTERPRETER_DLL_TODO(MessageType::User, runtime_message);
        ASSERT(selected_button_number > 0);

        // log the paradata events
        if( message_event != nullptr )
        {
            if( engine_app_type == EngineAppType::Entry )
                message_event->SetPostDisplayReturnValue(selected_button_number);

            GetParadataDriver_INTERPRETER_DLL_TODO().RegisterAndLogEvent(std::move(message_event));
        }

        if( operator_selection_event != nullptr )
        {
            operator_selection_event->SetPostSelectionValues(
                selected_button_number,
                ( selected_button_number == 0 ) ? SharableString() : runtime_message.select_buttons->button_texts[selected_button_number - 1],
                true // set_display_duration
            );

            GetParadataDriver_INTERPRETER_DLL_TODO().RegisterAndLogEvent(std::move(operator_selection_event));
        }

        // if not in a select statement, we are done, with the return value meaning success
        if( runtime_message.select_buttons == nullptr )
            return Engine::Value::Integer(1);

        // otherwise, the selected button number is returned, and if there is a movement
        // associated with the button, we need to execute the movement

        // the user must select a valid value
        if( selected_button_number == 0 )
            continue;

        // follow the selection
        const int select_expression = GetListNode(extended_message_node->select_movements_list).elements[selected_button_number - 1];

        // if not next or continue, execute the move command
        if( select_expression != -1 )
            ExecuteInstruction(select_expression);

        // return the index of the button selected
        return Engine::Value::Integer(selected_button_number);
    }
}
