#include "StandardSystemIncludes.h"
#include "Interpreter.h"
#include "EngineDictionaryModifier.h"
#include <zToolsO/Encoders.h>
#include <zLogicO/SpecialFunction.h>
#include <zEngineO/EngineDictionary.h>
#include <zPlatformO/PlatformInterface.h>
#include <zMessageO/Messages.h>
#include <zBridgeO/NPff.h>
#include <zDataO/ISyncableDataRepository.h>
#include <zNetwork/LoginAccessor.h>
#include <zSyncO/ApplicationPackageManager.h>
#include <zSyncO/BluetoothObexServer.h>
#include <zSyncO/DialogBasedSyncListener.h>
#include <zSyncO/IBluetoothAdapter.h>
#include <zSyncO/SyncClient.h>
#include <zSyncO/SyncLoginAccessor.h>
#include <zSyncO/SyncMessage.h>
#include <zSyncO/SyncObexHandler.h>
#include <zSyncO/SyncServiceFactory.h>
#include <zParadataO/Logger.h>


namespace
{
    std::optional<SyncDirection> GetSyncDirection(CIntDriver& interpreter, const int expression)
    {
        if( expression >= static_cast<int>(SyncDirection::Put) &&
            expression <= static_cast<int>(SyncDirection::Both) )
        {
            return static_cast<SyncDirection>(expression);
        }

        else if( expression < 0 )
        {
            const std::string direction_string = interpreter.EvaluateString(-1 * expression);

            return SO::EqualsNoCase(direction_string, ToString(SyncDirection::Put))  ? std::make_optional(SyncDirection::Put) :
                   SO::EqualsNoCase(direction_string, ToString(SyncDirection::Get))  ? std::make_optional(SyncDirection::Get) :
                   SO::EqualsNoCase(direction_string, ToString(SyncDirection::Both)) ? std::make_optional(SyncDirection::Both) :
                                                                                       std::nullopt;
        }

        else
        {
            return std::nullopt;
        }
    }


    std::unique_ptr<ApplicationPackageManager> CreateApplicationPackageManager()
    {
#ifdef WIN_DESKTOP
        return nullptr;
#else
        return std::make_unique<ApplicationPackageManager>(PlatformInterface::GetInstance()->GetCSEntryDirectory());
#endif
    }


    class SyncObexEngineAccessor : public ISyncObexEngineAccessor
    {
    public:
        SyncObexEngineAccessor(CIntDriver& interpreter, const int field_symbol_index)
            :   m_interpreter(interpreter),
                m_pEngineArea(m_interpreter.m_pEngineArea),
                m_fieldSymbolIndex(field_symbol_index)
        {
        }

        DataRepository* GetDataRepository(const std::string& syncable_dictionary_name, const std::string& dictionary_name) override
        {
            const int dictionary_symbol_index = m_pEngineArea->SymbolTableSearch(dictionary_name, { SymbolType::Pre80Dictionary });

            if( dictionary_symbol_index != 0 )
            {
                DICT* pDicT = DPT(dictionary_symbol_index);
                DICX* pDicX = pDicT->GetDicX();

                if( syncable_dictionary_name == pDicT->GetDataDict()->GetSyncableName() )
                    return &pDicX->GetDataRepository().GetRealRepository();
            }

            return nullptr;
        }

        std::unique_ptr<ApplicationPackageManager> CreateApplicationPackageManager() override
        {
            return ::CreateApplicationPackageManager();
        }

        std::optional<SharableString> OnSyncMessage(const SyncMessage& sync_message) override
        {
            if( !m_interpreter.HasSpecialFunction(SpecialFunction::Code::OnSyncMessage) )
                return std::nullopt;

            const double message_response = m_interpreter.ExecSpecialFunction(m_fieldSymbolIndex,
                                                                              SpecialFunction::Code::OnSyncMessage,
                                                                              { sync_message.GetName(), sync_message.GetValueForOnSyncMessage() });

            return m_interpreter.GetWorkingSharableString(static_cast<size_t>(message_response));
        }


    private:
        const Logic::SymbolTable& GetSymbolTable() const { return m_pEngineArea->GetSymbolTable(); }

    private:
        CIntDriver& m_interpreter;
        CEngineArea* m_pEngineArea;
        int m_fieldSymbolIndex;
    };
}


struct SyncObjects
{
    std::shared_ptr<LoginAccessor> login_accessor;
    std::unique_ptr<SyncClient> sync_client;
    std::shared_ptr<SyncListener> sync_listener;
};


SyncClient& CIntDriver::GetSyncClient()
{
    if( m_syncObjects == nullptr )
    {
        auto login_accessor = std::make_shared<SyncLoginAccessor>();

        m_syncObjects = std::make_unique<SyncObjects>(
            SyncObjects
            {
                login_accessor,
                std::make_unique<SyncClient>(GetDeviceId(), std::make_unique<SyncServiceFactory>(std::move(login_accessor))),
                std::make_unique<DialogBasedSyncListener>(m_pEngineDriver->GetSharedSystemMessageIssuer())
            });

            m_syncObjects->sync_client->SetSyncListener(m_syncObjects->sync_listener);
    }

    return *m_syncObjects->sync_client;
}


double CIntDriver::ex_syncconnect(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    SyncClient& sync_client = GetSyncClient();

    int connection_type = va_node.arguments[0];
    std::optional<SyncConnectionString> sync_connection_string;

    // process sync connection strings
    if( connection_type == 0 )
    {
        sync_connection_string.emplace(EvaluateSharableString(va_node.arguments[1]).GetString());
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
            std::string sync_connection_string_text = EvaluateString(va_node.arguments[1]);
            SO::MakeTrim(sync_connection_string_text);

            // add the type
            sync_connection_string_text.append(FormatText("%c%s=%s", PropertyString::PropertySeparatorInitial,
                                                                     SyncConnectionString::PropertyType,
                                                                     ToString(sync_service_type)));

            sync_connection_string.emplace(sync_connection_string_text);

            // add the username and password
            if( va_node.arguments[2] >= 0 )
            {
                sync_connection_string->SetUsernamePasswordProperties(EvaluateString(va_node.arguments[2]),
                                                                      EvaluateString(va_node.arguments[3]));
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
                    std::string service_device_name = EvaluateString(va_node.arguments[1]);
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
                std::string directory_path = EvaluateString(va_node.arguments[1]);

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
                return ReturnProgrammingError(0);
        }
    }

    ASSERT(sync_connection_string.has_value());

    const SyncClient::SyncResult result = sync_client.Connect(*sync_connection_string);

    return ( result == SyncClient::SyncResult::SYNC_OK );
}


double CIntDriver::ex_syncdisconnect(int /*program_index*/)
{
    return ( GetSyncClient().Disconnect() == SyncClient::SyncResult::SYNC_OK );
}


double CIntDriver::ex_syncdata(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    const std::optional<SyncDirection> direction = GetSyncDirection(*this, va_node.arguments[0]);

    if( !direction.has_value() )
    {
        issaerror(MessageType::Error, 94000, Logic::FunctionTable::GetFunctionName(va_node.function_code));
        return 0;
    }

    Symbol& symbol = NPT_Ref(va_node.arguments[1]);
    ISyncableDataRepository* syncable_data_repository;

    // because the cases may change during the sync, this object will ensure
    // that any cases currently loaded are properly updated post-sync
    std::unique_ptr<EngineDictionaryModifier> engine_dictionary_modifier;

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        EngineDictionary& engine_dictionary = assert_cast<EngineDictionary&>(symbol);
        syncable_data_repository = engine_dictionary.GetEngineDataRepository().GetDataRepository().GetSyncableDataRepository();

        if( *direction != SyncDirection::Put )
            engine_dictionary_modifier = EngineDictionaryModifier::Create(*this, engine_dictionary);
    }

    else
    {
        DICT& dict = assert_cast<DICT&>(symbol);
        syncable_data_repository = dict.GetDicX()->GetDataRepository().GetSyncableDataRepository();

        if( *direction != SyncDirection::Put )
            engine_dictionary_modifier = EngineDictionaryModifier::Create(*this, dict);
    }

    if( syncable_data_repository == nullptr )
    {
        issaerror(MessageType::Error, 100116, Logic::FunctionTable::GetFunctionName(va_node.function_code), symbol.GetName().c_str());
        return 0;
    }

    const std::string universe = EvaluateOptionalOrConstruct<std::string>(va_node.arguments[2]);
    bool success = false;

    try
    {
        std::exception_ptr sync_exception;

        if( engine_dictionary_modifier != nullptr )
            engine_dictionary_modifier->PrepareForModifications();

        try
        {
            if( GetSyncClient().SyncData(*direction, *syncable_data_repository, universe) == SyncClient::SyncResult::SYNC_OK )
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
        issaerror(MessageType::Error, 100114, exception.what());
    }

    return success;
}


double CIntDriver::ex_syncfile(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    const std::optional<SyncDirection> sync_direction = GetSyncDirection(*this, va_node.arguments[0]);

    if( !sync_direction.has_value() || *sync_direction == SyncDirection::Both )
    {
        issaerror(MessageType::Error, 94003);
        return 0;
    }

    auto evaluate_local_path = [&](std::string& path)
    {
        if( path.empty() )
        {
            path = PortableFunctions::PathGetDirectory(UTF8_TODO::GetUtf8(m_pEngineDriver->m_pPifFile->GetAppFName()));
        }

        else
        {
            MakeAbsolutePath(path);
        }
    };

    std::string from_path = EvaluateString(va_node.arguments[1]);
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

    return ( GetSyncClient().SyncFile(*sync_direction, std::move(from_path), std::move(to_path)) == SyncClient::SyncResult::SYNC_OK );
}


double CIntDriver::ex_syncserver(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    ASSERT(va_node.arguments[0] == 2); // the connection type is always 2 (Bluetooth) for now

    if( m_syncObjects == nullptr )
        GetSyncClient();

    ASSERT(m_syncObjects->login_accessor != nullptr && m_syncObjects->sync_listener != nullptr);

    std::shared_ptr<IBluetoothAdapter> bluetooth_adapter = m_syncObjects->login_accessor->GetBluetoothAdapter();

    // Bluetooth not supported on this device
    if( bluetooth_adapter == nullptr )
    {
        issaerror(MessageType::Error, 100146);
        return 0;
    }

    // Default file root is app directory
    std::string root_directory = ( va_node.arguments[1] != -1 ) ? EvaluatePath(va_node.arguments[1]) :
                                                                  UTF8_TODO::GetUtf8(GetFilePath(m_pEngineDriver->m_pPifFile->GetAppFName()));

    BluetoothObexServer bluetooth_server(std::move(bluetooth_adapter),
                                         std::make_unique<SyncObexHandler>(GetDeviceId(), std::move(root_directory), std::make_unique<SyncObexEngineAccessor>(*this, m_iExSymbol)),
                                         m_syncObjects->sync_listener);

    try
    {
        return bluetooth_server.run();
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100153, exception.what());
        return 0;
    }
}


double CIntDriver::ex_syncapp(int /*program_index*/)
{
    const std::string application_file_path = PortableFunctions::PathReplaceFileExtension(UTF8_TODO::GetUtf8(m_pEngineDriver->m_pPifFile->GetAppFName()), FileExtensions::BinaryEntryPen);
    const int64_t app_file_time_before = PortableFunctions::FileModifiedTime(application_file_path);

    const std::unique_ptr<ApplicationPackageManager> application_package_manager = CreateApplicationPackageManager();
    SyncClient& sync_client = GetSyncClient();

    if( application_package_manager != nullptr &&
        sync_client.UpdateApplication(*application_package_manager, UTF8_TODO::GetUtf8(m_pEngineDriver->m_pPifFile->GetAppFName())) == SyncClient::SyncResult::SYNC_OK )
    {
        if( PortableFunctions::FileModifiedTime(application_file_path) > app_file_time_before )
        {
            sync_client.Disconnect();

            const SharableString restart_message = MGF::GetMessageText(100152);
            ErrorMessage::Display(*restart_message);

#ifndef WIN_DESKTOP
            const CString& pff_file_path = m_pEngineDriver->m_pPifFile->GetPifFileName();
            PlatformInterface::GetInstance()->GetApplicationInterface()->ExecPff(UTF8_TODO::GetUtf8(pff_file_path));
#endif
            m_bStopProc = true;
            m_pEngineDriver->SetStopCode(1);
        }

        return 1;
    }

    return 0;
}


double CIntDriver::ex_syncmessage(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);
    ASSERT(va_node.arguments[0] == -1); // the type of message, for now, is ignored

    const SyncMessage sync_message(EvaluateSharableString(va_node.arguments[1]),
                                   EvaluateNullableSharableString(va_node.arguments[2]));

    const std::optional<JsonNode> response_json_node = GetSyncClient().SendSyncMessage(sync_message);

    if( !response_json_node.has_value() )
        return AssignStringNull();

    return AssignString(response_json_node->IsString() ? response_json_node->Get<SharableString>() :
                                                         response_json_node->GetNodeAsSharableString());
}


double CIntDriver::ex_syncparadata(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    const std::optional<SyncDirection> sync_direction = GetSyncDirection(*this, va_node.arguments[0]);

    if( !sync_direction.has_value() )
    {
        issaerror(MessageType::Error, 94000, Logic::FunctionTable::GetFunctionName(va_node.function_code));
        return 0;
    }

    try
    {
        if( !Paradata::Logger::IsOpen() )
            throw CSProException("A paradata log must be open before calling syncparadata.");

        if( GetSyncClient().SyncParadata(*sync_direction) == SyncClient::SyncResult::SYNC_OK )
            return 1;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 8295, exception.what());
    }

    return 0;
}


double CIntDriver::ex_synctime(const int program_index)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(program_index);

    Symbol& symbol = NPT_Ref(va_node.arguments[0]);
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
        issaerror(MessageType::Error, 100116, Logic::FunctionTable::GetFunctionName(va_node.function_code), symbol.GetName().c_str());
        return NOTAPPL;
    }

    const std::string device_identifier = EvaluateOptionalOrConstruct<std::string>(va_node.arguments[1]);
    const std::string case_uuid = EvaluateOptionalOrConstruct<std::string>(va_node.arguments[2]);

    try
    {
        std::optional<double> time = syncable_data_repository->GetSyncTime(device_identifier, case_uuid);

        if( time.has_value() )
            return *time;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100153, exception.what());
    }

    return NOTAPPL;
}


double CIntDriver::ex_getbluetoothname(int /*program_index*/)
{
    if( m_syncObjects == nullptr )
        GetSyncClient();

    ASSERT(m_syncObjects->login_accessor != nullptr);

    const std::shared_ptr<IBluetoothAdapter> bluetooth_adapter = m_syncObjects->login_accessor->GetBluetoothAdapter();

    return ( bluetooth_adapter != nullptr ) ? AssignString(bluetooth_adapter->GetName()) :
                                              AssignStringNull();
}


double CIntDriver::ex_setbluetoothname(const int program_index)
{
    const auto& fnn_node = GetNode<FNN_NODE>(program_index);
    const SharableString bluetooth_name = EvaluateSharableString(fnn_node.fn_expr[0]);

    if( m_syncObjects == nullptr )
        GetSyncClient();

    ASSERT(m_syncObjects->login_accessor != nullptr);

    const std::shared_ptr<IBluetoothAdapter> bluetooth_adapter = m_syncObjects->login_accessor->GetBluetoothAdapter();

    if( bluetooth_adapter == nullptr )
    {
        issaerror(MessageType::Error, 100146);
        return 0;
    }

    try
    {
        // only set the Bluetooth name when it differs from the current one
        if( bluetooth_adapter->GetName() != *bluetooth_name )
            bluetooth_adapter->SetName(*bluetooth_name);

        return 1;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100174, exception.what());
        return 0;
    }
}
