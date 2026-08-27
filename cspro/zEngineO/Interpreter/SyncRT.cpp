#include "stdafx.h"
#include "IncludesRT.h"
#include "EngineDictionary.h"
#include "SyncDriver.h"
#include <engine/DicT.h>
#include <engine/DicX.h>
#include <engine/EngineDictionaryModifier.h>
#include <zLogicO/SpecialFunction.h>
#include <zMessageO/Messages.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zSyncO/ApplicationPackageManager.h>
#include <zSyncO/BluetoothObexServer.h>
#include <zSyncO/DialogBasedSyncListener.h>
#include <zSyncO/IBluetoothAdapter.h>
#include <zSyncO/SyncClient.h>
#include <zSyncO/SyncLoginAccessor.h>
#include <zSyncO/SyncMessage.h>
#include <zSyncO/SyncObexHandler.h>
#include <zSyncO/SyncServiceFactory.h>

namespace SyncRT { class SyncObexEngineAccessor; }


// --------------------------------------------------------------------------
// SyncDriver
// --------------------------------------------------------------------------

SyncDriver::SyncDriver(LogicInterpreter& interpreter)
    :   m_interpreter(interpreter),
        m_loginAccessor(std::make_unique<SyncLoginAccessor>()),
        m_syncClient(std::make_unique<SyncClient>(GetDeviceId(), std::make_unique<SyncServiceFactory>(m_loginAccessor))),
        m_syncListener(std::make_unique<DialogBasedSyncListener>(m_interpreter.GetSharedSystemMessageIssuer_INTERPRETER_DLL_TODO()))
{
    m_syncClient->SetSyncListener(m_syncListener);
}


SyncDriver::~SyncDriver()
{
}


std::optional<SyncDirection> SyncDriver::EvaluateSyncDirection(const int expression) const
{
    if( expression >= static_cast<int>(SyncDirection::Put) &&
        expression <= static_cast<int>(SyncDirection::Both) )
    {
        return static_cast<SyncDirection>(expression);
    }

    else if( expression < 0 )
    {
        const SharableString direction_string = m_interpreter.Evaluate<SharableString>(-1 * expression);

        return SO::EqualsNoCase(*direction_string, ToString(SyncDirection::Put))  ? std::make_optional(SyncDirection::Put) :
               SO::EqualsNoCase(*direction_string, ToString(SyncDirection::Get))  ? std::make_optional(SyncDirection::Get) :
               SO::EqualsNoCase(*direction_string, ToString(SyncDirection::Both)) ? std::make_optional(SyncDirection::Both) :
                                                                                    std::nullopt;
    }

    else
    {
        return std::nullopt;
    }
}


std::unique_ptr<ApplicationPackageManager> SyncDriver::CreateApplicationPackageManager()
{
#ifdef WIN_DESKTOP
    return nullptr;
#else
    return std::make_unique<ApplicationPackageManager>(PlatformInterface::GetInstance()->GetCSEntryDirectory());
#endif
}



// --------------------------------------------------------------------------
// SyncRT::SyncObexEngineAccessor
// --------------------------------------------------------------------------

class SyncRT::SyncObexEngineAccessor : public ISyncObexEngineAccessor
{
public:
    SyncObexEngineAccessor(LogicInterpreter& interpreter, int field_symbol_index);

    DataRepository* GetDataRepository(const std::string& syncable_dictionary_name, const std::string& dictionary_name) override;
    std::unique_ptr<ApplicationPackageManager> CreateApplicationPackageManager() override;
    std::optional<SharableString> OnSyncMessage(const SyncMessage& sync_message) override;

private:
    LogicInterpreter& m_interpreter;
    int m_fieldSymbolIndex;
};


SyncRT::SyncObexEngineAccessor::SyncObexEngineAccessor(LogicInterpreter& interpreter, const int field_symbol_index)
    :   m_interpreter(interpreter),
        m_fieldSymbolIndex(field_symbol_index)
{
}


DataRepository* SyncRT::SyncObexEngineAccessor::GetDataRepository(const std::string& syncable_dictionary_name,
                                                                  const std::string& dictionary_name)
{
    const int dictionary_symbol_index = m_interpreter.SymbolTableSearch_INTERPRETER_DLL_TODO(
        dictionary_name, { SymbolType::Pre80Dictionary }
    );

    if( dictionary_symbol_index != 0 )
    {
        DICT& dict = m_interpreter.GetSymbol<DICT>(dictionary_symbol_index);

        if( syncable_dictionary_name == dict.GetDataDict()->GetSyncableName() )
            return &dict.GetDicX()->GetDataRepository().GetRealRepository();
    }

    return nullptr;
}


std::unique_ptr<ApplicationPackageManager> SyncRT::SyncObexEngineAccessor::CreateApplicationPackageManager()
{
    return SyncDriver::CreateApplicationPackageManager();
}


std::optional<SharableString> SyncRT::SyncObexEngineAccessor::OnSyncMessage(const SyncMessage& sync_message)
{
    if( !m_interpreter.HasSpecialFunction(SpecialFunction::Code::OnSyncMessage) )
        return std::nullopt;

    Engine::Value message_response = m_interpreter.ExecSpecialFunction(
        m_fieldSymbolIndex,
        SpecialFunction::Code::OnSyncMessage,
        { sync_message.GetName(), sync_message.GetValueForOnSyncMessage() }
    );

    ASSERT(message_response.is<SharableString>());

    return std::move(message_response).as<SharableString>();
}




// --------------------------------------------------------------------------
// synchronization functions
// --------------------------------------------------------------------------

SyncDriver& LogicInterpreter::GetSyncDriver()
{
    if( m_syncDriver == nullptr )
        m_syncDriver = std::make_unique<SyncDriver>(*this);

    return *m_syncDriver;
}


SyncClient& LogicInterpreter::GetSyncClient()
{
    SyncDriver& sync_driver = GetSyncDriver();
    return sync_driver .GetSyncClient();
}


Engine::Value LogicInterpreter::ex_syncconnect(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    SyncClient& sync_client = GetSyncClient();

    int connection_type = va_node.arguments[0];
    std::optional<SyncConnectionString> sync_connection_string;

    // process sync connection strings
    if( connection_type == 0 )
    {
        sync_connection_string.emplace(Evaluate<SharableString>(va_node.arguments[1]).GetString());
        sync_connection_string->AdjustRelativePath(GetCurrentWorkingDirectory());
    }

    // otherwise create a sync connection string from the arguments
    else
    {
        if( m_engineData->PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
        {
            if( connection_type == 5 ) // Web -> CSWeb
            {
                connection_type = 1;
            }

            else if( connection_type > 5 ) // to account for the removed Web
            {
                --connection_type;
            }
        }

        auto evaluate_host_url_and_username_and_password = [&](const SyncServiceType sync_service_type)
        {
            // add the URL and the type (in case the URL doesn't start properly)
            // e.g., in CSPro 8.0 you could say: syncconnect(FTP, "localhost")
            std::string sync_connection_string_text = Evaluate<std::string>(va_node.arguments[1]);
            SO::MakeTrim(sync_connection_string_text);

            // add the type
            sync_connection_string_text.append(FormatText("%c%s=%s", PropertyString::PropertySeparatorInitial,
                                                                     SyncConnectionString::PropertyType,
                                                                     ToString(sync_service_type)));

            sync_connection_string.emplace(sync_connection_string_text);

            // add the username and password
            if( va_node.arguments[2] >= 0 )
            {
                sync_connection_string->SetUsernamePasswordProperties(Evaluate<std::string>(va_node.arguments[2]),
                                                                      Evaluate<std::string>(va_node.arguments[3]));
            }
        };

        switch( connection_type )
        {
            // CSWeb
            case 1:
            {
                evaluate_host_url_and_username_and_password(SyncServiceType::CSWeb);
                break;
            }

            // Bluetooth sync
            case 2:
            {
                std::string sync_connection_string_text = ToString(SyncServiceType::Bluetooth);

                // add the server device name as a path
                if( va_node.arguments[1] != -1 )
                {
                    std::string service_device_name = Evaluate<std::string>(va_node.arguments[1]);
                    SO::MakeTrim(service_device_name);
                    Path::MakeCombineForwardSlash(sync_connection_string_text, Encoders::ToUri(std::move(service_device_name)));
                }

                sync_connection_string.emplace(sync_connection_string_text);

                break;
            }

            // Dropbox sync
            case 3:
            {
                sync_connection_string.emplace(SyncConnectionString::CreateDropboxSyncConnectionString());
                break;
            }

            // FTP
            case 4:
            {
                evaluate_host_url_and_username_and_password(SyncServiceType::Ftp);
                break;
            }

            // LocalDropbox
            case 5:
            {
                sync_connection_string.emplace(SyncConnectionString::CreateLocalDropboxSyncConnectionString());
                break;
            }

            // LocalFiles
            case 6:
            {
                std::string directory_path = Evaluate<std::string>(va_node.arguments[1]);

                if( SO::StartsWith(directory_path, "file:/") )
                {
                    sync_connection_string.emplace(directory_path);
                }

                else
                {
                    MakeAbsolutePath(directory_path);
                    sync_connection_string.emplace(SyncConnectionString::CreateLocalFilesSyncConnectionString(std::move(directory_path)));
                }

                ASSERT(sync_connection_string->GetType() == SyncServiceType::LocalFiles);

                break;
            }

            // Error
            default:
                return ReturnProgrammingError(Engine::Value::Bool(false));
        }
    }

    ASSERT(sync_connection_string.has_value());

    return Engine::Value::Bool(
        ( sync_client.Connect(*sync_connection_string) == SyncClient::SyncResult::SYNC_OK )
    );
}


Engine::Value LogicInterpreter::ex_syncdisconnect(int /*program_index*/)
{
    SyncClient& sync_client = GetSyncClient();

    return Engine::Value::Bool(
        ( sync_client.Disconnect() == SyncClient::SyncResult::SYNC_OK )
    );
}


Engine::Value LogicInterpreter::ex_syncdata(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    SyncDriver& sync_driver = GetSyncDriver();
    const std::optional<SyncDirection> sync_direction = sync_driver.EvaluateSyncDirection(va_node.arguments[0]);

    if( !sync_direction.has_value() )
    {
        IssueMessage(MessageType::Error, MGF::sync_direction_invalid_94000,
                     Logic::FunctionTable::GetFunctionName(va_node.function_code));

        return Engine::Value::Bool(false);
    }

    Symbol& symbol = GetSymbol(va_node.arguments[1]);
    ISyncableDataRepository* syncable_data_repository;

    // because the cases may change during the sync, this object will ensure
    // that any cases currently loaded are properly updated post-sync
    std::unique_ptr<EngineDictionaryModifier> engine_dictionary_modifier;

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
        syncable_data_repository = engine_dictionary.GetEngineDataRepository().GetDataRepository().GetSyncableDataRepository();

        if( *sync_direction != SyncDirection::Put )
            engine_dictionary_modifier = CreateEngineDictionaryModifier_INTERPRETER_DLL_TODO(engine_dictionary);
    }

    else
    {
        DICT& dict = assert_cast<DICT&>(symbol);
        syncable_data_repository = dict.GetDicX()->GetDataRepository().GetSyncableDataRepository();

        if( *sync_direction != SyncDirection::Put )
            engine_dictionary_modifier = CreateEngineDictionaryModifier_INTERPRETER_DLL_TODO(dict);
    }

    if( syncable_data_repository == nullptr )
    {
        IssueMessage(MessageType::Error, MGF::sync_invalid_data_source_100116,
                     Logic::FunctionTable::GetFunctionName(va_node.function_code),
                     symbol.GetName().c_str());

        return Engine::Value::Bool(false);
    }

    const SharableString universe = EvaluateNullableSharableString(va_node.arguments[2]);
    bool success = false;

    try
    {
        std::exception_ptr sync_exception;

        if( engine_dictionary_modifier != nullptr )
            engine_dictionary_modifier->PrepareForModifications();

        try
        {
            SyncClient& sync_client = sync_driver.GetSyncClient();

            if( sync_client.SyncData(*syncable_data_repository, *sync_direction, *universe) == SyncClient::SyncResult::SYNC_OK )
                success = true;
        }
        catch(...) { ASSERT(false); sync_exception = std::current_exception(); }

        if( engine_dictionary_modifier != nullptr )
            engine_dictionary_modifier->FinishedWithModifications();

        if( sync_exception )
            std::rethrow_exception(sync_exception);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::sync_generic_error_100114, exception.what());
    }

    return Engine::Value::Bool(success);
}


Engine::Value LogicInterpreter::ex_syncfile(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    SyncDriver& sync_driver = GetSyncDriver();
    const std::optional<SyncDirection> sync_direction = sync_driver.EvaluateSyncDirection(va_node.arguments[0]);

    if( !sync_direction.has_value() || *sync_direction == SyncDirection::Both )
    {
        IssueMessage(MessageType::Error, MGF::sync_direction_invalid_or_both_94003);
        return Engine::Value::Bool(false);
    }

    auto evaluate_local_path = [&](std::string& path)
    {
        if( path.empty() )
        {
            path = GetCurrentWorkingDirectory();
        }

        else
        {
            MakeAbsolutePath(path);
        }
    };

    std::string from_path = Evaluate<std::string>(va_node.arguments[1]);
    std::string to_path = EvaluateOptionalOrConstruct<std::string>(va_node.arguments[2]);

    if( *sync_direction == SyncDirection::Get )
    {
        evaluate_local_path(to_path);
    }

    else
    {
        ASSERT(*sync_direction == SyncDirection::Put);
        evaluate_local_path(from_path);
    }

    SyncClient& sync_client = sync_driver.GetSyncClient();

    return Engine::Value::Bool(
        ( sync_client.SyncFile(*sync_direction, std::move(from_path), std::move(to_path)) == SyncClient::SyncResult::SYNC_OK )
    );
}


Engine::Value LogicInterpreter::ex_syncserver(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    ASSERT(va_node.arguments[0] == 2); // the connection type is always 2 (Bluetooth) for now

    SyncDriver& sync_driver = GetSyncDriver();
    std::shared_ptr<IBluetoothAdapter> bluetooth_adapter = sync_driver.GetLoginAccessor().GetBluetoothAdapter();

    // Bluetooth not supported on this device
    if( bluetooth_adapter == nullptr )
    {
        IssueMessage(MessageType::Error, MGF::sync_feature_not_supported_100146, "Bluetooth");
        return Engine::Value::Bool(false);
    }

    // Default file root is app directory
    std::string root_directory = ( va_node.arguments[1] != -1 ) ? EvaluatePath(va_node.arguments[1]) :
                                                                  GetCurrentWorkingDirectory();

    BluetoothObexServer bluetooth_server(
        std::move(bluetooth_adapter),
        std::make_unique<SyncObexHandler>(GetDeviceId(), std::move(root_directory), std::make_unique<SyncRT::SyncObexEngineAccessor>(*this, Get_m_iExSymbol_INTERPRETER_DLL_TODO())),
        sync_driver.GetSharedSyncClient()
    );

    try
    {
        return Engine::Value::Bool(
            ( bluetooth_server.run() != 0)
        );
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::sync_error_running_sync_service_100153, exception.what());
        return Engine::Value::Bool(false);
    }
}


Engine::Value LogicInterpreter::ex_syncapp(int /*program_index*/)
{
    const std::string application_file_path = Path::ReplaceExtension(GetCurrentApplicationFilePath(), FileExtensions::BinaryEntryPen);
    const int64_t app_file_time_before = PortableFunctions::FileModifiedTime(application_file_path);

    const std::unique_ptr<ApplicationPackageManager> application_package_manager = SyncDriver::CreateApplicationPackageManager();
    SyncClient& sync_client = GetSyncClient();

    if( application_package_manager != nullptr &&
        sync_client.UpdateApplication(*application_package_manager, GetCurrentApplicationFilePath()) == SyncClient::SyncResult::SYNC_OK )
    {
        if( PortableFunctions::FileModifiedTime(application_file_path) > app_file_time_before )
        {
            sync_client.Disconnect();

            const SharableString restart_message = MGF::GetMessageText(100152);
            ErrorMessage::Display(*restart_message);

#ifndef WIN_DESKTOP
            if( m_engineData->pff != nullptr )
            {
                PlatformInterface::GetInstance()->GetApplicationInterface()->ExecPff(
                    UTF8_TODO::GetUtf8(m_engineData->pff->GetPifFileName())
                );
            }
#endif
            m_bStopProc = true;
            SetStopCode_INTERPRETER_DLL_TODO();
        }

        return Engine::Value::Bool(true);
    }

    return Engine::Value::Bool(false);
}


Engine::Value LogicInterpreter::ex_syncmessage(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    ASSERT(va_node.arguments[0] == -1); // the type of message, for now, is ignored

    const SyncMessage sync_message(Evaluate<SharableString>(va_node.arguments[1]),
                                   EvaluateNullableSharableString(va_node.arguments[2]));

    SyncClient& sync_client = GetSyncClient();
    const std::optional<JsonNode> response_json_node = sync_client.SendSyncMessage(sync_message);

    if( !response_json_node.has_value() )
        return Engine::Value::Undefined<SharableString>();

    return response_json_node->IsString() ? response_json_node->Get<SharableString>() :
                                            response_json_node->GetNodeAsSharableString();
}


Engine::Value LogicInterpreter::ex_syncparadata(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    SyncDriver& sync_driver = GetSyncDriver();
    const std::optional<SyncDirection> sync_direction = sync_driver.EvaluateSyncDirection(va_node.arguments[0]);

    if( !sync_direction.has_value() )
    {
        IssueMessage(MessageType::Error, MGF::sync_direction_invalid_94000, Logic::FunctionTable::GetFunctionName(va_node.function_code));
        return Engine::Value::Bool(false);
    }

    try
    {
        if( !Paradata::Logger::IsOpen() )
            throw CSProException("A paradata log must be open before calling syncparadata.");

        SyncClient& sync_client = sync_driver.GetSyncClient();

        if( sync_client.SyncParadata(*sync_direction) == SyncClient::SyncResult::SYNC_OK )
            return Engine::Value::Bool(true);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::Query_paradata_sync_error_8295, exception.what());
    }

    return Engine::Value::Bool(false);
}


Engine::Value LogicInterpreter::ex_synctime(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    Symbol& symbol = GetSymbol(va_node.arguments[0]);
    ISyncableDataRepository* syncable_data_repository;

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        syncable_data_repository = assert_cast<EngineDictionary&>(symbol).GetEngineDataRepository().GetDataRepository().GetSyncableDataRepository();
    }

    else
    {
        syncable_data_repository = assert_cast<DICT&>(symbol).GetDicX()->GetDataRepository().GetSyncableDataRepository();
    }

    if( syncable_data_repository == nullptr )
    {
        IssueMessage(MessageType::Error, MGF::sync_invalid_data_source_100116,
                     Logic::FunctionTable::GetFunctionName(va_node.function_code),
                     symbol.GetName().c_str());

        return Engine::Value::Undefined<double>();
    }

    SharableString device_identifier = EvaluateNullableSharableString(va_node.arguments[1]);
    const SharableString case_uuid = EvaluateNullableSharableString(va_node.arguments[2]);

    // if a case UUID is provided but no device identifier, make it clear that the device identifier was not set
    if( case_uuid.IsSet() && device_identifier->empty() )
        device_identifier.Reset();

    try
    {
        const std::optional<double> time = syncable_data_repository->GetSyncTime(device_identifier, case_uuid);

        if( time.has_value() )
            return *time;
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::sync_error_running_sync_service_100153, exception.what());
    }

    return Engine::Value::Undefined<double>();
}


Engine::Value LogicInterpreter::ex_getbluetoothname(int /*program_index*/)
{
    SyncDriver& sync_driver = GetSyncDriver();
    const std::shared_ptr<const IBluetoothAdapter> bluetooth_adapter = sync_driver.GetLoginAccessor().GetBluetoothAdapter();

    if( bluetooth_adapter == nullptr )
        return Engine::Value::Undefined<SharableString>();

    return bluetooth_adapter->GetName();
}


Engine::Value LogicInterpreter::ex_setbluetoothname(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString bluetooth_name = Evaluate<SharableString>(fnn_node.fn_expr[0]);

    SyncDriver& sync_driver = GetSyncDriver();
    const std::shared_ptr<IBluetoothAdapter> bluetooth_adapter = sync_driver.GetLoginAccessor().GetBluetoothAdapter();

    if( bluetooth_adapter == nullptr )
    {
        IssueMessage(MessageType::Error, MGF::sync_feature_not_supported_100146, "Bluetooth");
        return Engine::Value::Bool(false);
    }

    try
    {
        // only set the Bluetooth name when it differs from the current one
        if( bluetooth_adapter->GetName() != *bluetooth_name )
            bluetooth_adapter->SetName(*bluetooth_name);

        return Engine::Value::Bool(true);
    }

    catch( const CSProException& exception )
    {
        IssueMessage(MessageType::Error, MGF::sync_set_bluetooth_name_error_100174, exception.what());
        return Engine::Value::Bool(false);
    }
}
