#include "StandardSystemIncludes.h"
#include "INTERPRE.H"
#include <zLogicO/SpecialFunction.h>
#include <zEngineO/EngineDictionary.h>
#include <zEngineO/Versioning.h>
#include <zPlatformO/PlatformInterface.h>
#include <zMessageO/Messages.h>
#include <ZBRIDGEO/npff.h>
#include <zSyncO/BluetoothDeviceInfo.h>
#include <zSyncO/BluetoothObexServer.h>
#include <zSyncO/IChooseBluetoothDeviceDialog.h>
#include <zSyncO/IDataRepositoryRetriever.h>
#include <zSyncO/IDropboxAuthDialog.h>
#include <zSyncO/ILoginDialog.h>
#include <zSyncO/SyncClient.h>
#include <zSyncO/SyncObexHandler.h>
#include <zParadataO/Logger.h>

#ifdef WIN_DESKTOP
#include <zSyncF/ChooseBluetoothDeviceDialog.h>
#include <zSyncF/DropboxAuthDialog.h>
#include <zSyncF/LoginDialog.h>
#include <zSyncO/WinBluetoothAdapter.h>
#endif


namespace
{
    // Translate dictionary name to repository for sync
    class DataRepositoryRetriever : public IDataRepositoryRetriever
    {
    public:
        DataRepositoryRetriever(CEngineArea* pEngineArea)
            :   m_pEngineArea(pEngineArea)
        {
        }

        DataRepository* get(const std::wstring& dictionary_name)
        {
            int dictionary_symbol_index = m_pEngineArea->SymbolTableSearch(dictionary_name, { SymbolType::Pre80Dictionary });

            if( dictionary_symbol_index != 0 )
            {
                DICT* pDicT = DPT(dictionary_symbol_index);
                DICX* pDicX = pDicT->GetDicX();
                return &pDicX->GetDataRepository().GetRealRepository();
            }

            return nullptr;
        }

    private:
        const Logic::SymbolTable& GetSymbolTable() const { return m_pEngineArea->GetSymbolTable(); }

    private:
        CEngineArea* m_pEngineArea;
    };

#ifndef WIN_DESKTOP
    class CLoginDialog : public ILoginDialog
    {
        std::optional<std::tuple<CString, CString>> Show(const CString& server, bool show_invalid_error) override
        {
            auto credentials = PlatformInterface::GetInstance()->GetApplicationInterface()->ShowLoginDialog(server, show_invalid_error);
            if (credentials)
                return std::make_tuple(credentials->username, credentials->password);
            else
                return {};
        }
    };

    class DropboxAuthDialog : public IDropboxAuthDialog
    {
        CString Show(CString clientId) override
        {
            return PlatformInterface::GetInstance()->GetApplicationInterface()->AuthorizeDropbox(clientId);
        }
    };

    class ChooseBluetoothDeviceDialog : public IChooseBluetoothDeviceDialog
    {
    public:
        explicit ChooseBluetoothDeviceDialog(IBluetoothAdapter*)
        {
        }

        bool Show(BluetoothDeviceInfo& deviceInfo) override
        {
            auto result = PlatformInterface::GetInstance()->GetApplicationInterface()->ChooseBluetoothDevice(OBEX_SYNC_SERVICE_UUID);
            if (result) {
                deviceInfo = *result;
                return true;
            } else {
                return false;
            }
        }
    };

#endif

    struct SyncEngineFunctionCaller : public ISyncEngineFunctionCaller
    {
        SyncEngineFunctionCaller(CIntDriver* int_driver, int field_symbol_index)
            :   m_intDriver(int_driver),
                m_fieldSymbolIndex(field_symbol_index)
        {
        }

        std::optional<CString> onSyncMessage(const CString& message_key, const CString& message_value)
        {
            if( !m_intDriver->HasSpecialFunction(SpecialFunction::OnSyncMessage) )
                return std::nullopt;

            int message_response = (int)m_intDriver->ExecSpecialFunction(m_fieldSymbolIndex, SpecialFunction::OnSyncMessage, { CS2WS(message_key), CS2WS(message_value) });
            return m_intDriver->CharacterObjectToString<CString>(message_response);
        }

    private:
        CIntDriver* m_intDriver;
        int m_fieldSymbolIndex;
    };


    std::optional<SyncDirection> GetSyncDirection(CIntDriver* pIntDriver, int expression)
    {
        if( expression >= static_cast<int>(SyncDirection::Put) &&
            expression <= static_cast<int>(SyncDirection::Both) )
        {
            return static_cast<SyncDirection>(expression);
        }

        else if( expression < 0 )
        {
            std::wstring direction_string = pIntDriver->EvalAlphaExpr(-1 * expression);

            return SO::EqualsNoCase(direction_string, _T("PUT"))  ? std::make_optional(SyncDirection::Put) :
                   SO::EqualsNoCase(direction_string, _T("GET"))  ? std::make_optional(SyncDirection::Get) :
                   SO::EqualsNoCase(direction_string, _T("BOTH")) ? std::make_optional(SyncDirection::Both) :
                                                                    std::nullopt;
        }

        else
        {
            return std::nullopt;
        }
    }
}


double CIntDriver::exsyncconnect(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);

    int connection_type = va_node.arguments[0];

    if( Versioning::PredatesCompiledLogicVersion(Serializer::Iteration_8_0_000_1) )
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

    switch (connection_type) {
        // CSWeb
        case 1:
        {
            CString csHost = SO::Trim(EvalAlphaExpr(va_node.arguments[1]));
            CString csUsername, csPassword;
            if (va_node.arguments[2] >= 0) {
                // Username and password specified
                csUsername = EvalAlphaExpr<CString>(va_node.arguments[2]);
                csPassword = EvalAlphaExpr<CString>(va_node.arguments[3]);
                return m_pSyncClient->connectWeb(csHost, csUsername, csPassword) == SyncClient::SyncResult::SYNC_OK;
            } else {
                CLoginDialog loginDialog;
                return m_pSyncClient->connectWeb(csHost, &loginDialog, m_pSyncCredentialStore) == SyncClient::SyncResult::SYNC_OK;
            }
        }

        // Bluetooth sync
        case 2:
        {
            BluetoothDeviceInfo deviceInfo;
            if (va_node.arguments[1] >= 0) {
                deviceInfo.csName = CString(SO::Trim(EvalAlphaExpr(va_node.arguments[1])));
            }
            if (deviceInfo.csName.IsEmpty()) {
                // No device specified - let user choose from nearby devices
                ChooseBluetoothDeviceDialog chooseDeviceDlg(m_pBluetoothAdapter);
                return m_pSyncClient->connectBluetooth(&chooseDeviceDlg) == SyncClient::SyncResult::SYNC_OK;
            }
            else {
                return m_pSyncClient->connectBluetooth(deviceInfo) == SyncClient::SyncResult::SYNC_OK;
            }
        }

        // Dropbox sync
        case 3:
        {
            DropboxAuthDialog authDialog;
            return m_pSyncClient->connectDropbox(&authDialog, m_pSyncCredentialStore) == SyncClient::SyncResult::SYNC_OK;
        }

        // FTP
        case 4:
        {
            CString csHost = SO::Trim(EvalAlphaExpr(va_node.arguments[1]));
            CString csUsername, csPassword;
            if (va_node.arguments[2] >= 0) {
                // Username and password specified
                csUsername = EvalAlphaExpr<CString>(va_node.arguments[2]);
                csPassword = EvalAlphaExpr<CString>(va_node.arguments[3]);
                return m_pSyncClient->connectFtp(csHost, csUsername, csPassword) == SyncClient::SyncResult::SYNC_OK;
            } else {
                CLoginDialog loginDialog;
                return m_pSyncClient->connectFtp(csHost, &loginDialog, m_pSyncCredentialStore) == SyncClient::SyncResult::SYNC_OK;
            }
        }

        // Local Dropbox
        case 5:
        {
            return m_pSyncClient->connectDropboxLocal() == SyncClient::SyncResult::SYNC_OK;
        }

        // Local filesystem
        case 6:
        {
            CString csPath = EvalAlphaExpr<CString>(va_node.arguments[1]);
            if( csPath.Find(_T("file:/")) < 0 )
                MakeFullPathFileName(csPath);
            return m_pSyncClient->connectLocalFileSystem(csPath) == SyncClient::SyncResult::SYNC_OK;
        }
    }

    return ReturnProgrammingError(0);
}


double CIntDriver::exsyncdisconnect(int /*iExpr*/)
{
    return m_pSyncClient->disconnect() == SyncClient::SyncResult::SYNC_OK;
}


double CIntDriver::exsyncdata(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);

    std::optional<SyncDirection> direction = GetSyncDirection(this, va_node.arguments[0]);

    if( !direction.has_value() )
    {
        issaerror(MessageType::Error, 94000, Logic::FunctionTable::GetFunctionName(va_node.function_code));
        return 0;
    }

    Symbol& symbol = NPT_Ref(va_node.arguments[1]);
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
        return 0;
    }

    std::wstring universe = ValueOrDefault(EvaluateOptionalStringExpression(va_node.arguments[2]));

    return ( m_pSyncClient->syncData(*direction, *syncable_data_repository, WS2CS(universe)) == SyncClient::SyncResult::SYNC_OK );
}


double CIntDriver::exsyncfile(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);

    std::optional<SyncDirection> sync_direction = GetSyncDirection(this, va_node.arguments[0]);

    if( !sync_direction.has_value() || *sync_direction == SyncDirection::Both )
    {
        issaerror(MessageType::Error, 94003);
        return 0;
    }

    CString from = EvalAlphaExpr<CString>(va_node.arguments[1]);
    CString to = ( va_node.arguments[2] >= 0 ) ? EvalAlphaExpr<CString>(va_node.arguments[2]) : CString();

    CString fileRoot;
    fileRoot.Format(_T("%s%s"), GetFilePath(m_pEngineDriver->m_pPifFile->GetAppFName()).GetString(), PATH_STRING);

    return m_pSyncClient->syncFile(*sync_direction, from, to, fileRoot) == SyncClient::SyncResult::SYNC_OK;
}


double CIntDriver::exsyncserver(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    ASSERT(va_node.arguments[0] == 2); // the connection type is always 2 (Bluetooth) for now

    if (!m_pBluetoothAdapter) {
        // Bluetooth not supported on this device
        issaerror(MessageType::Error, 100146);
        return 0;
    }

    CString csFileRoot;
    if (va_node.arguments[1] > 0) {
        csFileRoot = EvalAlphaExpr<CString>(va_node.arguments[1]);
        csFileRoot = PortableFunctions::PathToNativeSlash(csFileRoot);
        if (PathIsRelative(csFileRoot)) {
            csFileRoot = PortableFunctions::PathAppendToPath(GetFilePath(m_pEngineDriver->m_pPifFile->GetAppFName()), csFileRoot);
        }
        TCHAR canonPath[MAX_PATH];
        if (!PathCanonicalize(canonPath, csFileRoot)) {
            issaerror(MessageType::Error, 100113, csFileRoot.GetString());
            return 0;
        }
        csFileRoot = canonPath;
    } else {
        // Default file root is app directory
        csFileRoot = GetFilePath(m_pEngineDriver->m_pPifFile->GetAppFName());
    }

    auto sync_engine_function_caller = std::make_unique<SyncEngineFunctionCaller>(this, m_iExSymbol);

    DataRepositoryRetriever repoRetriever(m_pEngineArea);
    SyncObexHandler obexHandler(GetDeviceId(), &repoRetriever, csFileRoot, sync_engine_function_caller.get());
    BluetoothObexServer bluetoothServer(GetDeviceId(), m_pBluetoothAdapter, &obexHandler);
    bluetoothServer.setListener(m_pSyncListener);

    try
    {
        return bluetoothServer.run();
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100153, exception.GetErrorMessage().c_str());
        return 0;
    }
}


double CIntDriver::exsyncapp(int /*iExpr*/)
{
#ifdef ANDROID
    auto app_path = PortableFunctions::PathRemoveFileExtension(m_pEngineDriver->m_pPifFile->GetAppFName()) + FileExtensions::WithDot::BinaryEntryPen;
    auto app_file_time_before = PortableFunctions::FileModifiedTime(app_path);

    if (m_pSyncClient->updateApplication(m_pEngineDriver->m_pPifFile->GetAppFName()) == SyncClient::SyncResult::SYNC_OK) {

        if (difftime(PortableFunctions::FileModifiedTime(app_path), app_file_time_before) > 0) {

            m_pSyncClient->disconnect();

            const std::wstring& restart_message = MGF::GetMessageText(100152);
            PlatformInterface::GetInstance()->GetApplicationInterface()->ShowModalDialog(_T(""), restart_message, MB_OK);

            const CString& pff_name = m_pEngineDriver->m_pPifFile->GetPifFileName();
            PlatformInterface::GetInstance()->GetApplicationInterface()->ExecPff(CS2WS(pff_name));
            m_bStopProc = true;
            m_pEngineDriver->SetStopCode(1);
        }

        return 1;
    }
#endif

    return 0;
}


double CIntDriver::exsyncmessage(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);
    ASSERT(va_node.arguments[0] == -1); // the type of message, for now, is ignored

    CString message_key = EvalAlphaExpr<CString>(va_node.arguments[1]);
    CString message_value;

    if( va_node.arguments[2] >= 0 )
        message_value = EvalAlphaExpr<CString>(va_node.arguments[2]);

    return AssignAlphaValue(m_pSyncClient->syncMessage(message_key, message_value));
}


double CIntDriver::exsyncparadata(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);

    std::optional<SyncDirection> sync_direction = GetSyncDirection(this, va_node.arguments[0]);

    if( !sync_direction.has_value() )
    {
        issaerror(MessageType::Error, 94000, Logic::FunctionTable::GetFunctionName(va_node.function_code));
        return 0;
    }

    try
    {
        if( !Paradata::Logger::IsOpen() )
            throw CSProException("A paradata log must be open before calling syncparadata.");

        if( m_pSyncClient->syncParadata(*sync_direction) == SyncClient::SyncResult::SYNC_OK )
            return 1;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 8295, exception.GetErrorMessage().c_str());
    }

    return 0;
}


double CIntDriver::exsynctime(int iExpr)
{
    const auto& va_node = GetNode<Nodes::VariableArguments>(iExpr);

    const Symbol& symbol = NPT_Ref(va_node.arguments[0]);
    const ISyncableDataRepository* syncable_data_repository;

    if( symbol.IsA(SymbolType::Dictionary) )
    {
        syncable_data_repository = assert_cast<const EngineDictionary&>(symbol).GetEngineDataRepository().GetDataRepository().GetSyncableDataRepository();
    }

    else
    {
        syncable_data_repository = assert_cast<const DICT&>(symbol).GetDicX()->GetDataRepository().GetSyncableDataRepository();
    }

    if( syncable_data_repository == nullptr )
    {
        issaerror(MessageType::Error, 100116, Logic::FunctionTable::GetFunctionName(va_node.function_code), symbol.GetName().c_str());
        return NOTAPPL;
    }

    std::wstring device_identifier = ValueOrDefault(EvaluateOptionalStringExpression(va_node.arguments[1]));
    std::wstring case_uuid = ValueOrDefault(EvaluateOptionalStringExpression(va_node.arguments[2]));

    try
    {
        std::optional<double> time = syncable_data_repository->GetSyncTime(device_identifier, case_uuid);

        if( time.has_value() )
            return *time;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100153, exception.GetErrorMessage().c_str());
    }

    return NOTAPPL;
}



double CIntDriver::exgetbluetoothname(int /*iExpr*/)
{
    return ( m_pBluetoothAdapter != nullptr ) ? AssignAlphaValue(m_pBluetoothAdapter->getName()) :
                                                AssignBlankAlphaValue();
}


double CIntDriver::exsetbluetoothname(int iExpr)
{
    const auto& fnn_node = GetNode<FNN_NODE>(iExpr);
    std::wstring bluetooth_name = EvalAlphaExpr(fnn_node.fn_expr[0]);

    if( m_pBluetoothAdapter == nullptr )
    {
        issaerror(MessageType::Error, 100146);
        return 0;
    }

    try
    {
        // only set the Bluetooth name when it differs from the current one
        if( m_pBluetoothAdapter->getName() != bluetooth_name )
            m_pBluetoothAdapter->setName(WS2CS(bluetooth_name));

        return 1;
    }

    catch( const CSProException& exception )
    {
        issaerror(MessageType::Error, 100174, exception.GetErrorMessage().c_str());
        return 0;
    }
}
