#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zEngineO/Userbar.h>
#include <zEngineO/UserFunction.h>
#include <zEngineO/Versioning.h>
#include <zEngineF/EngineUI.h>


namespace
{
    constexpr std::wstring_view ControlActionNames[] =
    {
        _T("NextField"),
        _T("PreviousField"),
        _T("AdvanceToEnd"),
        _T("EditNote"),
        _T("ChangeLanguage"),
        _T("PartialSave"),
        _T("FieldHelp"),
        _T("InsertLevelOcc"),
        _T("AddLevelOcc"),
        _T("DeleteLevelOcc"),
        _T("InsertGroupOcc"),
        _T("InsertGroupOccAfter"),
        _T("DeleteGroupOcc"),
        _T("SortGroupOcc"),
        _T("PreviousScreen"),
        _T("NextScreen"),
        _T("EndGroupOcc"),
        _T("EndGroup"),
        _T("EndLevelOcc"),
        _T("EndLevel"),
        _T("FullScreen"),
        _T("ToggleResponses"),
        _T("ToggleAllResponses")
    };
}


double CIntDriver::exuserbar(int iExpr)
{
    // userbar is only supported in entry
    if( Issamod != ModuleType::Entry )
        return 0;

    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    const Userbar::Command userbar_command = static_cast<Userbar::Command>(va_node.arguments[0]);

    std::vector<int> arguments;
    bool Iteration_7_6_000_1_assert_check = true;

    if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_7_6_000_1) )
    {
        arguments = GetListNodeContents(va_node.arguments[1]);
    }

    else
    {
        // this is risky because a certain compilation where the userbar is the last thing in the
        // buffer could lead to this crashing, but this is only temporary until <= 7.5 code is removed
        arguments.resize(4);
        memcpy(arguments.data(), &va_node.arguments[1], sizeof(int) * arguments.size());
        Iteration_7_6_000_1_assert_check = false;
    }

    try
    {
        // create the userbar if this is the first call
        const bool userbar_already_existed = m_pEngineDriver->HasUserbar();

        if( !userbar_already_existed )
        {
            std::unique_ptr<Userbar> userbar;
            SendEngineUIMessage(EngineUI::Type::CreateUserbar, userbar);
            m_pEngineDriver->SetUserbar(std::move(userbar));

            if( !m_pEngineDriver->HasUserbar() )
                throw CSProException("Could not create a userbar");
        }

        Userbar& userbar = m_pEngineDriver->GetUserbar();


        // by default if the first call to the userbar is one of the add functions, then we will show the bar
        // at the onset; if the user wants to create the bar unseen, he should call userbar(hide) before any other statement
        if( !userbar_already_existed && ( userbar_command == Userbar::Command::AddButton  || userbar_command == Userbar::Command::AddField ||
                                          userbar_command == Userbar::Command::AddSpacing || userbar_command == Userbar::Command::AddText ) )
        {
            userbar.Show();
        }


        // process each command...


        // a routine to parse command names
        auto get_control_action = [&](const std::wstring& name)
        {
            for( size_t i = 0; i < _countof(ControlActionNames); ++i )
            {
                if( SO::EqualsNoCase(name, ControlActionNames[i]) )
                    return static_cast<Userbar::ControlAction>(i);
            }

            throw CSProException(_T("The command '%s' is not supported on this system"), name.c_str());
        };


        // show
        if( userbar_command == Userbar::Command::Show )
        {
            userbar.Show();
            return 1;
        }


        // hide
        else if( userbar_command == Userbar::Command::Hide )
        {
            userbar.Hide();
            return 1;
        }


        // clear
        else if( userbar_command == Userbar::Command::Clear )
        {
            userbar.Clear();
            return 1;
        }


        // set color
        else if( userbar_command == Userbar::Command::SetColor )
        {
            ASSERT(!Iteration_7_6_000_1_assert_check || arguments.size() == 4);

            auto evaluate_color = [&](size_t index) { return std::min(255, evalexpr<int>(arguments[index])); };
            const int red = evaluate_color(1);
            const int green = evaluate_color(2);
            const int blue = evaluate_color(3);

            const COLORREF color = RGB(red, green, blue);
            std::optional<int> id;

            // evaluate the resource ID if the user isn't setting the color of the entire bar
            if( arguments[0] != -1 )
                id = evalexpr<int>(arguments[0]);

            return userbar.SetColor(color, id);
        }


        // remove
        else if( userbar_command == Userbar::Command::Remove )
        {
            ASSERT(!Iteration_7_6_000_1_assert_check || arguments.size() == 1);
            return userbar.Remove(evalexpr<int>(arguments.front()));
        }


        // add button / add field
        else if( userbar_command == Userbar::Command::AddButton || userbar_command == Userbar::Command::AddField )
        {
            ASSERT(!Iteration_7_6_000_1_assert_check || arguments.size() == 2 || arguments.size() == 3);
            std::wstring text = EvalAlphaExpr(arguments[0]);
            std::optional<Userbar::Action> action;

            // adding a control (next, previous, note, etc.)
            if( arguments[1] == -2 )
            {
                action = get_control_action(EvalAlphaExpr(arguments[2]));
            }

            else if( arguments[1] > 0 )
            {
                action = EvaluateArgumentsForCallbackUserFunction(arguments[1], FunctionCode::FNUSERBAR_CODE);
            }

            if( userbar_command == Userbar::Command::AddButton )
            {
                return userbar.AddButton(std::move(text), std::move(action));
            }

            else
            {
                return userbar.AddField(std::move(text), std::move(action));
            }
        }


        // add text
        else if( userbar_command == Userbar::Command::AddText )
        {
            ASSERT(!Iteration_7_6_000_1_assert_check || arguments.size() == 1);
            return userbar.AddText(EvalAlphaExpr(arguments.front()));
        }


        // add spacing
        else if( userbar_command == Userbar::Command::AddSpacing )
        {
            ASSERT(!Iteration_7_6_000_1_assert_check || arguments.size() == 1);
            return userbar.AddSpacing(evalexpr<int>(arguments.front()));
        }


        // modify
        else if( userbar_command == Userbar::Command::Modify )
        {
            ASSERT(!Iteration_7_6_000_1_assert_check || arguments.size() >= 2 && arguments.size() <= 4);

            const int id = evalexpr<int>(arguments[0]);
            std::optional<std::wstring> text;
            std::optional<Userbar::Action> action;
            std::optional<int> spacing;

            if( arguments[1] == -2 ) // then the user is modifying spacing
            {
                spacing = evalexpr<int>(arguments[2]);
            }

            else
            {
                if( arguments[1] != -1 ) // then the user is modifying the text
                    text = EvalAlphaExpr(arguments[1]);

                if( arguments[2] == -2 ) // then the user is modifying the function with a control
                {
                    action = get_control_action(EvalAlphaExpr(arguments[3]));
                }

                else if( arguments[2] != -1 ) // then the user is modifying the function
                {
                    action = EvaluateArgumentsForCallbackUserFunction(arguments[2], FunctionCode::FNUSERBAR_CODE);
                }
            }

            return userbar.Modify(id, std::move(text), std::move(action), std::move(spacing));
        }


        // get
        else if( userbar_command == Userbar::Command::GetField )
        {
            ASSERT(!Iteration_7_6_000_1_assert_check || arguments.size() == 1 || arguments.size() == 2);

            // the user is querying the last called resource ID
            if( arguments[0] == -1 )
                return userbar.GetLastActivatedItem().value_or(0);

            // otherwise get the field text
            std::optional<std::wstring> field_text = userbar.GetFieldText(evalexpr<int>(arguments[0]));

            if( !field_text.has_value() )
                return 0;

            if( Versioning::MeetsCompiledLogicVersion(Serializer::Iteration_7_6_000_1) )
            {
                const auto& symbol_value_node = GetNode<Nodes::SymbolValue>(arguments[1]);
                AssignValueToSymbol(symbol_value_node, std::move(*field_text));
            }

            else
            {
                VART* pVarT = VPT(arguments[1]);

                if( pVarT->GetLogicStringPtr() ) // 20140326 a variable length string
                {
                    *( pVarT->GetLogicStringPtr() ) = WS2CS(*field_text);
                }

                else
                {
                    TCHAR* pBuffer = (TCHAR*)svaraddr(pVarT->GetVarX());
                    int bufferLength = pVarT->GetLength();

                    int charsToCopy = std::min(bufferLength, static_cast<int>(field_text->length()));

                    _tmemcpy(pBuffer, field_text->c_str(), charsToCopy);

                    if( charsToCopy < bufferLength )
                        _tmemset(pBuffer + charsToCopy, BLANK, bufferLength - charsToCopy);
                }
            }

            return 1;
        }
    }

    catch( const Userbar::FeatureNotImplemented& )
    {
        // in the portable environment, some features aren't implemented but will not
        // result in an error message
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 50106, exception.GetErrorMessage().c_str());
    }

    return 0;
}
