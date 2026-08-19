#include "stdafx.h"
#include "IncludesRT.h"
#include "Userbar.h"


namespace
{
    constexpr std::string_view ControlActionNames_sv[] =
    {
        "NextField",
        "PreviousField",
        "AdvanceToEnd",
        "EditNote",
        "ChangeLanguage",
        "PartialSave",
        "FieldHelp",
        "InsertLevelOcc",
        "AddLevelOcc",
        "DeleteLevelOcc",
        "InsertGroupOcc",
        "InsertGroupOccAfter",
        "DeleteGroupOcc",
        "SortGroupOcc",
        "PreviousScreen",
        "NextScreen",
        "EndGroupOcc",
        "EndGroup",
        "EndLevelOcc",
        "EndLevel",
        "FullScreen",
        "ToggleResponses",
        "ToggleAllResponses"
    };
}


Engine::Value LogicInterpreter::ex_userbar(const int program_index)
{
    // userbar is only supported in entry
    if( GetEngineAppType() != EngineAppType::Entry )
        return Engine::Value::Bool(false);

    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const Userbar::Command userbar_command = static_cast<Userbar::Command>(va_node.arguments[0]);
    const std::vector<int> arguments = GetListNodeContents(va_node.arguments[1]);

    try
    {
        // create the userbar if this is the first call
        const bool userbar_just_created = ( m_userbar == nullptr );

        if( userbar_just_created )
        {
            SendEngineUIMessage(EngineUI::Type::CreateUserbar, m_userbar);

            if( m_userbar == nullptr )
                throw CSProException("Could not create a userbar.");
        }

        ASSERT(m_userbar != nullptr);

        // by default if the first call to the userbar is one of the add functions, then we will show the bar
        // at the onset; if the user wants to create the bar unseen, he should call userbar(hide) before any other statement
        if( userbar_just_created )
        {
            switch( userbar_command )
            {
                case Userbar::Command::AddButton:
                case Userbar::Command::AddField:
                case Userbar::Command::AddSpacing:
                case Userbar::Command::AddText:
                    m_userbar->Show();
                    break;
            }
        }


        // process each command...


        // a routine to parse command names
        auto get_control_action = [&](const std::string& command)
        {
            for( size_t i = 0; i < _countof(ControlActionNames_sv); ++i )
            {
                if( SO::EqualsNoCase(command, ControlActionNames_sv[i]) )
                    return static_cast<Userbar::ControlAction>(i);
            }

            throw CSProException("The command '%s' is not supported on this system.", command.c_str());
        };


        // show
        if( userbar_command == Userbar::Command::Show )
        {
            m_userbar->Show();
            return Engine::Value::Bool(true);
        }


        // hide
        else if( userbar_command == Userbar::Command::Hide )
        {
            m_userbar->Hide();
            return Engine::Value::Bool(true);
        }


        // clear
        else if( userbar_command == Userbar::Command::Clear )
        {
            m_userbar->Clear();
            return Engine::Value::Bool(true);
        }


        // set color
        else if( userbar_command == Userbar::Command::SetColor )
        {
            ASSERT(arguments.size() == 4);

            auto evaluate_color = [&](const size_t index) { return std::min(255, Evaluate<int>(arguments[index])); };
            const int red = evaluate_color(1);
            const int green = evaluate_color(2);
            const int blue = evaluate_color(3);

            const COLORREF color = RGB(red, green, blue);
            std::optional<int> id;

            // evaluate the resource ID if the user isn't setting the color of the entire bar
            if( arguments[0] != -1 )
                id = Evaluate<int>(arguments[0]);

            return Engine::Value::Bool(
                m_userbar->SetColor(color, id)
            );
        }


        // remove
        else if( userbar_command == Userbar::Command::Remove )
        {
            ASSERT(arguments.size() == 1);

            return Engine::Value::Bool(
                m_userbar->Remove(Evaluate<int>(arguments.front()))
            );
        }


        // add button / add field
        else if( userbar_command == Userbar::Command::AddButton || userbar_command == Userbar::Command::AddField )
        {
            ASSERT(arguments.size() == 2 || arguments.size() == 3);
            std::optional<Userbar::Action> action;

            // adding a control (next, previous, note, etc.)
            if( arguments[1] == -2 )
            {
                const SharableString command = Evaluate<SharableString>(arguments[2]);
                action = get_control_action(*command);
            }

            else if( arguments[1] > 0 )
            {
                action = EvaluateArgumentsForCallbackUserFunction(arguments[1], FunctionCode::FNUSERBAR_CODE);
            }

            std::string text = Evaluate<std::string>(arguments[0]);

            if( userbar_command == Userbar::Command::AddButton )
            {
                return Engine::Value::Integer(
                    m_userbar->AddButton(std::move(text), std::move(action))
                );
            }

            else
            {
                return Engine::Value::Integer(
                    m_userbar->AddField(std::move(text), std::move(action))
                );
            }
        }


        // add text
        else if( userbar_command == Userbar::Command::AddText )
        {
            ASSERT(arguments.size() == 1);

            return Engine::Value::Integer(
                m_userbar->AddText(Evaluate<std::string>(arguments.front()))
            );
        }


        // add spacing
        else if( userbar_command == Userbar::Command::AddSpacing )
        {
            ASSERT(arguments.size() == 1);

            return Engine::Value::Integer(
                m_userbar->AddSpacing(Evaluate<int>(arguments.front()))
            );
        }


        // modify
        else if( userbar_command == Userbar::Command::Modify )
        {
            ASSERT(arguments.size() >= 2 && arguments.size() <= 4);

            const int id = Evaluate<int>(arguments[0]);
            std::optional<std::string> text;
            std::optional<Userbar::Action> action;
            std::optional<int> spacing;

            if( arguments[1] == -2 ) // then the user is modifying spacing
            {
                spacing = Evaluate<int>(arguments[2]);
            }

            else
            {
                if( arguments[1] != -1 ) // then the user is modifying the text
                    text = Evaluate<std::string>(arguments[1]);

                if( arguments[2] == -2 ) // then the user is modifying the function with a control
                {
                    const SharableString command = Evaluate<SharableString>(arguments[3]);
                    action = get_control_action(*command);
                }

                else if( arguments[2] != -1 ) // then the user is modifying the function
                {
                    action = EvaluateArgumentsForCallbackUserFunction(arguments[2], FunctionCode::FNUSERBAR_CODE);
                }
            }

            return Engine::Value::Bool(
                m_userbar->Modify(id, std::move(text), std::move(action), std::move(spacing))
            );
        }


        // get
        else if( userbar_command == Userbar::Command::GetField )
        {
            ASSERT(arguments.size() == 1 || arguments.size() == 2);

            // the user is querying the last called resource ID
            if( arguments[0] == -1 )
            {
                return Engine::Value::Integer(
                    m_userbar->GetLastActivatedItem().value_or(0)
                );
            }

            // otherwise get the field text
            std::optional<std::string> field_text = m_userbar->GetFieldText(Evaluate<int>(arguments[0]));

            if( !field_text.has_value() )
                return Engine::Value::Bool(false);

            const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(arguments[1]);
            AssignValueToSymbol<SharableString>(symbol_value_node, std::move(*field_text));

            return Engine::Value::Bool(true);
        }
    }

    catch( const Userbar::FeatureNotImplemented& )
    {
        // in the portable environment, some features aren't implemented but will not
        // result in an error message
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::userbar_error_50106, exception.what());
    }

    return Engine::Value::Bool(false);
}
