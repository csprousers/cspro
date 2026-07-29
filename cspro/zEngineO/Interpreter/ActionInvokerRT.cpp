#include "stdafx.h"
#include "IncludesRT.h"
#include "EngineAccessor.h"
#include "EngineCaseConstructionReporter.h"
#include "Nodes/GeneralizedFunction.h"
#include <zToolsO/ObjectTransporter.h>
#include <zToolsO/UniqueId.h>
#include <zLogicO/GeneralizedFunction.h>
#include <zLogicO/SpecialFunction.h>
#include <zAction/ActionInvoker.h>
#include <zAction/JsonResponse.h>
#include <zAction/NameProcessors.h>


// --------------------------------------------------------------------------
// ActionInvokerEngineCaller
// --------------------------------------------------------------------------

class ActionInvokerEngineCaller : public ActionInvoker::Caller
{
public:
    ActionInvokerEngineCaller(EngineData& engine_data, CancelFlag& stop_flag)
        :   m_engineData(engine_data),
            m_callerId(UniqueId::CreateInt()),
            m_cancelFlag(stop_flag),
            m_rootDirectory(PortableFunctions::PathGetDirectory(UTF8_TODO::GetUtf8(m_engineData.pff->GetAppFName())))
    {
        ASSERT(m_engineData.engine_accessor != nullptr);
    }

    int GetCallerId() const override
    {
        return m_callerId;
    }

    CancelFlag& GetCancelFlag() override
    {
        return m_cancelFlag;
    }

    std::string GetRootDirectory() override
    {
        return m_rootDirectory;
    }

    std::shared_ptr<CaseConstructionReporter> CreateCaseConstructionReporter() override
    {
        return std::make_unique<EngineCaseConstructionReporter>(
            m_engineData.engine_accessor->ea_GetSharedSystemMessageIssuer(),
            nullptr
        );
    }

private:
    EngineData& m_engineData;
    int m_callerId;
    CancelFlag& m_cancelFlag;
    const std::string m_rootDirectory;
};



// --------------------------------------------------------------------------
// LogicInterpreter
// --------------------------------------------------------------------------

void LogicInterpreter::SetActionInvokerRuntime(std::shared_ptr<ActionInvoker::Runtime> runtime)
{
    ASSERT(m_actionInvokerRuntime == nullptr && runtime != nullptr);
    m_actionInvokerRuntime = std::move(runtime);
}


double LogicInterpreter::ex_ActionInvoker(const int program_index)
{
    // OnActionInvokerResult routines
    const bool has_OnActionInvokerResult = HasSpecialFunction(SpecialFunction::Code::OnActionInvokerResult);
    std::optional<double> result_override_OnActionInvokerResult;

    auto OnActionInvokerResult_process = [&](SharableString action_name, SharableString result, const char* const result_type)
    {
        const double result_override = ExecSpecialFunction(Get_m_iExSymbol_INTERPRETER_DLL_TODO(),
                                                           SpecialFunction::Code::OnActionInvokerResult,
                                                           { std::move(action_name), std::move(result), result_type });
        SharableString result_override_text = GetWorkingSharableString(static_cast<size_t>(result_override));

        if( result_override_text->empty() )
            return false;

        result_override_OnActionInvokerResult = AssignString(std::move(result_override_text));

        return true;
    };

    auto OnActionInvokerResult_process_exception = [&](SharableString action_name, const CSProException& exception)
    {
        return OnActionInvokerResult_process(std::move(action_name), exception.what(), "exception");
    };


    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(program_index);
    ASSERT(va_with_size_node.number_arguments >= 1);

    const ActionInvoker::Action action = static_cast<ActionInvoker::Action>(va_with_size_node.arguments[0]);

    try
    {
        if( m_actionInvokerCaller == nullptr )
        {
            if( m_actionInvokerRuntime == nullptr )
            {
                ObjectTransporter::GetActionInvokerRuntime();
                ASSERT(m_actionInvokerRuntime != nullptr);
            }

            ASSERT(m_engineData->pff != nullptr);
            m_actionInvokerCaller = std::make_unique<ActionInvokerEngineCaller>(*m_engineData, m_bStopProc);
        }

        ASSERT(m_actionInvokerRuntime != nullptr && m_actionInvokerCaller != nullptr);


        SharableString json_arguments;

        // CS.actionName()
        if( va_with_size_node.number_arguments == 1 )
        {
            // nothing to evaluate
        }


        // CS.actionName(json_arguments_text)
        else if( va_with_size_node.arguments[1] < 0 )
        {
            ASSERT(va_with_size_node.number_arguments == 2);
            json_arguments = EvaluateSharableString(-1 * va_with_size_node.arguments[1]);
        }


        // CS.actionName(named_argument1 := , named_argument2 := , ...)
        else
        {
            constexpr size_t ElementsPerArgument = 2;
            ASSERT(va_with_size_node.number_arguments % ElementsPerArgument == 1);

            // convert each argument to JSON
            const std::unique_ptr<JsonStringWriter> json_writer = Json::CreateStringWriter();

            json_writer->BeginObject();

            for( int i = 1; i < va_with_size_node.number_arguments; i += ElementsPerArgument )
            {
                const SharableString name = EvaluateSharableString(va_with_size_node.arguments[i]);
                const auto& gf_value_node = GetNode<Nodes::GeneralizedFunctionValue>(va_with_size_node.arguments[i + 1]);

                json_writer->Key(*name);

                switch( gf_value_node.parameter_variable_type )
                {
                    case GF::VariableType::String:
                    {
                        ASSERT(gf_value_node.argument_variable_type == GF::VariableType::String);
                        json_writer->Write(EvaluateSharableString(gf_value_node.argument_expression));
                        break;
                    }

                    case GF::VariableType::Number:
                    {
                        ASSERT(gf_value_node.argument_variable_type == GF::VariableType::Number);
                        json_writer->Write(Evaluate<double>(gf_value_node.argument_expression));
                        break;
                    }

                    case GF::VariableType::Boolean:
                    {
                        ASSERT(gf_value_node.argument_variable_type == GF::VariableType::Number);
                        json_writer->Write(EvaluateConditional(gf_value_node.argument_expression));
                        break;
                    }

                    case GF::VariableType::Array:
                    {
                        if( gf_value_node.argument_variable_type == GF::VariableType::Array )
                        {
                            const Symbol& symbol = NPT_Ref(gf_value_node.argument_expression);
                            ASSERT(symbol.IsOneOf(SymbolType::Array, SymbolType::List));

                            symbol.WriteValueToJson(*json_writer);
                        }

                        else
                        {
                            ASSERT(gf_value_node.argument_variable_type == GF::VariableType::String);
                            json_writer->Write(Json::Parse(EvaluateSharableString(gf_value_node.argument_expression).GetString()));
                        }

                        break;
                    }

                    case GF::VariableType::Object:
                    {
                        ASSERT(gf_value_node.argument_variable_type == GF::VariableType::String);
                        json_writer->Write(Json::Parse(EvaluateSharableString(gf_value_node.argument_expression).GetString()));
                        break;
                    }

                    default:
                    {
                        ASSERT(false);
                        json_writer->WriteNull();
                        break;
                    }
                }
            }

            json_writer->EndObject();

            json_arguments = json_writer->ReleaseSharableString();
        }


        // run the action
        const ActionInvoker::Result result = m_actionInvokerRuntime->ProcessAction(action, json_arguments, *m_actionInvokerCaller);

        // string results may need to be encoded to JSON string format depending on whether results should be converted
        std::optional<SharableString> string_result_in_json;

        auto ensure_result_in_json = [&]()
        {
            if( !string_result_in_json.has_value() )
            {
                if( result.GetType() == ActionInvoker::Result::Type::JsonText )
                {
                    string_result_in_json = result.GetStringResult();
                }

                else if( result.GetType() != ActionInvoker::Result::Type::Undefined )
                {
                    ASSERT(result.GetType() == ActionInvoker::Result::Type::Bool ||
                           result.GetType() == ActionInvoker::Result::Type::Number ||
                           result.GetType() == ActionInvoker::Result::Type::String);

                    string_result_in_json = result.GetResultAsJsonText<false>();
                }
            }
        };

        if( has_OnActionInvokerResult )
        {
            const char* const result_type = ActionInvoker::JsonResponse::GetResultTypeText<true>(result);

            ensure_result_in_json();

            ASSERT(!string_result_in_json.has_value() == ( strcmp(result_type, "undefined") == 0 ));

            if( !string_result_in_json.has_value() )
                string_result_in_json.emplace();

            if( OnActionInvokerResult_process(ActionInvoker::GetActionName(action), *string_result_in_json, result_type) )
                return *result_override_OnActionInvokerResult;
        }


        // return the result
        if( result.GetType() == ActionInvoker::Result::Type::Undefined )
        {
            return AssignStringNull();
        }

        else if( result.GetType() == ActionInvoker::Result::Type::JsonText )
        {
            ASSERT(SO::EqualsOneOf(ActionInvoker::JsonResponse::GetResultTypeText<true>(result), "object", "array", "null"));
            return AssignString(result.GetStringResult());
        }

        // for bool/numeric/string values, potentially convert the results
        else if( m_engineData->application != nullptr &&
                 m_engineData->application->GetLogicSettings().GetActionInvokerConvertResults() )
        {
            return AssignString(result.GetResultAsString<true>());
        }

        // if not converted, return bool/numeric/string values in JSON string format
        else
        {
            ensure_result_in_json();
            ASSERT(string_result_in_json.has_value());
            return AssignString(std::move(*string_result_in_json));
        }
    }

    catch( const ActionInvoker::Exception& exception )
    {
        const SharableString action_name = ActionInvoker::GetActionName(action);

        if( has_OnActionInvokerResult && OnActionInvokerResult_process_exception(action_name, exception) )
            return *result_override_OnActionInvokerResult;

        IssueMessage(MessageType::Error, 9206, action_name->c_str(), exception.what());
    }

    catch( const CSProException& exception )
    {
        ASSERT(false);
        IssueMessage(MessageType::Error, 9207, exception.what());
    }

    return AssignStringNull();
}
