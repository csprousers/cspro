#include "StdAfx.h"
#include <engine/Engarea.h>
#include <engine/Engdrv.h>
#include <zEngineO/EngineAccessor.h>


std::shared_ptr<EngineAccessor> CEngineArea::CreateEngineAccessor(CEngineArea* pEngineArea)
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

        SystemMessageIssuer& ea_GetSystemMessageIssuer() override
        {
            ASSERT(m_pEngineArea->m_pEngineDriver != nullptr);
            return m_pEngineArea->m_pEngineDriver->GetSystemMessageIssuer();
        }

        std::shared_ptr<CommonStore> ea_CommonStore() override
        {
            ASSERT(m_pEngineArea->m_pEngineDriver != nullptr);
            return m_pEngineArea->m_pEngineDriver->GetCommonStore();
        }

        std::set<int>& ea_GetPersistentSymbolsNeedingResetSet() override
        {
            return m_pEngineArea->m_persistentSymbolsNeedingResetSet;
        }

    private:
        CEngineArea* m_pEngineArea;
        std::function<void(Symbol* pVarT, std::wstring value)> m_vartSetter;
    };

    return std::make_unique<ThisEngineAccessor>(pEngineArea);
}
