#include "stdafx.h"
#include "SyncRunnerActionInvoker.h"
#include "ISyncService.h"
#include "SyncMessage.h"
#include "SyncRunner.h"
#include <zParadataO/Logger.h>
#include <zAction/Caller.h>


// --------------------------------------------------------------------------
// ActionInvokerSyncListener
// --------------------------------------------------------------------------

class ActionInvokerSyncListener : public SyncListener
{
public:
    ActionInvokerSyncListener(const CancelFlag*& cancel_flag);

protected:
    void OnStart(const std::string& message_text) override;
    void OnFinish() override;
    void OnProgress(int reportable_progress, cs::cref_optional<std::string> message_text) override;
    bool IsCanceled() const override;
    void OnError(int message_number, const std::string& message_text) override;

private:
    const CancelFlag*& m_cancelFlag;
};


ActionInvokerSyncListener::ActionInvokerSyncListener(const CancelFlag*& cancel_flag)
    :   m_cancelFlag(cancel_flag)
{
}


void ActionInvokerSyncListener::OnStart(const std::string& /*message_text*/)
{
}


void ActionInvokerSyncListener::OnFinish()
{
}


void ActionInvokerSyncListener::OnProgress(int /*reportable_progress*/, cs::cref_optional<std::string> /*message_text*/)
{
}


bool ActionInvokerSyncListener::IsCanceled() const
{
    return ( m_cancelFlag != nullptr ) ? *m_cancelFlag :
                                         false;
}


void ActionInvokerSyncListener::OnError(int /*message_number*/, const std::string& message_text)
{
    throw CSProException(message_text);
}



// --------------------------------------------------------------------------
// ActionInvokerSyncRunnerImpl
// --------------------------------------------------------------------------

class ActionInvokerSyncRunnerImpl : public ActionInvokerSyncRunner
{
public:
    ActionInvokerSyncRunnerImpl();
    ~ActionInvokerSyncRunnerImpl();

    std::shared_ptr<const ConnectResponse> Connect(ActionInvoker::Caller& caller, const SyncConnectionString& sync_connection_string) override;
    void Disconnect(ActionInvoker::Caller& caller) override;

    DataSyncStatistics SyncData(ActionInvoker::Caller& caller, ISyncableDataRepository& syncable_data_repository,
                                SyncDirection sync_direction, const std::string& universe) override;

    std::optional<JsonNode> SendSyncMessage(ActionInvoker::Caller& caller, SharableString message_name, JsonNode message_value) override;

    void SyncParadata(ActionInvoker::Caller& caller, Paradata::Syncer& paradata_syncer, SyncDirection sync_direction) override;

private:
    // Executes the callback function using the supplied Action Invoker Caller,
    // which will be used to access the cancelation flag.
    // Exceptions will be properly formatted before thrown.
    template<typename CF>
    auto ExecuteWithCaller(ActionInvoker::Caller& caller, const CF& callback_function);

    void Disconnect();

private:
    const CancelFlag* m_cancelFlag;
    std::shared_ptr<ActionInvokerSyncListener> m_syncListener;
    SyncRunner m_syncRunner;
    std::shared_ptr<ISyncService> m_syncService;
    std::shared_ptr<ConnectResponse> m_connectResponse;
    std::unique_ptr<SyncRunner::ParadataLogger> m_paradataLogger;
};


ActionInvokerSyncRunnerImpl::ActionInvokerSyncRunnerImpl()
    :   m_cancelFlag(nullptr),
        m_syncListener(std::make_unique<ActionInvokerSyncListener>(m_cancelFlag)),
        m_syncRunner(m_syncListener)
{
}


ActionInvokerSyncRunnerImpl::~ActionInvokerSyncRunnerImpl()
{
    if( m_syncService != nullptr )
    {
        try
        {
            Disconnect();
        }
        catch(...) { }
    }
}


template<typename CF>
auto ActionInvokerSyncRunnerImpl::ExecuteWithCaller(ActionInvoker::Caller& caller, const CF& callback_function)
{
    ASSERT(m_cancelFlag == nullptr);
    const RAII::SetValueAndRestoreOnDestruction cancel_flag_modifier(m_cancelFlag, &caller.GetCancelFlag());

    try
    {
        return callback_function();
    }

    catch( const CSProException& exception )
    {
        m_syncListener->ReportError(exception);

        // ReportError will always throw an exception, so this throw is only to suppress a compiler error
        throw ProgrammingErrorException();
    }
}


std::shared_ptr<const ConnectResponse> ActionInvokerSyncRunnerImpl::Connect(ActionInvoker::Caller& caller, const SyncConnectionString& sync_connection_string)
{
    ASSERT(m_syncService == nullptr && m_connectResponse == nullptr);
    ASSERT(m_paradataLogger == nullptr);

    std::unique_ptr<SyncRunner::ParadataData> paradata_data;

    if( Paradata::Logger::IsOpen() )
    {
        paradata_data = std::make_unique<SyncRunner::ParadataData>(
            SyncRunner::ParadataData
            {
                GetDeviceId(),
                SyncRunner::ConnectionSource::ActionInvoker
            });
    }

    ExecuteWithCaller(caller,
        [&]()
        {
            std::tie(m_syncService, m_connectResponse) = m_syncRunner.Connect(sync_connection_string, paradata_data.get());

            if( paradata_data != nullptr )
            {
                m_paradataLogger = std::move(paradata_data->paradata_logger);
                ASSERT(m_paradataLogger != nullptr);
            }
        });

    return m_connectResponse;
}


void ActionInvokerSyncRunnerImpl::Disconnect(ActionInvoker::Caller& caller)
{
    ExecuteWithCaller(caller,
        [&]()
        {
            Disconnect();
        });
}


void ActionInvokerSyncRunnerImpl::Disconnect()
{
    ASSERT(m_syncService != nullptr);

    const std::shared_ptr<ISyncService> sync_service = std::move(m_syncService);
    m_connectResponse.reset();
    const std::unique_ptr<SyncRunner::ParadataLogger> paradata_logger = std::move(m_paradataLogger);

    m_syncRunner.Disconnect(*sync_service, paradata_logger.get());
}


DataSyncStatistics ActionInvokerSyncRunnerImpl::SyncData(ActionInvoker::Caller& caller, ISyncableDataRepository& syncable_data_repository,
                                                         const SyncDirection sync_direction, const std::string& universe)
{
    ASSERT(m_syncService != nullptr && m_connectResponse != nullptr);

    return ExecuteWithCaller(caller,
        [&]()
        {
            return m_syncRunner.SyncData(*m_syncService, *m_connectResponse, GetDeviceId(),
                                         syncable_data_repository, sync_direction, universe,
                                         m_paradataLogger.get());
        });
}


std::optional<JsonNode> ActionInvokerSyncRunnerImpl::SendSyncMessage(ActionInvoker::Caller& caller, SharableString message_name, JsonNode message_value)
{
    ASSERT(m_syncService != nullptr);

    const SyncMessage sync_message(std::move(message_name), std::move(message_value));

    return ExecuteWithCaller(caller,
        [&]()
        {
            return m_syncRunner.SendSyncMessage(*m_syncService, GetDeviceId(), sync_message, m_paradataLogger.get());
        });
}


void ActionInvokerSyncRunnerImpl::SyncParadata(ActionInvoker::Caller& caller, Paradata::Syncer& paradata_syncer, const SyncDirection sync_direction)
{
    ASSERT(m_syncService != nullptr);

    ExecuteWithCaller(caller,
        [&]()
        {
            m_syncRunner.SyncParadata(*m_syncService, paradata_syncer, sync_direction, m_paradataLogger.get());
        });
}



// --------------------------------------------------------------------------
// ActionInvokerSyncRunner
// --------------------------------------------------------------------------

std::unique_ptr<ActionInvokerSyncRunner> ActionInvokerSyncRunner::Instantiate()
{
    return std::make_unique<ActionInvokerSyncRunnerImpl>();
}
