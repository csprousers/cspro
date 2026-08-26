#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "Engdrv.h"
#include "Engine.h"
#include <zEngineO/Nodes/Messages.h>
#include <zMessageO/Messages.h>
#include <zMessageO/MessageEvaluator.h>
#include <zMessageO/MessageManager.h>
#include <zListingO/WriteFile.h>
#include <zBridgeO/NPff.h>
#include <zIssaLib/CsDriver.h>


// --------------------------------------------------------------------------
// MessageArgument +
// MessageArgumentsMessageParameterEvaluator
// --------------------------------------------------------------------------

struct MessageArgument
{
    std::variant<double, SharableString> value;
    int value_expression;
};


class MessageArgumentsMessageParameterEvaluator : public MessageParameterEvaluator
{
public:
    MessageArgumentsMessageParameterEvaluator(CIntDriver* interpreter, const std::vector<MessageArgument>& arguments, FunctionCode function_code);

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

private:
    CIntDriver* m_interpreter;
    const std::vector<MessageArgument>& m_arguments;
    size_t m_nextArgumentIndex;
    FunctionCode m_functionCode;
};


MessageArgumentsMessageParameterEvaluator::MessageArgumentsMessageParameterEvaluator(CIntDriver* const interpreter, const std::vector<MessageArgument>& arguments, const FunctionCode function_code)
    :   m_interpreter(interpreter),
        m_arguments(arguments),
        m_nextArgumentIndex(0),
        m_functionCode(function_code)
{
    ASSERT(m_interpreter != nullptr);
}


MessageFormat::Type MessageArgumentsMessageParameterEvaluator::GetMessageFormatType(const MessageFormat& message_format) const
{
    // process integers as doubles so that special values can be formatted properly
    return ( message_format.type == MessageFormat::Type::Integer ) ? MessageFormat::Type::Double :
                                                                     message_format.type;
}


bool MessageArgumentsMessageParameterEvaluator::ReplaceSpecialValuesWithSpaces() const
{
    return ( m_functionCode == FNWRITE_CODE || m_functionCode == FILEFN_WRITE_CODE );
}


int MessageArgumentsMessageParameterEvaluator::GetInteger()
{
    throw ProgrammingErrorException();
}


double MessageArgumentsMessageParameterEvaluator::GetDouble()
{
    const MessageArgument& argument = GetArgument(ArgumentType::Number);
    return std::get<double>(argument.value);
}


SharableString MessageArgumentsMessageParameterEvaluator::GetString()
{
    const MessageArgument& argument = GetArgument(ArgumentType::String);
    return std::get<SharableString>(argument.value);
}


std::variant<int, SharableString> MessageArgumentsMessageParameterEvaluator::GetChar()
{
    return MessageArgumentsMessageParameterEvaluator::GetString();
}


SharableString MessageArgumentsMessageParameterEvaluator::GetProc()
{
    return m_interpreter.GetCurrentProcName();
}


SharableString MessageArgumentsMessageParameterEvaluator::GetVariable()
{
    const MessageArgument& argument = GetArgument(ArgumentType::Either);
    return m_interpreter->EvaluateVariableParameter(argument.value, argument.value_expression, false);
}


SharableString MessageArgumentsMessageParameterEvaluator::GetVariableLabel()
{
    const MessageArgument& argument = GetArgument(ArgumentType::Either);
    return m_interpreter->EvaluateVariableParameter(argument.value, argument.value_expression, true);
}


const MessageArgument& MessageArgumentsMessageParameterEvaluator::GetArgument(const ArgumentType argument_type)
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

    throw MessageParameterEvaluator::EvaluationException(formatter->c_str(), ParameterTypes[expected_parameter_type_index], ParameterTypes[1 - expected_parameter_type_index]);
}



// --------------------------------------------------------------------------
// CIntDriver
// --------------------------------------------------------------------------

SharableString CIntDriver::EvaluateVariableParameter(const std::variant<double, SharableString>& value, const int value_expression, const bool request_label)
{
    const bool is_numeric = std::holds_alternative<double>(value);
    const int expression_type = GetNode<int>(value_expression);

    if( expression_type == FunctionCode::FN_VARIABLE_VALUE_CODE ) // CSPro 7.3+
    {
        const auto& variable_value_node = GetNode<Nodes::VariableValue>(value_expression);
        const VART* const pVarT = VPT(variable_value_node.symbol_index);
        const CDictItem* const dict_item = pVarT->GetDictItem();

        if( request_label && dict_item != nullptr )
        {
            return GetValueLabel(pVarT, value);
        }

        else if( is_numeric )
        {
            const VARX* const pVarX = pVarT->GetVarX();
            const double numeric_value = pVarX->varoutval(std::get<double>(value));

            std::wstring formatted_variable(pVarT->GetLength(), '\0');
            pVarT->dvaltochar(numeric_value, formatted_variable.data());

            return UTF8_TODO::GetUtf8(formatted_variable);
        }
    }

    return is_numeric ? SharableString(DoubleToString(std::get<double>(value))) :
                        std::get<SharableString>(value);
}


SharableString CIntDriver::EvaluateUserMessage(const int message_node_index, const FunctionCode function_code, int* const out_message_number/* = nullptr*/)
{
    const auto& message_node = GetNode<Nodes::Message>(message_node_index);
    MessageManager& user_message_manager = m_pEngineDriver->GetUserMessageManager();
    MessageEvaluator& user_message_evaluator = m_pEngineDriver->GetUserMessageEvaluator();

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
    std::vector<MessageArgument> arguments;

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
            else if( variable_value_node.function_code == FunctionCode::CHOBJ_CODE || variable_value_node.function_code == FunctionCode::FNCONCAT_CODE )
            {
                argument_data_type = DataType::String;
            }

            else
            {
                argument_data_type = DataType::Numeric;
            }
        }

        arguments.emplace_back(MessageArgument
            {
                EvaluateVariant<SharableString>(argument_data_type, argument_list_node.elements[i]),
                argument_list_node.elements[i]
            });
    }

    // format the message text
    MessageArgumentsMessageParameterEvaluator message_parameter_evaluator(this, arguments, function_code);

    if( unformatted_message_text.IsSet() )
    {
        return user_message_evaluator.GetFormattedMessage(message_parameter_evaluator, *unformatted_message_text);
    }

    else
    {
        return user_message_evaluator.GetFormattedMessage(message_parameter_evaluator, message_number);
    }
}


double CIntDriver::exerrmsg(const int program_index)
{
    if( !m_pEngineSettings->IsErrmsgMessageOn() )
        return 0;

    return DisplayUserMessage(program_index);
}


double CIntDriver::exdisplay(const int program_index)
{
    if( !m_pEngineSettings->IsDisplayMessageOn() )
        return 0;

    return DisplayUserMessage(program_index);
}


double CIntDriver::exwrite(const int program_index)
{
    ASSERT(m_pEngineDriver->GetWriteFile() != nullptr);

    m_pEngineDriver->GetWriteFile()->WriteLine(EvaluateUserMessage(program_index, FunctionCode::FNWRITE_CODE));

    return 1;
}


Engine::Value CIntDriver::exmaketext(const int program_index)
{
    return EvaluateUserMessage(program_index, FunctionCode::FNMAKETEXT_CODE);
}


double CIntDriver::exlogtext(const int program_index)
{
    if( !Paradata::Logger::IsOpen() )
        return 0;

    int message_number;
    SharableString message_text = EvaluateUserMessage(program_index, FunctionCode::FNLOGTEXT_CODE, &message_number);

    m_paradataDriver->RegisterAndLogEvent(m_paradataDriver->CreateMessageEvent(FunctionCode::FNLOGTEXT_CODE, message_number, std::move(message_text)));

    return 1;
}


double CIntDriver::exwarning(const int program_index)
{
    if( Issamod == ModuleType::Entry )
    {
        // if advancing, don't display the message
        const bool is_advancing = ( m_pCsDriver->GetSourceOfNodeAdvance() >= 0 ) ||
                                  ( m_pCsDriver->GetNumOfPendingAdvances() > 0 );

        if( is_advancing )
        {
            const auto& message_node = GetNode<Nodes::Message>(program_index);

            // return if there was no select statement
            if( message_node.extended_message_node_index == -1 )
                return 1;

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
                            Evaluate<double>(select_expression);

                        return default_button_number;
                    }
                }

                // if no default button, set the return value to the first continue value
                else
                {
                    for( int i = 0; i < select_movements_list_node.number_elements; ++i )
                    {
                        if( select_movements_list_node.elements[i] == -1 )
                            return i + 1;
                    }
                }
            }
        }
    }

    return DisplayUserMessage(program_index);
}


double CIntDriver::DisplayUserMessage(const int message_node_index)
{
    const auto& message_node = GetNode<Nodes::Message>(message_node_index);
    const Nodes::ExtendedMessage* extended_message_node = nullptr;

    MessageManager& user_message_manager = m_pEngineDriver->GetUserMessageManager();

    // evaluate the message
    int message_number;
    SharableString message_text = ConvertV0Escapes(EvaluateUserMessage(message_node_index, message_node.function_code, &message_number));

    // process the extended options
    std::unique_ptr<CEngineDriver::MessageSelectDetails> select_details;
    Nodes::ExtendedMessage::DisplayType display_type = Nodes::ExtendedMessage::DisplayType::Default;

    if( message_node.extended_message_node_index != -1 )
    {
        extended_message_node = &GetNode<Nodes::ExtendedMessage>(message_node.extended_message_node_index);

        // with variable-numbered messages, we need to set the denominator information at runtime
        if( extended_message_node->denominator_symbol_index != -1 )
            user_message_manager.AddDenominator(message_number, extended_message_node->denominator_symbol_index);

        // modify the default display type
        display_type = extended_message_node->display_type;

        // process any select options
        if( Issamod == ModuleType::Entry )
        {
            const Nodes::List& select_button_texts_list_node = GetListNode(extended_message_node->select_button_texts_list);

            if( select_button_texts_list_node.number_elements > 0 )
            {
                select_details = std::make_unique<CEngineDriver::MessageSelectDetails>();

                // add the button text
                for( int i = 0; i < select_button_texts_list_node.number_elements; ++i )
                    select_details->button_texts.emplace_back(Evaluate<SharableString>(select_button_texts_list_node.elements[i]));

                // check if there is a valid default button number
                if( extended_message_node->select_default_button_expression != -1 )
                    select_details->default_button_number = Evaluate<int>(extended_message_node->select_default_button_expression);

                if( select_details->default_button_number < 0 || select_details->default_button_number > static_cast<int>(select_details->button_texts.size()) )
                    select_details->default_button_number = 0;

                // the button number should be zero-based (or -1 if none specified)
                --select_details->default_button_number;
            }
        }
    }

    // it is possible to override the default display type in a batch application
    if( Issamod == ModuleType::Batch &&
        display_type == Nodes::ExtendedMessage::DisplayType::Default &&
        m_pEngineDriver->m_pPifFile->GetErrMsgOverride() != ErrMsgOverride::No )
    {
        display_type = ( m_pEngineDriver->m_pPifFile->GetErrMsgOverride() == ErrMsgOverride::Case ) ? Nodes::ExtendedMessage::DisplayType::Case :
                                                                                                      Nodes::ExtendedMessage::DisplayType::Summary;
    }

    // increment the message count if this isn't a case message
    if( display_type != Nodes::ExtendedMessage::DisplayType::Case )
    {
        user_message_manager.IncrementMessageCount(message_number);

        // if this is a summary message, return without displaying the message
        if( display_type == Nodes::ExtendedMessage::DisplayType::Summary )
            return 1;
    }


    // display the message until a proper response is given (showing the line number for unnumbered messages)
    const int message_number_for_display = user_message_manager.GetMessageNumberForDisplay(message_number);

    while( true )
    {
        // create paradata events if necessary
        std::unique_ptr<Paradata::MessageEvent> message_event;
        std::unique_ptr<Paradata::OperatorSelectionEvent> operator_selection_event;

        if( Paradata::Logger::IsOpen() )
        {
            message_event = m_paradataDriver->CreateMessageEvent(message_node.function_code, message_number, message_text);

            if( select_details != nullptr )
                operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(Paradata::OperatorSelectionEvent::Source::Errmsg);
        }

        // display the message
        const int selected_button_number = m_pEngineDriver->DisplayMessage(MessageType::User, message_number_for_display, std::move(message_text), select_details.get());

        ASSERT(selected_button_number > 0);

        // log the paradata events
        if( message_event != nullptr )
        {
            if( Issamod == ModuleType::Entry )
                message_event->SetPostDisplayReturnValue(selected_button_number);

            m_paradataDriver->RegisterAndLogEvent(std::move(message_event));
        }

        if( operator_selection_event != nullptr )
        {
            SharableString button_text = ( selected_button_number == 0 ) ? SharableString() :
                                                                           select_details->button_texts[selected_button_number - 1];

            operator_selection_event->SetPostSelectionValues(selected_button_number, std::move(button_text), true);
            m_paradataDriver->RegisterAndLogEvent(std::move(operator_selection_event));
        }

        // if not in a select statement, we are done, with the return value meaning success
        if( select_details == nullptr )
            return 1;

        // otherwise, the selected button number is returned, and if there is a movement
        // associated with the button, we need to execute the movement

        // the user must select a valid value
        if( selected_button_number == 0 )
            continue;

        // follow the selection
        const int select_expression = GetListNode(extended_message_node->select_movements_list).elements[selected_button_number - 1];

        // if not next or continue, execute the move command
        if( select_expression != -1 )
            Evaluate<double>(select_expression);

        // return the index of the button selected
        return selected_button_number;
    }
}


Engine::Value CIntDriver::ex_variablevalue(const int program_index)
{
    const auto& variable_value_node = GetNode<Nodes::VariableValue>(program_index);
    return Evaluate<Engine::Value>(variable_value_node.expression);
}
