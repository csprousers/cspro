#include "StdAfx.h"
#include <engine/Engine.h>


FORM::CSymbolForm(std::wstring name, CDEForm* pCDEForm)
    :   Symbol(std::move(name), SymbolType::Form),
        m_pCDEForm(pCDEForm)
{
}


int FORM::GetSymGroup() const
{
    CDEGroup* pGroup = m_pCDEForm->GetGroup();
    int iSymGroup = ( pGroup != NULL ) ? pGroup->GetSymbol() : 0;
    return iSymGroup;
}
