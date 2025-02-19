#pragma once

#include <zNetwork/SyncListener.h>


// --------------------------------------------------------------------------
// WrapperSyncListener
//
// This class overrides all of SyncListener's virtual methods, calling the
// parent sync listener's methods. It is intended for sync listeners that
// want to override a specific method while maintaining the behavior of the
// a parent sync listener.
// --------------------------------------------------------------------------

class WrapperSyncListener : public SyncListener
{
public:
    WrapperSyncListener(std::shared_ptr<SyncListener> sync_listener);

protected:
    void SetLastCaseSynced(const Case& data_case, bool received) override;
    bool IsCanceled() const override;
    void OnStart(const std::string& message_text) override;
    void OnFinish() override;
    void OnProgress(int reportable_progress, cs::cref_optional<std::string> message_text) override;
    void OnError(int message_number, const std::string& message_text) override;

private:
    std::shared_ptr<SyncListener> m_syncListener;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline WrapperSyncListener::WrapperSyncListener(std::shared_ptr<SyncListener> sync_listener)
    :   m_syncListener(std::move(sync_listener))
{
}


inline void WrapperSyncListener::SetLastCaseSynced(const Case& data_case, const bool received)
{
    if( m_syncListener != nullptr )
        m_syncListener->SetLastCaseSynced(data_case, received);
}


inline bool WrapperSyncListener::IsCanceled() const
{
    return ( m_syncListener != nullptr ) ? m_syncListener->IsCanceled() :
                                           false;
}


inline void WrapperSyncListener::OnStart(const std::string& message_text)
{
    if( m_syncListener != nullptr )
        m_syncListener->OnStart(message_text);
}


inline void WrapperSyncListener::OnFinish()
{
    if( m_syncListener != nullptr )
        m_syncListener->OnFinish();
}


inline void WrapperSyncListener::OnProgress(const int reportable_progress, cs::cref_optional<std::string> message_text)
{
    if( m_syncListener != nullptr )
        m_syncListener->OnProgress(reportable_progress, std::move(message_text));
}


inline void WrapperSyncListener::OnError(const int message_number, const std::string& message_text)
{
    if( m_syncListener != nullptr )
        m_syncListener->OnError(message_number, message_text);
}
