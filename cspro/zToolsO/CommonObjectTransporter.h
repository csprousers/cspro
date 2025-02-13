#pragma once

#include <zToolsO/ObjectTransporter.h>
#include <zUtilO/CommonStore.h>
#include <zAction/ActionInvoker.h>


class CommonObjectTransporter : public ObjectTransporter
{
protected:
    std::shared_ptr<CommonStore> OnGetCommonStore() override;

    std::shared_ptr<ActionInvoker::Runtime> OnGetActionInvokerRuntime() override;

    // overridable methods
    virtual bool DisableAccessTokenCheckForExternalCallers() const { return false; }

protected:
    std::shared_ptr<CommonStore> m_commonStore;
    std::shared_ptr<ActionInvoker::Runtime> m_actionInvokerRuntime;
};



// --------------------------------------------------------------------------
// inline implementations
// --------------------------------------------------------------------------

inline std::shared_ptr<CommonStore> CommonObjectTransporter::OnGetCommonStore()
{
    if( m_commonStore == nullptr )
    {
        auto common_store = std::make_unique<CommonStore>();

        if( !common_store->Open({ CommonStore::TableType::UserSettings }) )
            throw CSProException("There was an error opening the Common Store file.");

        m_commonStore = std::move(common_store);
    }

    return m_commonStore;
}


inline std::shared_ptr<ActionInvoker::Runtime> CommonObjectTransporter::OnGetActionInvokerRuntime()
{
    if( m_actionInvokerRuntime == nullptr )
    {
        m_actionInvokerRuntime = std::make_shared<ActionInvoker::Runtime>();

        if( DisableAccessTokenCheckForExternalCallers() )
            m_actionInvokerRuntime->DisableAccessTokenCheckForExternalCallers();
    }

    return m_actionInvokerRuntime;
}
