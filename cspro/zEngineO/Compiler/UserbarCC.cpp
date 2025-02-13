#include "stdafx.h"
#include "IncludesCC.h"
#include "Userbar.h"


int LogicCompiler::CompileUserbarFunction()
{
    Userbar::Command userbar_command;
    std::vector<int> arguments;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    switch( NextKeywordOrError({ _T("hide"), _T("show"), _T("clear"), _T("remove"), _T("modify"), _T("get"), _T("add"), _T("set") }) )
    {
        case 1:
            userbar_command = Userbar::Command::Hide;
            break;

        case 2:
            userbar_command = Userbar::Command::Show;
            break;

        case 3:
            userbar_command = Userbar::Command::Clear;
            break;

        case 4:
            userbar_command = Userbar::Command::Remove;
            break;

        case 5:
            userbar_command = Userbar::Command::Modify;
            break;

        case 6:
            userbar_command = Userbar::Command::GetField;
            break;

        case 7:
        {
            switch( NextKeywordOrError({ _T("text"), _T("button"), _T("field"), _T("spacing") }) )
            {
                case 1:
                    userbar_command = Userbar::Command::AddText;
                    break;

                case 2:
                    userbar_command = Userbar::Command::AddButton;
                    break;

                case 3:
                    userbar_command = Userbar::Command::AddField;
                    break;

                case 4:
                    userbar_command = Userbar::Command::AddSpacing;
                    break;

                default:
                    throw ProgrammingErrorException();
            }

            break;
        }

        case 8:
        {
            NextKeywordOrError({ _T("color") });

            userbar_command = Userbar::Command::SetColor;
            break;
        }

        default:
            throw ProgrammingErrorException();
    }

    NextToken();


    // --------------------------------------------------------------------------
    // set color takes three or four arguments ([resource id,] red, green, blue)
    // --------------------------------------------------------------------------
    if( userbar_command == Userbar::Command::SetColor )
    {
        arguments.emplace_back(-1); // the default case (-1) means that the color of the bar is getting set

        for( int i = 0; i < 3; ++i )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();
            arguments.emplace_back(exprlog());
        }

        if( Tkn == TOKCOMMA ) // then there is a fourth parameter, and we will shift all the already-read parameters
        {
            arguments.erase(arguments.begin());

            NextToken();
            arguments.emplace_back(exprlog());
        }
    }


    // --------------------------------------------------------------------------
    // remove takes one argument (resource id)
    // --------------------------------------------------------------------------
    else if( userbar_command == Userbar::Command::Remove )
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();
        arguments.emplace_back(exprlog());
    }


    // --------------------------------------------------------------------------
    // get takes two arguments (resource id, string destination variable); or
    // get will also take no arguments, in which case it returns the resource ID
    // of the last called button/field
    // --------------------------------------------------------------------------
    else if( userbar_command == Userbar::Command::GetField )
    {
        if( Tkn == TOKRPAREN )
        {
            arguments.emplace_back(-1); // -1 will signify that we're looking for the last called resource ID
        }

        else
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();
            arguments.emplace_back(exprlog());

            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();
            arguments.emplace_back(CompileDestinationVariable(DataType::String));
        }
    }


    // --------------------------------------------------------------------------
    // add text takes one argument (string)
    // --------------------------------------------------------------------------
    else if( userbar_command == Userbar::Command::AddText )
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();
        arguments.emplace_back(CompileStringExpression());
    }


    // --------------------------------------------------------------------------
    // add spacing takes one argument (number of pixels)
    // --------------------------------------------------------------------------
    else if( userbar_command == Userbar::Command::AddSpacing )
    {
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();
        arguments.emplace_back(exprlog());
    }


    // --------------------------------------------------------------------------
    // add button/field takes one or two arguments (string, optional function name)
    // modify takes two or three arguments (resource id, either/or...string / function name)
    // modify can also take just one argument for modifying spacing (number of pixels)
    // --------------------------------------------------------------------------
    else if( userbar_command == Userbar::Command::AddButton ||
             userbar_command == Userbar::Command::AddField ||
             userbar_command == Userbar::Command::Modify )
    {
        bool modify_mode = ( userbar_command == Userbar::Command::Modify );
        arguments.resize(modify_mode ? 3 : 2);

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();

        if( modify_mode ) // read the resource ID
        {
            arguments[0] = exprlog();

            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();
        }

        if( !modify_mode || IsCurrentTokenString() )
        {
            arguments[modify_mode ? 1 : 0] = CompileStringExpression();

            if( Tkn == TOKCOMMA )
                NextToken();
        }

        else
        {
            arguments[1] = -1; // in modify mode and the user is only modifying the function or spacing
        }

        if( Tkn != TOKUSERFUNCTION && Tkn != TOKDO )
        {
            if( modify_mode && arguments[1] != -1 ) // in modify mode and the user is only modifying the text
            {
                arguments[2] = -1;
            }

            else if( !modify_mode ) // then the user is not specifying a function (using a null function)
            {
                arguments[1] = 0;
            }

            else // we will assume that the user is trying to modify spacing, so we expect a number
            {
                arguments[1] = -2; // -2 will indicate spacing
                arguments[2] = exprlog();
            }
        }

        else if( Tkn == TOKUSERFUNCTION ) // see if the function has arguments (in which case we'll have to properly compile the function)
        {
            arguments[modify_mode ? 2 : 1] = CompileUserFunctionCall(true);
        }

        else
        {
            ASSERT(Tkn == TOKDO);

            arguments.emplace_back();
            arguments[modify_mode ? 2 : 1] = -2; // -2 will indicate that this is a "control"

            NextToken();
            IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

            NextToken();
            arguments[modify_mode ? 3 : 2] = CompileStringExpression();

            IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

            NextToken();
        }
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return CreateVariableArgumentsNode(FunctionCode::FNUSERBAR_CODE, { static_cast<int>(userbar_command), CreateListNode(arguments) });
}
