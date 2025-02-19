#include "stdafx.h"
#include "IncludesCC.h"
#include <zLogicO/ActionInvoker.h>
#include <zLogicO/GeneralizedFunction.h>
#include <zAction/NameProcessors.h>


int LogicCompiler::CompileActionInvokerFunctions()
{
    // CS[.namespace].actionName(named_argument1 := , named_argument2 := , ...)
    // CS[.namespace].actionName(json_arguments_text)
    ASSERT(CurrentToken.function_details->code == FunctionCode::CSFN_ACTIONINVOKER_CODE);
    ASSERT(CurrentToken.function_details->compilation_type == Logic::FunctionCompilationType::CS);

    const ActionInvoker::Action action = static_cast<ActionInvoker::Action>(CurrentToken.function_details->number_arguments);
    std::vector<int> argument_expressions = { static_cast<int>(action) };

    std::optional<std::vector<std::string>> argument_names_provided;

    // validate the action name, which should come in preceeded by CS.
    const size_t dot_pos = CurrentToken.text.find('.');
    ASSERT(SO::StartsWithNoCase(SO::TrimRight(CurrentToken.text.substr(0, dot_pos)), "CS"));

    ActionInvoker::Action evaluated_action = ActionInvoker::GetActionFromText(std::string_view(CurrentToken.text).substr(dot_pos + 1), *this);
    ASSERT(action == evaluated_action);

    // get the details about the action
    const GF::Function* function_definition_for_evaluated_action;

    auto get_function_definition_for_evaluated_action = [&]()
    {
        function_definition_for_evaluated_action = ActionInvoker::GetFunctionDefinition(evaluated_action);

        if( function_definition_for_evaluated_action == nullptr )
            IssueError(ReturnProgrammingError(MGF::statement_invalid_1));
    };

    get_function_definition_for_evaluated_action();


    // compile any arguments...
    NextToken();
    IssueErrorOnTokenMismatch(TOKLPAREN, MGF::left_parenthesis_expected_in_function_call_14);

    // ...using named arguments
    if( IsNextTokenNamedArgument() )
    {
        const size_t number_non_deprecated_arguments = function_definition_for_evaluated_action->parameters.size();
        const size_t number_arguments = number_non_deprecated_arguments + function_definition_for_evaluated_action->deprecated_parameters.size();

        OptionalNamedArgumentsCompiler optional_named_arguments_compiler(*this, true);
        std::vector<int> named_argument_expressions(number_arguments, -1);

        for( size_t i = 0; i < function_definition_for_evaluated_action->parameters.size(); ++i )
        {
            ASSERT(!function_definition_for_evaluated_action->parameters[i].variable.types.empty());
            optional_named_arguments_compiler.AddArgument(function_definition_for_evaluated_action->parameters[i].variable.name,
                                                          named_argument_expressions[i],
                                                          function_definition_for_evaluated_action->parameters[i].variable.types);
        }

        // add deprecated parameters
        for( size_t i = 0; i < function_definition_for_evaluated_action->deprecated_parameters.size(); ++i )
        {
            ASSERT(!function_definition_for_evaluated_action->deprecated_parameters[i].types.empty());
            optional_named_arguments_compiler.AddArgument(function_definition_for_evaluated_action->deprecated_parameters[i].name,
                                                          named_argument_expressions[number_non_deprecated_arguments + i],
                                                          function_definition_for_evaluated_action->deprecated_parameters[i].types);
        }

        optional_named_arguments_compiler.Compile(true);

        // store only the provided arguments
        argument_names_provided.emplace();

        for( size_t i = 0; i < number_arguments; ++i )
        {
            if( named_argument_expressions[i] != -1 )
            {
                const std::string& argument_name = ( i < number_non_deprecated_arguments ) ?
                    function_definition_for_evaluated_action->parameters[i].variable.name :
                    function_definition_for_evaluated_action->deprecated_parameters[i - number_non_deprecated_arguments].name;

                argument_expressions.emplace_back(CreateStringLiteralNode(argument_name));
                argument_expressions.emplace_back(named_argument_expressions[i]);

                argument_names_provided->emplace_back(argument_name);
            }
        }
    }

    // ...using no arguments or providing arguments via JSON
    else
    {
        NextToken();

        // if no arguments are provided, we can check if there any required arguments
        if( Tkn == TOKRPAREN )
        {
            argument_names_provided.emplace();
        }

        // if not using named arguments, the arguments will be provided via JSON
        else
        {
            argument_expressions.emplace_back(-1 * CompileJsonText(
                [&](const JsonNode& json_node)
                {
                    if( action == ActionInvoker::Action::execute )
                    {
                        if( !json_node.Contains(JK::action) )
                            IssueError(MGF::CS_action_missing_9201);

                        evaluated_action = ActionInvoker::GetActionFromText(json_node.Get<std::string_view>(JK::action), *this);
                        get_function_definition_for_evaluated_action();

                        argument_names_provided = json_node.GetOrEmpty(JK::arguments).GetKeys();
                    }

                    else
                    {
                        argument_names_provided = json_node.GetKeys();
                    }
                }));
        }
    }


    // when possible, check that an argument is provided for every required parameter
    if( argument_names_provided.has_value() )
    {
        std::vector<std::string> required_arguments_not_provided;

        for( const GF::Parameter& parameter : function_definition_for_evaluated_action->parameters )
        {
            if( parameter.required &&
                std::find(argument_names_provided->cbegin(), argument_names_provided->cend(), parameter.variable.name) == argument_names_provided->cend() )
            {
                required_arguments_not_provided.emplace_back(parameter.variable.name);
            }
        }

        if( !required_arguments_not_provided.empty() )
        {
            IssueError(MGF::CS_missing_required_arguments_9204, ActionInvoker::GetActionName(evaluated_action).c_str(),
                                                                SO::CreateSingleString(required_arguments_not_provided).c_str());
        }
    }


    IssueErrorOnTokenMismatch(TOKRPAREN, MGF::right_parenthesis_expected_in_function_call_17);

    NextToken();

    return CreateVariableArgumentsWithSizeNode(FunctionCode::CSFN_ACTIONINVOKER_CODE, argument_expressions);
}
