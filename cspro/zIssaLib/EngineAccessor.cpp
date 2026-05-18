#include "StdAfx.h"
#include <engine/Engarea.h>
#include <engine/Engdrv.h>
#include <zEngineO/EngineAccessor.h>


std::shared_ptr<EngineAccessor> CEngineArea::CreateEngineAccessor(CEngineArea* const pEngineArea)
{
    class ThisEngineAccessor : public EngineAccessor
    {
    public:
        ThisEngineAccessor(CEngineArea* pEngineArea)
            :   m_pEngineArea(pEngineArea)
        {
        }

        void ea_SetVarTValueSetter(std::function<void(Symbol* pVarT, std::wstring)> setter) override
        {
            m_vartSetter = std::move(setter);
        }

        void ea_SetVarTValue(Symbol* symbol, std::wstring value) override
        {
            if( m_vartSetter )
                m_vartSetter(symbol, std::move(value));
        }

        std::shared_ptr<SystemMessageIssuer> ea_GetSharedSystemMessageIssuer() override
        {
            ASSERT(m_pEngineArea->m_pEngineDriver != nullptr &&
                   m_pEngineArea->m_pEngineDriver->GetSharedSystemMessageIssuer() != nullptr);

            return m_pEngineArea->m_pEngineDriver->GetSharedSystemMessageIssuer();
        }

        std::set<int>& ea_GetPersistentSymbolsNeedingResetSet() override
        {
            return m_pEngineArea->m_persistentSymbolsNeedingResetSet;
        }

        Listing::Lister* ea_GetLister() override
        {
            ASSERT(m_pEngineArea->m_pEngineDriver != nullptr);
            return m_pEngineArea->m_pEngineDriver->GetLister();
        }

    private:
        CEngineArea* m_pEngineArea;
        std::function<void(Symbol* pVarT, std::wstring value)> m_vartSetter;
    };

    return std::make_unique<ThisEngineAccessor>(pEngineArea);
}
