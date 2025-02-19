#include "stdafx.h"
#include "IncludesCC.h"
#include "EngineDictionary.h"
#include "Nodes/Dictionaries.h"
#include <zLogicO/BaseCompilerSettings.h>
#include <engine/Dict.h>


int LogicCompiler::CompileSyncFunctions()
{
    FunctionCode function_code = CurrentToken.function_details->code;

    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);


    // shared routines
    auto read_dictionary_symbol = [&](bool must_be_external) -> int
    {
        NextToken();

        if( Tkn != TOKDICT && Tkn != TOKDICT_PRE80 )
            IssueError(MGF::dictionary_expected_544);

        Symbol& symbol = NPT_Ref(Tokstindex);

        int dictionary_use_flags = ( must_be_external ? VerifyDictionaryFlag::External : 0 ) |
                                   VerifyDictionaryFlag::NeedsIndex |
                                   VerifyDictionaryFlag::Writeable;

        if( symbol.IsA(SymbolType::Dictionary) )
        {
            EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
            VerifyEngineDataRepository(&engine_dictionary, dictionary_use_flags);
            engine_dictionary.GetEngineDataRepository().SetUsesSync();
        }

        else
        {
            DICT* pDicT = assert_cast<DICT*>(&symbol);
            VerifyDictionary(pDicT, dictionary_use_flags);
            pDicT->SetUsesSync();
        }

        return symbol.GetSymbolIndex();
    };

    auto read_direction = [&](const bool allow_both)
    {
        constexpr const char* directions[] = { "put", "get", "both" };
        const cs::span<const char* const> valid_directions(directions, directions + _countof(directions) + ( allow_both ? 0 : -1 ));

        int direction = static_cast<int>(NextKeyword(valid_directions));
        NextToken();

        if( direction == 0 )
        {
            // NextKeywordOrError will display the valid options
            auto issue_error = [&]() { NextKeywordOrError(valid_directions); };

            if( IsCurrentTokenString() )
            {
                std::optional<int> direction_override;

                direction = -1 * CompileStringExpressionWithStringLiteralCheck(
                    [&](const std::string& text)
                    {
                        const auto& lookup = std::find_if(valid_directions.cbegin(), valid_directions.cend(),
                                                          [&](const char* const direction_text) { return SO::EqualsNoCase(direction_text, text); });

                        if( lookup == valid_directions.cend() )
                            issue_error();

                        direction_override = 1 + std::distance(valid_directions.cbegin(), lookup);
                    });

                if( direction_override.has_value() )
                    direction = *direction_override;
            }

            else
            {
                issue_error();
            }
        }

        return direction;
    };

    auto finalize_compilation = [&](const cs::span<const int> arguments)
    {
        IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

        NextToken();

        return CreateVariableArgumentsNode(function_code, arguments);
    };


    // --------------------------------------------------------------------------
    // syncconnect
    // --------------------------------------------------------------------------
    if( function_code == FunctionCode::FNSYNC_CONNECT_CODE )
    {
        constexpr const char* service_types[] = { "CSWeb", "Bluetooth", "Dropbox", "FTP", "LocalDropbox", "LocalFiles" };
        size_t connection_type = NextKeyword(service_types);
        int host_or_server_device_name = -1;
        int username = -1;
        int password = -1;

        // allow the sync service to be specified using a sync connection string
        if( connection_type == 0 )
        {
            bool sync_connection_string_specified;

            // suppress error reporting while getting the next token so that invalid
            // keywords like "Dropbox2" can be processed properly
            try
            {
                Logic::BaseCompilerSettings compiler_settings_modifier = ModifyCompilerSettings();
                compiler_settings_modifier.SuppressErrorReporting();

                NextToken();
                sync_connection_string_specified = IsCurrentTokenString();
            }

            catch( const Logic::ParserError& )
            {
                sync_connection_string_specified = false;
            }

            if( !sync_connection_string_specified )
                IssueError(94001, SO::CreateSingleString(tcb::make_span(service_types)).c_str());

            host_or_server_device_name = CompileStringExpression();
        }

        else
        {
            NextToken();

            switch( connection_type )
            {
                // CSWeb/FTP - require host, optional username and password
                case 1:
                case 4:
                {
                    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                    NextToken();
                    host_or_server_device_name = CompileStringExpression();

                    if( Tkn == TOKCOMMA )
                    {
                        NextToken();
                        username = CompileStringExpression();

                        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                        NextToken();
                        password = CompileStringExpression();
                    }

                    break;
                }

                // Bluetooth - optional serverDeviceName
                case 2:
                {
                    if( Tkn == TOKCOMMA )
                    {
                        NextToken();
                        host_or_server_device_name = CompileStringExpression();
                    }

                    break;
                }

                // Dropbox - no additional parameters
                case 3:
                case 5:
                    break;

                // LocalFiles - directory path
                case 6:
                {
                    IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

                    NextToken();
                    host_or_server_device_name = CompileStringExpression();

                    break;
                }
            }
        }

        return finalize_compilation({ static_cast<int>(connection_type), host_or_server_device_name, username, password });
    }


    // --------------------------------------------------------------------------
    // syncdata
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSYNC_DATA_CODE )
    {
        int direction = read_direction(true);

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        int dictionary_symbol_index = read_dictionary_symbol(true);

        NextToken();

        int universe = -1;

        if( Tkn == TOKCOMMA ) // they are specifying a universe
        {
            NextToken();
            universe = CompileStringExpression();
        }

        return finalize_compilation({ direction, dictionary_symbol_index, universe });
    }


    // --------------------------------------------------------------------------
    // syncfile
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSYNC_FILE_CODE )
    {
        int direction = read_direction(false);

        IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

        NextToken();
        int from = CompileStringExpression();

        int to = -1;

        if( Tkn != TOKRPAREN )
        {
            IssueErrorOnTokenMismatch(TOKCOMMA, MGF::function_call_comma_expected_528);

            NextToken();
            to = CompileStringExpression();
        }

        return finalize_compilation({ direction, from, to });
    }


    // --------------------------------------------------------------------------
    // syncserver
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSYNC_SERVER_CODE )
    {
        constexpr int BluetoothCode = 2;

        NextKeywordOrError({ "Bluetooth" });

        NextToken();

        int file_root = -1;

        if( Tkn == TOKCOMMA )
        {
            NextToken();
            file_root = CompileStringExpression();
        }

        return finalize_compilation({ BluetoothCode, file_root });
    }


    // --------------------------------------------------------------------------
    // syncmessage
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSYNC_MESSAGE_CODE )
    {
        constexpr int MessageType = -1; // for now this will be ignored

        NextToken();

        const int key_expression = CompileStringExpression();
        int value_expression;

        if( Tkn == TOKCOMMA )
        {
            NextToken();
            value_expression = CompileStringExpression();
        }

        else
        {
             value_expression = -1;
        }

        return finalize_compilation({ MessageType, key_expression, value_expression });
    }


    // --------------------------------------------------------------------------
    // syncparadata
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSYNC_PARADATA_CODE )
    {
        int direction = read_direction(true);

        return finalize_compilation({ direction });
    }


    // --------------------------------------------------------------------------
    // synctime
    // --------------------------------------------------------------------------
    else if( function_code == FunctionCode::FNSYNC_TIME_CODE )
    {
        int dictionary_symbol_index = read_dictionary_symbol(false);

        NextToken();

        int device_identifier_expression = -1;

        if( Tkn == TOKCOMMA )
        {
            NextToken();
            device_identifier_expression = CompileStringExpression();
        }

        int case_uuid_expression = -1;

        if( Tkn == TOKCOMMA )
        {
            NextToken();
            case_uuid_expression = CompileStringExpression();
        }

        return finalize_compilation({ dictionary_symbol_index, device_identifier_expression, case_uuid_expression });
    }


    // --------------------------------------------------------------------------
    // unhandled function code
    // --------------------------------------------------------------------------
    else
    {
        return ReturnProgrammingError(-1);
    }
}
