#include "stdafx.h"
#include "IncludesRT.h"
#include "EngineDictionary.h"
#include "HashMap.h"
#include <engine/DicX.h>
#include <zUtilO/CommonStore.h>


Engine::Value LogicInterpreter::ex_getusername(int /*program_index*/)
{
    return GetDeviceUserName();
}


Engine::Value LogicInterpreter::ex_getos(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const int& additional_details_node = va_node.arguments[0];

    // the return value: Windows = 10, Android = 20
    constexpr int os_number = OnWindows() ? 10 : 20;

    if( additional_details_node != -1 )
    {
        const OperatingSystemDetails& operating_system_details = GetOperatingSystemDetails();

        // fill in a hashmap with all details...
        if( additional_details_node == -2 )
        {
            LogicHashMap& hashmap = GetSymbolLogicHashMap(va_node.arguments[1]);
            ASSERT(hashmap.IsValueTypeString() &&
                   hashmap.GetNumberDimensions() == 1 &&
                   hashmap.DimensionTypeHandles(0, DataType::String));

            hashmap.Reset();

            hashmap.SetValue({ "name" }, SharableString::FromStaticStringPointer(&operating_system_details.operating_system));
            hashmap.SetValue({ "version" }, SharableString::FromStaticStringPointer(&operating_system_details.version_number));

            if( operating_system_details.build_number.has_value() )
                hashmap.SetValue({ "build" }, SharableString::FromStaticStringPointer(&(*operating_system_details.build_number)));
        }

        // ...or a string with the operating system and version
        else
        {
            SharableString text_description = SO::Concatenate(
                operating_system_details.operating_system, ";", operating_system_details.version_number
            );

            AssignValueToSymbol(GetNode<Nodes::SymbolValue>(additional_details_node), std::move(text_description));
        }
    }

    return Engine::Value::Integer(os_number);
}


Engine::Value LogicInterpreter::ex_getdeviceid(int /*program_index*/)
{
    // originally this function was called getmac and returend the MAC address; on 20141218 it was decided
    // to change it so that it returns a unique device ID; on Windows it will return the MAC address, while on
    // Android it will return the ANDROID_ID (which is longer than the MAC address)
    return SharableString::FromStaticStringPointer(&GetDeviceId());
}


Engine::Value LogicInterpreter::ex_uuid(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    if( va_node.arguments[0] == -1 )
    {
        return CreateUuid();
    }

    else
    {
        // get the UUID of a case or create one if needed
        Symbol& symbol = NPT_Ref(va_node.arguments[0]);

        if( symbol.IsA(SymbolType::Dictionary) )
        {
            return assert_cast<EngineDictionary&>(symbol).GetEngineCase().GetCase().GetOrCreateUuid();
        }

        else
        {
            return assert_cast<DICT&>(symbol).GetDicX()->GetCase().GetOrCreateUuid();
        }
    }
}


Engine::Value LogicInterpreter::ex_sysparm(const int program_index)
{
    // previously this function only returned the one Parameter= parameter, but
    // now it can also return values from a map of parameters
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);

    if( fnn_node.fn_nargs == 0 )
    {
        if( m_engineData->pff != nullptr )
            return UTF8_TODO::GetUtf8(m_engineData->pff->GetParamString());
    }

    else
    {
        SharableString argument = Evaluate<SharableString>(fnn_node.fn_expr[0]);

        SharableString parameter = ( m_engineData->pff != nullptr )
            ? m_engineData->pff->GetCustomParamString(*argument)
            : SharableString();

        // if the parameter isn't specified in the PFF file, check if it is a command line argument;
        // if so, return the argument (meaning that checking if sysparm isn't blank is a way of seeing
        // if something is defined on the command line)
        if( parameter->empty() )
        {
            const std::string command_line = SO::ToLower(PortableFunctions::GetCommandLine());
            argument.MakeLower();
            const size_t argument_pos = command_line.find(*argument);

            // make sure that the argument is a standalone argument
            if( argument_pos != std::string::npos && argument_pos > 0 )
            {
                const size_t argument_end_pos = argument_pos + argument->length();

                if( std::isspace(command_line[argument_pos - 1]) &&
                    ( argument_end_pos == command_line.length() || std::isspace(command_line[argument_end_pos]) ) )
                {
                    parameter = command_line.substr(argument_pos, argument->length());
                }
            }
        }

        return parameter;
    }

    return Engine::Value::Undefined<SharableString>();
}


Engine::Value LogicInterpreter::ex_savesetting(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    bool success = false;

    CommonStore* const common_store = m_engineData->GetCommonStore().get();

    if( common_store != nullptr )
    {
        common_store->SwitchTable(CommonStore::TableType::UserSettings);

        // clear the database
        if( va_node.arguments[0] == -1 )
        {
            success = common_store->Clear();
        }

        else
        {
            const SharableString key = Evaluate<SharableString>(va_node.arguments[0]);
            const SharableString value = Evaluate<Engine::Value>(va_node.arguments[2]).as<SharableString>();

            if( value->empty() )
            {
                success = common_store->Delete(*key);
            }

            else
            {
                success = common_store->PutString(*key, *value);
            }
        }
    }

    return success;
}


Engine::Value LogicInterpreter::ex_loadsetting(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    const SharableString key = Evaluate<SharableString>(va_node.arguments[0]);
    std::optional<std::string> value;

    CommonStore* const common_store = m_engineData->GetCommonStore().get();

    if( common_store != nullptr )
    {
        common_store->SwitchTable(CommonStore::TableType::UserSettings);

        value = common_store->GetString(*key);

        // if they gave a default value, put that in the database and return it
        if( !value.has_value() && va_node.arguments[1] != -1 )
        {
            SharableString default_value = Evaluate<Engine::Value>(va_node.arguments[2]).as<SharableString>();
            common_store->PutString(*key, *default_value);
            return default_value;
        }
    }

    return value.has_value() ? Engine::Value(std::move(*value)) :
                               Engine::Value::Undefined<SharableString>();
}
