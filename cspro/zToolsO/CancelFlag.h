#pragma once

#include <zToolsO/RaiiHelpers.h>


// --------------------------------------------------------------------------
// CancelFlag
//
// This class wraps a boolean that indicates if an operation is canceled.
// RAII listeners can be added that are notified of the cancelation.
// --------------------------------------------------------------------------

class CancelFlag
{
public:
    CancelFlag()
        :   m_cancelFlag(false)
    {
    }

    operator bool() const
    {
        return m_cancelFlag;
    }

    CancelFlag& operator=(bool canceled)
    {
        m_cancelFlag = canceled;

        if( canceled )
            NotifyListeners();

        return *this;
    }

    class ListenerHolder : public RAII::PushOnVectorAndPopOnDestruction<std::function<void()>>
    {
        using ParentType = RAII::PushOnVectorAndPopOnDestruction<std::function<void()>>;
        using ParentType::ParentType;
    };

    [[nodiscard]] ListenerHolder AddListener(std::function<void()> listener)
    {
        ASSERT(listener);
        return ListenerHolder(m_listeners, std::move(listener));
    }

private:
    void NotifyListeners()
    {
        ASSERT(m_cancelFlag);

        // notify the listeners (in order of last added)
        const auto& listener_cend = m_listeners.crend();

        for( auto listener_itr = m_listeners.crbegin(); listener_itr != listener_cend; ++listener_itr )
            (*listener_itr)();
    }

private:
    volatile bool m_cancelFlag;
    std::vector<std::function<void()>> m_listeners;
};
