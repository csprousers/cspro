#pragma once

#include <engine/Engdrv.h>
#include <engine/INTERPRE.H>
#include <engine/InterpreterAccessor.h>
#include <zToolsO/CommonObjectTransporter.h>
#include <zSyncO/SyncRunnerActionInvoker.h>


class EngineObjectTransporter : public CommonObjectTransporter
{
public:
    EngineObjectTransporter(const CEngineArea* engine_area);

protected:
    std::shared_ptr<CommonStore> OnGetCommonStore() override;
    std::shared_ptr<ActionInvoker::Runtime> OnGetActionInvokerRuntime() override;
    std::shared_ptr<InterpreterAccessor> OnGetInterpreterAccessor() override;
    std::unique_ptr<ActionInvokerSyncRunner> OnCreateActionInvokerSyncRunner() const override;

private:
    bool IsEngineDriverAvailable() const { return ( m_engineArea != nullptr && m_engineArea->m_pEngineDriver != nullptr ); }

private:
    const CEngineArea* m_engineArea;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline EngineObjectTransporter::EngineObjectTransporter(const CEngineArea* engine_area)
    :   m_engineArea(engine_area)
{
}


inline std::shared_ptr<CommonStore> EngineObjectTransporter::OnGetCommonStore()
{
    if( m_commonStore == nullptr )
    {
        if( !IsEngineDriverAvailable() )
            return CommonObjectTransporter::OnGetCommonStore();

        m_commonStore = m_engineArea->m_engineData->GetCommonStore();
    }

    return m_commonStore;
}


inline std::shared_ptr<ActionInvoker::Runtime> EngineObjectTransporter::OnGetActionInvokerRuntime()
{
    if( m_actionInvokerRuntime == nullptr )
    {
        CommonObjectTransporter::OnGetActionInvokerRuntime();
        ASSERT(m_actionInvokerRuntime != nullptr);

        if( IsEngineDriverAvailable() && m_engineArea->m_pEngineDriver->m_pIntDriver != nullptr )
            m_engineArea->m_pEngineDriver->m_pIntDriver->SetActionInvokerRuntime(m_actionInvokerRuntime);
    }

    return m_actionInvokerRuntime;
}


inline std::shared_ptr<InterpreterAccessor> EngineObjectTransporter::OnGetInterpreterAccessor()
{
    if( IsEngineDriverAvailable() )
        return m_engineArea->m_pEngineDriver->CreateInterpreterAccessor();

    return nullptr;
}


inline std::unique_ptr<ActionInvokerSyncRunner> EngineObjectTransporter::OnCreateActionInvokerSyncRunner() const
{
    return ActionInvokerSyncRunner::Instantiate();
}
