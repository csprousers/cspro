#include "stdafx.h"
#include "IncludesCC.h"
#include "Nodes/GPS.h"


int LogicCompiler::CompileGpsFunction()
{
    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    size_t command_value = NextKeyword({ _T("open"),            _T("close"),        _T("status"),       _T("read"),         _T("readlast"),
                                         _T("latitude"),        _T("longitude"),    _T("altitude"),     _T("satellites"),
                                         _T("accuracy"),        _T("readtime"),     _T("distance"),
                                         _T("readinteractive"), _T("select") });

    if( command_value == 0 )
        IssueError(MGF::GPS_invalid_command_8251);

    Nodes::GPS::Command command = static_cast<Nodes::GPS::Command>(command_value);

    int number_arguments = ( command == Nodes::GPS::Command::Open )            ? 2 :
                           ( command == Nodes::GPS::Command::Read )            ? 3 :
                           ( command == Nodes::GPS::Command::Distance )        ? 4 :
                           ( command == Nodes::GPS::Command::ReadInteractive ) ? 4 :
                           ( command == Nodes::GPS::Command::Select )          ? 3 :
                                                                                 0;

    auto& gps_node = CreateVariableSizeNode<Nodes::GPS>(FunctionCode::FNGPS_CODE, number_arguments);
    gps_node.command = command;

    NextToken();


    // gps(open[, ...])
    if( command == Nodes::GPS::Command::Open )
    {
        // for Windows desktop, the first parameter is COM port (default: 3), next is baud (default: 4800)
        int& com_port = gps_node.options[0] = -1;
        int& baud_rate = gps_node.options[1] = -1;

        if( Tkn == TOKCOMMA )
        {
            NextToken();
            com_port = exprlog();

            if( Tkn == TOKCOMMA )
            {
                NextToken();
                baud_rate = exprlog();
            }
        }
    }


    // gps(read[, ...])
    else if( command == Nodes::GPS::Command::Read )
    {
        int& seconds_to_wait = gps_node.options[0] = 0;
        int& desired_accuracy = gps_node.options[1] = 0;
        int& wait_string = gps_node.options[2] = 0;

        if( Tkn == TOKCOMMA ) // now we expect to read in the wait time
        {
            NextToken();
            seconds_to_wait = exprlog();

            if( Tkn == TOKCOMMA )
            {
                NextToken();

                bool read_wait_string = true;

                if( !IsCurrentTokenString() ) // now we expect to read in the desired accuracy
                {
                    desired_accuracy = exprlog();

                    if( Tkn == TOKCOMMA )
                    {
                        NextToken();
                    }

                    else
                    {
                        read_wait_string = false;
                    }
                }

                if( read_wait_string ) // now we expect to read in a wait string
                    wait_string = CompileStringExpression();
            }
        }
    }


    // gps(distance, ...)
    else if( command == Nodes::GPS::Command::Distance )
    {
        // read two sets of coordinates
        for( size_t i = 0; i < 4; ++i )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, 528);

            NextToken();
            gps_node.options[i] = exprlog();
        }
    }


    // gps(readInteractive[, message], baseMap := None[, readDuration :=, message := ])
    // gps(readInteractive[, message][, baseMap := Normal/Satellite/Hybrid/Terrain/filename, message := ])
    // gps(select[, message][, baseMap := Normal/Satellite/Hybrid/Terrain/None/filename, message := ])
    else if( bool read_interactive_mode = ( command == Nodes::GPS::Command::ReadInteractive );
             read_interactive_mode || command == Nodes::GPS::Command::Select )
    {
        int& base_map_type = gps_node.options[0] = -1;
        int& base_map_filename_expression = gps_node.options[1] = -1;
        int& message_expression = gps_node.options[2] = -1;
        int* read_duration_expression = read_interactive_mode ? &gps_node.options[3] : nullptr;

        if( read_duration_expression != nullptr )
            *read_duration_expression = -1;

        if( Tkn == TOKCOMMA )
        {
            // the message can be specified directly...
            if( !IsNextTokenNamedArgument() )
            {
                NextToken();
                message_expression = CompileStringExpression();
            }

            // ...but other arguments are specified using named arguments
            OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this);

            optional_named_arguments_compiler.AddArgument(_T("baseMap"), base_map_type,
                [&]()
                {
                    base_map_type = static_cast<int>(NextKeyword(GetBaseMapStrings()));
                    NextToken();

                    // if a keyword was not specified, try to read a filename
                    if( base_map_type == 0 )
                        base_map_filename_expression = CompileStringExpression();

                    return base_map_type;
                });

            if( message_expression == -1 )
                optional_named_arguments_compiler.AddArgument(_T("message"), message_expression, DataType::String);

            if( read_duration_expression != nullptr )
                optional_named_arguments_compiler.AddArgument(_T("readDuration"), *read_duration_expression, DataType::Numeric);

            optional_named_arguments_compiler.Compile();

            // the read duration can only be specified when using no map
            if( base_map_type != static_cast<int>(BaseMap::None) && read_duration_expression != nullptr && *read_duration_expression != -1 )
                IssueError(MGF::GPS_readDuration_argument_invalid_8252);
        }
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return GetProgramIndex(gps_node);
}
