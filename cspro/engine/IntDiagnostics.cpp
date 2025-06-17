#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "Engine.h"
#include "ParameterManager.h"
#include <zToolsO/Serializer.h>


double CIntDriver::exdiagnostics(int iExpr)
{
    auto run = [&](int number_arguments, const int* arguments)
    {
        bool show_all_parameters = ( number_arguments == 0 );
        std::optional<ParameterManager::Parameter> parameter;

        if( !show_all_parameters )
        {
            std::wstring parameter_text = EvalAlphaExpr(arguments[0]);
            int min_arguments;
            int max_arguments;
            int provided_arguments = number_arguments - 1;

            parameter = ParameterManager::Parse(FNDIAGNOSTICS_CODE, parameter_text, &min_arguments, &max_arguments);

            if( *parameter == ParameterManager::Parameter::Invalid )
            {
                issaerror(MessageType::Error, 1100, UTF8_TODO::GetUtf8(parameter_text).c_str());
                return AssignStringNull();
            }

            // check if the number of arguments is valid
            if( provided_arguments < min_arguments || provided_arguments > max_arguments )
            {
                issaerror(MessageType::Error, 1101, UTF8_TODO::GetUtf8(parameter_text).c_str(), provided_arguments);
                return AssignStringNull();
            }
        }

        ASSERT(show_all_parameters == !parameter.has_value() && parameter != ParameterManager::Parameter::Invalid);

        // if no parameter was provided, construct a string with all of the values of the zero-argument parameters
        std::wstring diagnostics_text;

        auto assign_parameter = [&](auto parameter, wstring_view value)
        {
            if( show_all_parameters )
            {
                SO::AppendWithSeparator(diagnostics_text,
                                        SO::ConcatenateWS(ParameterManager::GetDisplayName(parameter), _T(": "), value),
                                        _T(", "));
            }

            else
            {
                diagnostics_text = value;
            }
        };

        if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_Version )
            assign_parameter(ParameterManager::Parameter::Diagnostics_Version, UTF8_TODO::GetWide(Versioning::NumberText));

        if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_VersionDetailed )
            assign_parameter(ParameterManager::Parameter::Diagnostics_VersionDetailed, UTF8_TODO::GetWide(Versioning::NumberDetailedText));

        if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_ReleaseDate )
            assign_parameter(ParameterManager::Parameter::Diagnostics_ReleaseDate, UTF8_TODO::GetCString(IntToString(Versioning::GetReleaseDate())));

        if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_Beta )
            assign_parameter(ParameterManager::Parameter::Diagnostics_Beta, Versioning::IsBeta ? _T("1") : _T("0"));

        if( show_all_parameters || *parameter == ParameterManager::Parameter::Diagnostics_Serializer )
            assign_parameter(ParameterManager::Parameter::Diagnostics_Serializer, UTF8_TODO::GetCString(IntToString(Serializer::GetCurrentVersion())));

        if( parameter.has_value() && *parameter == ParameterManager::Parameter::Diagnostics_Md5 )
        {
            std::wstring filename = EvalFullPathFileName(arguments[1]);
            diagnostics_text = UTF8_TODO::GetWide(PortableFunctions::FileMd5(filename));
        }

        return AssignAlphaValue(std::move(diagnostics_text));
    };

    if( m_engineData->MeetsCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
    {
        const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
        return run(fnn_node.fn_nargs, fnn_node.fn_expr);
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

        const LL* ll_node = &GetNode<LL>(iExpr);
        ASSERT(ll_node->code_or_value == FNDIAGNOSTICS_CODE);

        while( ll_node->next >= 0 )
        {
            ll_node = &GetNode<LL>(ll_node->next);
            arguments.emplace_back(ll_node->code_or_value);
        }

        return run((int)arguments.size(), arguments.data());
    }
}
