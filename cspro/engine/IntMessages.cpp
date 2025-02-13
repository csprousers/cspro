#include "StandardSystemIncludes.h"
#include "COMPILAD.H"
#include "Engdrv.h"
#include "Engine.h"
#include "Exappl.h"
#include <zEngineO/Versioning.h>
#include <zEngineO/Nodes/Messages.h>
#include <zToolsO/Tools.h>
#include <zMessageO/Messages.h>
#include <zMessageO/MessageEvaluator.h>
#include <zMessageO/MessageManager.h>
#include <zListingO/WriteFile.h>
#include <ZBRIDGEO/npff.h>
#include <zParadataO/Logger.h>
#include <Zissalib/CsDriver.h>


namespace
{
    struct MessageArgument
    {
        std::variant<double, std::wstring> value;
        int value_expression;
    };

    class MessageArgumentsMessageParameterEvaluator : public MessageParameterEvaluator
    {
    public:
        MessageArgumentsMessageParameterEvaluator(CIntDriver* pIntDriver, const std::vector<MessageArgument>& arguments, FunctionCode function_code)
            :   m_pIntDriver(pIntDriver),
                m_arguments(arguments),
                m_nextArgumentIndex(0),
                m_functionCode(function_code)
        {
            ASSERT(m_pIntDriver != nullptr);
        }

        MessageFormat::Type GetMessageFormatType(const MessageFormat& message_format) const override
        {
            // process integers as doubles so that special values can be formatted properly
            return ( message_format.type == MessageFormat::Type::Integer ) ? MessageFormat::Type::Double :
                                                                             message_format.type;
        }

        bool ReplaceSpecialValuesWithSpaces() const override
        {
            return ( m_functionCode == FNWRITE_CODE || m_functionCode == FNFILE_WRITE_CODE );
        }

        int GetInteger() override
        {
            throw ProgrammingErrorException();
        }

        double GetDouble() override
        {
            const MessageArgument& argument = GetArgument(ArgumentType::Number);
            return std::get<double>(argument.value);
        }

        std::wstring GetString() override
        {
            const MessageArgument& argument = GetArgument(ArgumentType::String);
            return std::get<std::wstring>(argument.value);
        }

        wchar_t GetChar() override
        {
            const MessageArgument& argument = GetArgument(ArgumentType::String);
            const std::wstring& text = std::get<std::wstring>(argument.value);
            return text.empty() ? 0 : text.front();
        }

        std::wstring GetProc() override
        {
            return CS2WS(m_pIntDriver->ProcName());
        }

        std::wstring GetVariable() override
        {
            const MessageArgument& argument = GetArgument(ArgumentType::Either);
            return m_pIntDriver->EvaluateVariableParameter(argument.value, argument.value_expression, false);
        }

        std::wstring GetVariableLabel() override
        {
            const MessageArgument& argument = GetArgument(ArgumentType::Either);
            return m_pIntDriver->EvaluateVariableParameter(argument.value, argument.value_expression, true);
        }

    private:
        enum class ArgumentType { Number, String, Either };

        const MessageArgument& GetArgument(ArgumentType argument_type)
        {
            if( m_nextArgumentIndex >= m_arguments.size() )
                throw MessageParameterEvaluator::EvaluationException(MGF::GetMessageText(MGF::InvalidMessageParameterNumber));

            const MessageArgument& argument = m_arguments[m_nextArgumentIndex++];

            if( ( argument_type != ArgumentType::String && std::holds_alternative<double>(argument.value) ) ||
                ( argument_type != ArgumentType::Number && std::holds_alternative<std::wstring>(argument.value) ) )
            {
                return argument;
            }

            // data type error
            constexpr const TCHAR* ParameterTypes[] = { _T("numeric"), _T("string") };
            const size_t expected_parameter_type_index = ( argument_type == ArgumentType::Number ) ? 0 : 1;
            const std::wstring& formatter = MGF::GetMessageText(MGF::InvalidMessageParameterCount);

            throw MessageParameterEvaluator::EvaluationException(formatter.c_str(), ParameterTypes[expected_parameter_type_index], ParameterTypes[1 - expected_parameter_type_index]);
        }

    private:
        CIntDriver* m_pIntDriver;
        const std::vector<MessageArgument>& m_arguments;
        size_t m_nextArgumentIndex;
        FunctionCode m_functionCode;
    };
}


std::wstring CIntDriver::EvaluateVariableParameter(const std::variant<double, std::wstring>& value, int value_expression, bool request_label)
{
    bool is_numeric = std::holds_alternative<double>(value);
    int expression_type = GetNode<int>(value_expression);

    if( expression_type == FunctionCode::FN_VARIABLE_VALUE_CODE ) // CSPro 7.3+
    {
        const auto& variable_value_node = GetNode<Nodes::VariableValue>(value_expression);
        const VART* pVarT = VPT(variable_value_node.symbol_index);
        const CDictItem* dict_item = pVarT->GetDictItem();

        if( request_label && dict_item != nullptr )
        {
            return GetValueLabel(pVarT, value);
        }

        else if( is_numeric )
        {
            const VARX* pVarX = pVarT->GetVarX();
            double numeric_value = pVarX->varoutval(std::get<double>(value));

            std::wstring formatted_variable(pVarT->GetLength(), '\0');
            pVarT->dvaltochar(numeric_value, formatted_variable.data());

            return formatted_variable;
        }
    }

    return is_numeric ? DoubleToString(std::get<double>(value)) : 
                        std::get<std::wstring>(value);
}


namespace
{
    // for pre-7.5 compilation
    typedef struct {                       // victor Oct 05, 00
        // specific for DISPLAY & ERRMSG (formerly FNN)
        int     fn_code;
        int     m_iMsgNum;                 // 0: interpreted thru a num-expr

        unsigned short m_denomCaseSummary; // this was previously two booleans
        enum class DenomCaseSummaryMask : unsigned short { DenomIsLastArgument = 0x8000, CaseDisplay = 0x0100, SummaryDisplay = 0x0001 };

        int     iButtonTitleList;          // Buttons text, -1 if none
        int     iButtonReenterList;        // Reenter list, -1 if NEXT used
        int     iDefaultButton;            // Default button. -1 if no default

        int     fn_nargs;
        int     fn_expr[1];
    } FNMSG_NODE;
}


std::wstring CIntDriver::EvaluateUserMessage(int message_node_index, FunctionCode function_code, int* out_message_number/* = nullptr*/)
{
    const auto& message_node = GetNode<Nodes::Message>(message_node_index);
    MessageManager& user_message_manager = m_pEngineDriver->GetUserMessageManager();
    MessageEvaluator& user_message_evaluator = m_pEngineDriver->GetUserMessageEvaluator();

    // get the message number
    int message_number;
    std::optional<std::wstring> unformatted_message_text;

    // a variable-numbered message
    if( message_node.message_number == -1 )
    {
        message_number = static_cast<int>(evalexpr(message_node.message_expression));
    }

    // constant message number or a string-based message
    else
    {
        message_number = message_node.message_number;

        // for messages that are not in the message file, evaluate the message text
        if( message_node.message_expression != -1 )
        {
            unformatted_message_text = EvalAlphaExpr(message_node.message_expression);
            user_message_manager.UpdateUnnumberedMessageText(message_number, *unformatted_message_text);
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

        if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
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
                EvaluateVariantExpression(argument_data_type, argument_list_node.elements[i]),
                argument_list_node.elements[i]
            });
    }

    // format the message text
    MessageArgumentsMessageParameterEvaluator message_parameter_evaluator(this, arguments, function_code);

    if( unformatted_message_text.has_value() )
    {
        return user_message_evaluator.GetFormattedMessage(message_parameter_evaluator, *unformatted_message_text);
    }

    else
    {
        return user_message_evaluator.GetFormattedMessage(message_parameter_evaluator, message_number);
    }
}


double CIntDriver::exerrmsg(int iExpr)
{
    if( !m_pEngineSettings->IsErrmsgMessageOn() )
        return 0;

    return DisplayUserMessage(iExpr);
}


double CIntDriver::exdisplay(int iExpr)
{
    if( !m_pEngineSettings->IsDisplayMessageOn() )
        return 0;

    return DisplayUserMessage(iExpr);
}


double CIntDriver::exwritemsg(int iExpr)
{
    ASSERT(m_pEngineDriver->GetWriteFile() != nullptr);

    std::wstring message_text = EvaluateUserMessage(iExpr, FunctionCode::FNWRITE_CODE);
    m_pEngineDriver->GetWriteFile()->WriteLine(std::move(message_text));

    return 1;
}


double CIntDriver::exmaketext(int iExpr)
{
    std::wstring message_text = EvaluateUserMessage(iExpr, FunctionCode::FNMAKETEXT_CODE);
    return AssignAlphaValue(std::move(message_text));
}


double CIntDriver::exlogtext(int iExpr)
{
    if( !Paradata::Logger::IsOpen() )
        return 0;

    int message_number;
    std::wstring message_text = EvaluateUserMessage(iExpr, FunctionCode::FNLOGTEXT_CODE, &message_number);

    m_pParadataDriver->RegisterAndLogEvent(m_pParadataDriver->CreateMessageEvent(FunctionCode::FNLOGTEXT_CODE, message_number, std::move(message_text)));

    return 1;
}


double CIntDriver::exwarning(int iExpr)
{
    if( Issamod == ModuleType::Entry )
    {
        // if advancing, don't display the message
        bool is_advancing = ( m_pCsDriver->GetSourceOfNodeAdvance() >= 0 ) ||
                            ( m_pCsDriver->GetNumOfPendingAdvances() > 0 );

        if( is_advancing )
        {
            const auto& message_node = GetNode<Nodes::Message>(iExpr);

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
                    int default_button_number = evalexpr<int>(extended_message_node.select_default_button_expression);

                    if( default_button_number >= 1 && default_button_number <= select_movements_list_node.number_elements )
                    {
                        int select_expression = select_movements_list_node.elements[default_button_number - 1];

                        if( select_expression != -1 )
                            evalexpr(select_expression);

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

    return DisplayUserMessage(iExpr);
}


double CIntDriver::DisplayUserMessage(int message_node_index)
{
    const auto& message_node = GetNode<Nodes::Message>(message_node_index);
    const Nodes::ExtendedMessage* extended_message_node = nullptr;

    MessageManager& user_message_manager = m_pEngineDriver->GetUserMessageManager();

    // evaluate the message
    int message_number;
    const std::wstring message_text = ConvertV0Escapes(EvaluateUserMessage(message_node_index, message_node.function_code, &message_number));

    // process the extended options
    std::unique_ptr<std::tuple<std::vector<std::wstring>, int>> button_text_and_default_button_number;
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
                button_text_and_default_button_number = std::make_unique<std::tuple<std::vector<std::wstring>, int>>();
                std::vector<std::wstring>& message_buttons = std::get<0>(*button_text_and_default_button_number);
                int& default_button_number = std::get<1>(*button_text_and_default_button_number);

                // add the button text
                for( int i = 0; i < select_button_texts_list_node.number_elements; ++i )
                    message_buttons.emplace_back(EvalAlphaExpr(select_button_texts_list_node.elements[i]));

                // check if there is a valid default button number
                if( extended_message_node->select_default_button_expression != -1 )
                    default_button_number = evalexpr<int>(extended_message_node->select_default_button_expression);

                if( default_button_number < 0 || default_button_number > static_cast<int>(message_buttons.size()) )
                    default_button_number = 0;

                // the button number should be zero-based (or -1 if none specified)
                --default_button_number;
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
            message_event = m_pParadataDriver->CreateMessageEvent(message_node.function_code, message_number, message_text);

            if( button_text_and_default_button_number != nullptr )
                operator_selection_event = std::make_unique<Paradata::OperatorSelectionEvent>(Paradata::OperatorSelectionEvent::Source::Errmsg);
        }

        // display the message
        const int selected_button_number = m_pEngineDriver->DisplayMessage(MessageType::User, message_number_for_display, message_text, button_text_and_default_button_number.get());

        ASSERT(selected_button_number > 0);

        // log the paradata events
        if( message_event != nullptr )
        {
            if( Issamod == ModuleType::Entry )
                message_event->SetPostDisplayReturnValue(selected_button_number);

            m_pParadataDriver->RegisterAndLogEvent(std::move(message_event));
        }

        if( operator_selection_event != nullptr )
        {
            std::optional<std::wstring> button_text = ( selected_button_number == 0 ) ? std::nullopt :
                                                                                        std::make_optional(std::get<0>(*button_text_and_default_button_number)[selected_button_number - 1]);

            operator_selection_event->SetPostSelectionValues(selected_button_number, std::move(button_text), true);
            m_pParadataDriver->RegisterAndLogEvent(std::move(operator_selection_event));
        }

        // if not in a select statement, we are done, with the return value meaning success
        if( button_text_and_default_button_number == nullptr )
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
            evalexpr(select_expression);

        // return the index of the button selected
        return selected_button_number;
    }
}


double CIntDriver::exvariablevalue(int iExpr)
{
    const auto& variable_value_node = GetNode<Nodes::VariableValue>(iExpr);
    return evalexpr(variable_value_node.expression);
}
