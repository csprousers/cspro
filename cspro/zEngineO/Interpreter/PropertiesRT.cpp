#include "stdafx.h"
#include "IncludesRT.h"
#include "ParameterManager.h"
#include <zToolsO/Hash.h>


double LogicInterpreter::ex_diagnostics(const int program_index)
{
    std::unique_ptr<std::byte[]> fnn_node_for_pre_80;
    const FNN_NODE* fnn_node;

    if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        fnn_node = &GetNode<FNN_NODE>(program_index);
    }

    else
    {
        std::vector<int> arguments;

        // process the old FNLL_NODE linked list node
        struct LL
        {
            int code_or_value;
            int next;
        };

        const LL* ll_node = &GetNode<LL>(program_index);
        ASSERT(ll_node->code_or_value == FunctionCode::FNDIAGNOSTICS_CODE);

        while( ll_node->next >= 0 )
        {
            ll_node = &GetNode<LL>(ll_node->next);
            arguments.emplace_back(ll_node->code_or_value);
        }

        fnn_node_for_pre_80 = std::make_unique_for_overwrite<std::byte[]>(sizeof(FNN_NODE) + sizeof(int) * ( arguments.size() - 1 ));
        FNN_NODE* const modifiable_fnn_node = reinterpret_cast<FNN_NODE*>(fnn_node_for_pre_80.get());
        modifiable_fnn_node->fn_nargs = int32_cast(arguments.size());
        memcpy(modifiable_fnn_node->fn_expr, arguments.data(), arguments.size() * sizeof(int));
        fnn_node = modifiable_fnn_node;
    }

    const bool show_all_parameters = ( fnn_node->fn_nargs == 0 );
    std::optional<ParameterManager::Parameter> parameter;

    if( !show_all_parameters )
    {
        const SharableString parameter_text = Evaluate<SharableString>(fnn_node->fn_expr[0]);
        int min_arguments;
        int max_arguments;
        const int provided_arguments = fnn_node->fn_nargs - 1;

        parameter = ParameterManager::Parse(FunctionCode::FNDIAGNOSTICS_CODE, *parameter_text, &min_arguments, &max_arguments);

        if( *parameter == ParameterManager::Parameter::Invalid )
        {
            IssueMessage(MessageType::Error, MGF::property_invalid_parameter_1100, parameter_text->c_str());
            return AssignStringNull();
        }

        // check if the number of arguments is valid
        if( provided_arguments < min_arguments || provided_arguments > max_arguments )
        {
            IssueMessage(MessageType::Error, MGF::property_arguments_count_mismatch_1101,
                         ParameterManager::GetDisplayName(*parameter), provided_arguments);
            return AssignStringNull();
        }
    }

    ASSERT(show_all_parameters == !parameter.has_value() &&
           parameter != ParameterManager::Parameter::Invalid);

    // if no parameter was provided, construct a string with all of the values of the zero-argument parameters
    std::string diagnostics_text;

    auto assign_parameter = [&](const ParameterManager::Parameter parameter, const std::string_view value_sv)
    {
        if( show_all_parameters )
        {
            if( !diagnostics_text.empty() )
                diagnostics_text.append(", ");

            diagnostics_text.append(ParameterManager::GetDisplayName(parameter))
                            .append(": ")
                            .append(value_sv);
        }

        else
        {
            diagnostics_text = value_sv;
        }
    };

    if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_Version )
        assign_parameter(ParameterManager::Parameter::Diagnostics_Version, Versioning::NumberText);

    if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_VersionDetailed )
        assign_parameter(ParameterManager::Parameter::Diagnostics_VersionDetailed, Versioning::NumberDetailedText);

    if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_ReleaseDate )
        assign_parameter(ParameterManager::Parameter::Diagnostics_ReleaseDate, IntToString(Versioning::GetReleaseDate()));

    if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_Beta )
        assign_parameter(ParameterManager::Parameter::Diagnostics_Beta, Versioning::IsPrerelease ? "1" : "0");

    if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_Serializer )
        assign_parameter(ParameterManager::Parameter::Diagnostics_Serializer, IntToString(Serializer::GetCurrentVersion()));

    if( parameter.has_value() && *parameter == ParameterManager::Parameter::Diagnostics_Md5 )
    {
        const std::string file_path = EvaluatePath(fnn_node->fn_expr[1]);
        diagnostics_text = Hash::Md5::CreateFromFile(file_path);
    }

    return AssignString(std::move(diagnostics_text));
}
