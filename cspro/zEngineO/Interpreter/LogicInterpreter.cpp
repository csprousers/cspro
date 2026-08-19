#include "stdafx.h"
#include "IncludesRT.h"
#include "LogicInterpreter.h"
#include "Userbar.h"
#include "Nodes/TextTemplate.h"
#include <zHtml/VirtualFileMapping.h>
#include <zEngineF/TraceHandler.h>
#include <zAction/Caller.h>


LogicInterpreter::LogicInterpreter(cs::non_null_shared_or_raw_ptr<EngineData> engine_data,
                                   cs::non_null_shared_or_raw_ptr<ApplicationInterface> application_interface)
    :   m_symbolTable(engine_data->symbol_table),
        m_logicByteCode(engine_data->logic_byte_code),
        m_currentEncodeType(EncodeType::Html),
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
    return IsTrue(Evaluate<double>(program_index));
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
        return Evaluate<double>(program_index);
    }

    else
    {
        ASSERT(value_data_type == DataType::String);
        return Evaluate<ST>(program_index);
    }
}

template ZENGINEO_API std::variant<double, SharableString> LogicInterpreter::EvaluateVariant(DataType value_data_type, int program_index);
template ZENGINEO_API std::variant<double, std::string> LogicInterpreter::EvaluateVariant(DataType value_data_type, int program_index);
