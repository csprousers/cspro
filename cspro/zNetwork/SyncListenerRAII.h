#pragma once

#include <zNetwork/SyncListener.h>


// --------------------------------------------------------------------------
// SyncListenerCloser
//
// Helper class to close the sync listener when a method exits, even when
// exceptions are thrown. The second constructor also calls Start.
// --------------------------------------------------------------------------

class SyncListenerCloser
{
public:
    SyncListenerCloser(SyncListener* sync_listener);

    template<typename... Args>
    SyncListenerCloser(SyncListener* sync_listener, int message_number, Args const&... args);

    ~SyncListenerCloser();

private:
    SyncListener* m_syncListener;
};


// --------------------------------------------------------------------------
// SyncListenerServerSaverAndCloser
//
// Helper class to save a server's sync listener, set it to a new one, and
// restore it when when a method exits, even when exceptions are thrown.
// As a subclass of SyncListenerCloser, it also closes the sync listener.
// --------------------------------------------------------------------------

template<typename ServerType>
class SyncListenerServerSaverAndCloser : public SyncListenerCloser
{
private:
    using ConstructionParams = std::tuple<std::optional<std::shared_ptr<SyncListener>>, std::shared_ptr<SyncListener>>;

    template<typename... Args>
    SyncListenerServerSaverAndCloser(ServerType* server, ConstructionParams params, Args const&... args);

public:
    SyncListenerServerSaverAndCloser(ServerType* server, std::shared_ptr<SyncListener> new_sync_listener);

    template<typename... Args>
    SyncListenerServerSaverAndCloser(ServerType* server, std::shared_ptr<SyncListener> new_sync_listener, int message_number, Args const&... args);

    ~SyncListenerServerSaverAndCloser();

private:
    static ConstructionParams GetConstructionParams(ServerType* server, std::shared_ptr<SyncListener> new_sync_listener);

private:
    ServerType* m_server;
    std::optional<std::shared_ptr<SyncListener>> m_serverSavedSyncListener;
    std::shared_ptr<SyncListener> m_newSyncListener;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline SyncListenerCloser::SyncListenerCloser(SyncListener* const sync_listener)
    :   m_syncListener(sync_listener)
{
}


template<typename... Args>
SyncListenerCloser::SyncListenerCloser(SyncListener* const sync_listener, const int message_number, Args const&... args)
    :   SyncListenerCloser(sync_listener)
{
    if( m_syncListener != nullptr )
        m_syncListener->Start(message_number, args...);
}


inline SyncListenerCloser::~SyncListenerCloser()
{
    if( m_syncListener != nullptr )
        m_syncListener->Finish();
}


template<typename ServerType>
template<typename... Args>
SyncListenerServerSaverAndCloser<ServerType>::SyncListenerServerSaverAndCloser(ServerType* const server, ConstructionParams params, Args const&... args)
    :   SyncListenerCloser(std::get<1>(params).get(), args...),
        m_server(server),
        m_serverSavedSyncListener(std::move(std::get<0>(params))),
        m_newSyncListener(std::move(std::get<1>(params)))
{
}


template<typename ServerType>
SyncListenerServerSaverAndCloser<ServerType>::SyncListenerServerSaverAndCloser(ServerType* const server, std::shared_ptr<SyncListener> new_sync_listener)
    :   SyncListenerServerSaverAndCloser(server, GetConstructionParams(server, std::move(new_sync_listener)))
{
}


template<typename ServerType>
template<typename... Args>
SyncListenerServerSaverAndCloser<ServerType>::SyncListenerServerSaverAndCloser(ServerType* const server, std::shared_ptr<SyncListener> new_sync_listener,
                                                                               const int message_number, Args const&... args)
    :   SyncListenerServerSaverAndCloser(server, GetConstructionParams(server, std::move(new_sync_listener)), message_number, args...)
{
}


template<typename ServerType>
SyncListenerServerSaverAndCloser<ServerType>::~SyncListenerServerSaverAndCloser()
{
    if( m_serverSavedSyncListener.has_value() && m_server != nullptr )
        m_server->SetSyncListener(std::move(*m_serverSavedSyncListener));
}


template<typename ServerType>
typename SyncListenerServerSaverAndCloser<ServerType>::ConstructionParams
SyncListenerServerSaverAndCloser<ServerType>::GetConstructionParams(ServerType* const server, std::shared_ptr<SyncListener> new_sync_listener)
{
    std::optional<std::shared_ptr<SyncListener>> server_saved_sync_listener;

    if( server != nullptr )
    {
        server_saved_sync_listener = server->GetSharedSyncListener();

        // only modify the server's sync listener when necessary
        if( server != nullptr && server_saved_sync_listener->get() != new_sync_listener.get() )
        {
            server->SetSyncListener(new_sync_listener);
        }

        else
        {
            server_saved_sync_listener.reset();
        }
    }

    return { std::move(server_saved_sync_listener), std::move(new_sync_listener) };
}
