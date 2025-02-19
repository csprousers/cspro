#include "stdafx.h"
#include "IncludesRT.h"
#include "LogicInterpreter.h"
#include "Nodes/Various.h"
#include <zHtml/VirtualFileMapping.h>
#include <zAction/Caller.h>


LogicInterpreter::LogicInterpreter(cs::non_null_shared_or_raw_ptr<EngineData> engine_data,
                                   cs::non_null_shared_or_raw_ptr<ApplicationInterface> application_interface)
    :   m_symbolTable(engine_data->symbol_table),
        m_logicByteCode(engine_data->logic_byte_code),
        m_currentEncodeType(Nodes::EncodeType::Html),
        m_engineData(std::move(engine_data)),
        m_applicationInterface(std::move(application_interface)),
        m_usingLogicSettingsV0(true)
{
}


LogicInterpreter::~LogicInterpreter()
{
}



// --------------------------------------------------------------------------
// general evaluation routines
// --------------------------------------------------------------------------

bool LogicInterpreter::EvaluateConditional(const int program_index)
{
    return IsTrue(Evaluate(program_index));
}


bool LogicInterpreter::EvaluateOptionalConditional(const int program_index, const bool default_value)
{
    if( program_index != -1 )
        return EvaluateConditional(program_index);

    return default_value;
}


std::optional<bool> LogicInterpreter::EvaluateOptionalConditional(const int program_index)
{
    if( program_index != -1 )
        return EvaluateConditional(program_index);

    return std::nullopt;
}


template<typename ST/* = SharableString*/>
std::variant<double, ST> LogicInterpreter::EvaluateVariant(const DataType value_data_type, const int program_index)
{
    if( value_data_type == DataType::Numeric )
    {
        return Evaluate(program_index);
    }

    else
    {
        ASSERT(value_data_type == DataType::String);
        return Evaluate<ST>(program_index);
    }
}

template ZENGINEO_API std::variant<double, SharableString> LogicInterpreter::EvaluateVariant(DataType value_data_type, int program_index);
template ZENGINEO_API std::variant<double, std::string> LogicInterpreter::EvaluateVariant(DataType value_data_type, int program_index);



// --------------------------------------------------------------------------
// general assignment routines
// --------------------------------------------------------------------------

template<typename T>
T LogicInterpreter::GetInvalidValue()
{
    if constexpr(std::is_same_v<T, double>)
    {
        return DEFAULT;
    }

    else if constexpr(std::is_same_v<T, SharableString>)
    {
        return SharableString();
    }

    else
    {
        static_assert_false();
    }
}

template ZENGINEO_API double LogicInterpreter::GetInvalidValue();
template ZENGINEO_API SharableString LogicInterpreter::GetInvalidValue();


double LogicInterpreter::AssignInvalidValue(const DataType data_type)
{
    if( IsNumeric(data_type) )
    {
        return GetInvalidValue<double>();
    }

    else
    {
        ASSERT(IsString(data_type));
        return AssignStringNull();
    }
}


double LogicInterpreter::AssignVariantValue(std::variant<double, SharableString>&& value)
{
    return std::holds_alternative<double>(value) ? std::get<double>(value) :
                                                   AssignString(std::move(std::get<SharableString>(value)));
}


double LogicInterpreter::AssignVariantValue(const std::variant<double, SharableString>& value)
{
    return std::holds_alternative<double>(value) ? std::get<double>(value) :
                                                   AssignString(std::get<SharableString>(value));
}
