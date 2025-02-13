#include "StdAfx.h"
#include "DictPropertyGridLevelManager.h"


DictPropertyGridLevelManager::DictPropertyGridLevelManager(CDDDoc* pDDDoc, DictLevel& dict_level)
    :   DictPropertyGridBaseManager(pDDDoc, dict_level, _T("Level")),
        m_dictLevel(dict_level)
{
}

void DictPropertyGridLevelManager::PushUndo()
{
    m_pDDDoc->PushUndo(m_dictLevel, m_pDDDoc->GetLevel());
}


void DictPropertyGridLevelManager::SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    AddGeneralSection<DictLevel>(property_grid_ctrl);
}
