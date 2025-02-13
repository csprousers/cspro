#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zToolsO/ObjectTransporter.h>
#include <zJson/Json.h>
#include <ZBRIDGEO/npff.h>
#include <zLogicO/GeneralizedFunction.h>
#include <zLogicO/SpecialFunction.h>
#include <zEngineO/Nodes/GeneralizedFunction.h>
#include <zAction/ActionInvoker.h>
#include <zAction/JsonResponse.h>
#include <zAction/NameProcessors.h>


// --------------------------------------------------------------------------
// ActionInvokerEngineCaller
// --------------------------------------------------------------------------

namespace
{
    class ActionInvokerEngineCaller : public ActionInvoker::Caller
    {
    public:
        ActionInvokerEngineCaller(const PFF& pff, bool& stop_flag)
            :   m_cancelFlag(stop_flag),
                m_rootDirectory(PortableFunctions::PathGetDirectory(pff.GetAppFName()))
        {
        }

        bool& GetCancelFlag() override { return m_cancelFlag; }

        std::wstring GetRootDirectory() override { return m_rootDirectory; }

    private:
        bool& m_cancelFlag;
        const std::wstring m_rootDirectory;
    };
}



// --------------------------------------------------------------------------
// CIntDriver
// --------------------------------------------------------------------------

double CIntDriver::exActionInvoker(int program_index)
{
    // OnActionInvokerResult routines
    const bool has_OnActionInvokerResult = HasSpecialFunction(SpecialFunction::OnActionInvokerResult);
    std::optional<double> result_override_OnActionInvokerResult;

    auto OnActionInvokerResult_process = [&](std::wstring action_name, const std::wstring& result, const TCHAR* const result_type)
    {
        const std::vector<std::variant<double, std::wstring>> arguments({ std::move(action_name), result, result_type });

        std::wstring result_override_text = CharacterObjectToString(ExecSpecialFunction(m_iExSymbol, SpecialFunction::OnActionInvokerResult, arguments));

        if( result_override_text.empty() )
            return false;

        result_override_OnActionInvokerResult = AssignAlphaValue(std::move(result_override_text));

        return true;
    };

    auto OnActionInvokerResult_process_exception = [&](std::wstring action_name, const CSProException& exception)
    {
        return OnActionInvokerResult_process(std::move(action_name), exception.GetErrorMessage(), _T("exception"));
    };


    const auto& va_with_size_node = GetNode<Nodes::VariableArgumentsWithSize>(program_index);
    ASSERT(va_with_size_node.number_arguments >= 1);

    const ActionInvoker::Action action = static_cast<ActionInvoker::Action>(va_with_size_node.arguments[0]);

    try
    {
        if( m_actionInvokerRuntime == nullptr )
        {
            m_actionInvokerRuntime = ObjectTransporter::GetActionInvokerRuntime();

            ASSERT(m_pEngineDriver->m_pPifFile != nullptr);
            m_actionInvokerCaller = std::make_unique<ActionInvokerEngineCaller>(*m_pEngineDriver->m_pPifFile, m_bStopProc);
        }

        ASSERT(m_actionInvokerRuntime != nullptr && m_actionInvokerCaller != nullptr);

        std::optional<std::wstring> json_arguments;

        // CS.actionName()
        if( va_with_size_node.number_arguments == 1 )
        {
            // nothing to evaluate
        }


        // CS.actionName(json_arguments_text)
        else if( va_with_size_node.arguments[1] < 0 )
        {
            ASSERT(va_with_size_node.number_arguments == 2);
            json_arguments = EvalAlphaExpr(-1 * va_with_size_node.arguments[1]);
        }


        // CS.actionName(named_argument1 := , named_argument2 := , ...)
        else
        {
            constexpr size_t ElementsPerArgument = 2;
            ASSERT(va_with_size_node.number_arguments % ElementsPerArgument == 1);

            // convert each argument to JSON
            auto json_writer = Json::CreateStringWriter(json_arguments.emplace());

            json_writer->BeginObject();

            for( int i = 1; i < va_with_size_node.number_arguments; i += ElementsPerArgument )
            {
                const std::wstring name = EvalAlphaExpr(va_with_size_node.arguments[i]);
                const auto& gf_value_node = GetNode<Nodes::GeneralizedFunctionValue>(va_with_size_node.arguments[i + 1]);

                json_writer->Key(name);

                switch( gf_value_node.parameter_variable_type )
                {
                    case GF::VariableType::String:
                    {
                        ASSERT(gf_value_node.argument_variable_type == GF::VariableType::String);
                        json_writer->Write(EvalAlphaExpr(gf_value_node.argument_expression));                        
                        break;
                    }

                    case GF::VariableType::Number:
                    {
                        ASSERT(gf_value_node.argument_variable_type == GF::VariableType::Number);
                        json_writer->Write(evalexpr(gf_value_node.argument_expression));
                        break;
                    }

                    case GF::VariableType::Boolean:
                    {
                        ASSERT(gf_value_node.argument_variable_type == GF::VariableType::Number);
                        json_writer->Write(ConditionalValueIsTrue(evalexpr(gf_value_node.argument_expression)));
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
                            json_writer->Write(Json::Parse(EvalAlphaExpr(gf_value_node.argument_expression)));
                        }

                        break;
                    }

                    case GF::VariableType::Object:
                    {
                        ASSERT(gf_value_node.argument_variable_type == GF::VariableType::String);
                        json_writer->Write(Json::Parse(EvalAlphaExpr(gf_value_node.argument_expression)));
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
        }


        // run the action
        ActionInvoker::Result result = m_actionInvokerRuntime->ProcessAction(action, json_arguments, *m_actionInvokerCaller);

        // string results may need to be encoded to JSON string format depending on whether results should be converted
        std::unique_ptr<std::wstring> string_result_in_json;
        const std::wstring* result_in_json = nullptr;

        auto ensure_result_in_json = [&]()
        {
            if( result_in_json == nullptr )
            {
                if( result.GetType() == ActionInvoker::Result::Type::JsonText )
                {
                    result_in_json = &result.GetStringResult();
                }

                else if( result.GetType() != ActionInvoker::Result::Type::Undefined )
                {
                    ASSERT(result.GetType() == ActionInvoker::Result::Type::Bool ||
                           result.GetType() == ActionInvoker::Result::Type::Number ||
                           result.GetType() == ActionInvoker::Result::Type::String);

                    string_result_in_json = std::make_unique<std::wstring>(result.GetResultAsJsonText<false>());
                    result_in_json = string_result_in_json.get();
                }
            }
        };

        if( has_OnActionInvokerResult )
        {
            const TCHAR* const result_type = ActionInvoker::JsonResponse::GetResultTypeText<true>(result);

            ensure_result_in_json();

            ASSERT(( result_in_json == nullptr ) == SO::Equals(result_type, _T("undefined")));

            if( result_in_json == nullptr )
                result_in_json = &SO::EmptyString;

            if( OnActionInvokerResult_process(ActionInvoker::GetActionName(action), *result_in_json, result_type) )
                return *result_override_OnActionInvokerResult;
        }


        // return the result
        if( result.GetType() == ActionInvoker::Result::Type::Undefined )
        {
            return AssignBlankAlphaValue();
        }

        else if( result.GetType() == ActionInvoker::Result::Type::JsonText )
        {
            ASSERT(SO::EqualsOneOf(ActionInvoker::JsonResponse::GetResultTypeText<true>(result), _T("object"), _T("array"), _T("null")));
            return AssignAlphaValue(result.ReleaseStringResult());
        }

        // for bool/numeric/string values, potentially convert the results
        else if( m_pEngineDriver->GetApplication()->GetLogicSettings().GetActionInvokerConvertResults() )
        {
            return AssignAlphaValue(result.ReleaseResultAsString<true>());
        }

        // if not converted, return bool/numeric/string values in JSON string format
        else
        {
            ensure_result_in_json();
            ASSERT(string_result_in_json != nullptr);
            return AssignAlphaValue(std::move(*string_result_in_json));
        }
    }

    catch( const ActionInvoker::ExceptionWithActionName& exception )
    {
        if( has_OnActionInvokerResult && OnActionInvokerResult_process_exception(exception.GetActionName(), exception) )
            return *result_override_OnActionInvokerResult;

        issaerror(MessageType::Error, 9206, exception.GetActionName().c_str(), exception.GetErrorMessage().c_str());
    }
    
    catch( const CSProException& exception )
    {
        if( has_OnActionInvokerResult && OnActionInvokerResult_process_exception(std::wstring(), exception) )
            return *result_override_OnActionInvokerResult;

        issaerror(MessageType::Error, 9207, exception.GetErrorMessage().c_str());
    }

    return AssignBlankAlphaValue();
}
