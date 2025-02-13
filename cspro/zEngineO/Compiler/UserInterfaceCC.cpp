#include "stdafx.h"
#include "IncludesCC.h"
#include "Nodes/UserInterface.h"


int LogicCompiler::CompileViewerOptions(const bool allow_left_parenthesis_starting_token)
{
    Nodes::ViewerOptions temp_viewer_options_node;
    InitializeNode(temp_viewer_options_node, -1);

    OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);

    optional_named_arguments_compiler.AddArgument(JK::width, temp_viewer_options_node.width_expression, DataType::Numeric);
    optional_named_arguments_compiler.AddArgument(JK::height, temp_viewer_options_node.height_expression, DataType::Numeric);
    optional_named_arguments_compiler.AddArgument(JK::title, temp_viewer_options_node.title_expression, DataType::String);
    optional_named_arguments_compiler.AddArgument(JK::showCloseButton, temp_viewer_options_node.show_close_button_expression, DataType::Numeric);

    if( optional_named_arguments_compiler.Compile(allow_left_parenthesis_starting_token) == 0 )
        return -1;

    // if width or height or specified, both must be specified
    if( ( temp_viewer_options_node.width_expression != -1 ) != ( temp_viewer_options_node.height_expression != -1 ) )
        IssueError(MGF::width_height_both_must_be_specified_2033);

    // viewer options are rarely used, so only create the real compilation node if they are set
    auto& real_viewer_options_node = CreateNode<Nodes::ViewerOptions>();
    memcpy(&real_viewer_options_node, &temp_viewer_options_node, sizeof(Nodes::ViewerOptions));

    return GetProgramIndex(real_viewer_options_node);
}


int LogicCompiler::CompileUserInterfaceFunctions()
{
    FunctionCode function_code = CurrentToken.function_details->code;
    int program_index;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);


    // setfont
    // --------------------------------------------------------------------------
    if( function_code == FunctionCode::FNSETFONT_CODE )
    {
        auto& setfont_node = CreateNode<Nodes::SetFont>(function_code);
        setfont_node.font_name_expression = -1;
        setfont_node.font_size_expression = -1;

        setfont_node.font_attributes = static_cast<int>(NextKeywordOrError({ _T("ERRMSG"), _T("VALUESETS"), _T("USERBAR"),
                                                                             _T("NOTES"),  _T("ALL"),       _T("NUMBERPAD") }));

        NextToken();
        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        // allow the font to reset to the default
        if( NextKeyword({ _T("DEFAULT") }) == 1 )
        {
            setfont_node.font_attributes |= Nodes::SetFont::DefaultMask;
            NextToken();
        }

        else
        {
            NextToken();
            setfont_node.font_name_expression = CompileStringExpression();

            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();
            setfont_node.font_size_expression = exprlog();

            // there can be up to two more parameters: bold, italics
            for( int i = 0; i < 2; ++i )
            {
                if( Tkn == TOKRPAREN )
                    break;

                IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                size_t parameter_type = NextKeywordOrError( {_T("BOLD"), _T("ITALICS") });

                setfont_node.font_attributes |= ( parameter_type == 1 ) ? Nodes::SetFont::BoldMask :
                                                                          Nodes::SetFont::ItalicsMask;

                NextToken();
            }
        }

        program_index = GetProgramIndex(setfont_node);
    }


    // unknown function
    // --------------------------------------------------------------------------
    else
    {
        throw ProgrammingErrorException();
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return program_index;
}
