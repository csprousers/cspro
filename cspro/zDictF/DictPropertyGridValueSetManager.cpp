#include "StdAfx.h"
#include "DictPropertyGridValueSetManager.h"


DictPropertyGridValueSetManager::DictPropertyGridValueSetManager(CDDDoc* pDDDoc, DictValueSet& dict_value_set)
    :   DictPropertyGridBaseManager(pDDDoc, dict_value_set, _T("Value Set")),
        m_dictValueSet(dict_value_set)
{
}


void DictPropertyGridValueSetManager::PushUndo()
{
    // push the entire item
    POSITION pos = m_pDDDoc->GetFirstViewPosition();
    CDDGView* pView = (CDDGView*)m_pDDDoc->GetNextView(pos);

    int level = pView->m_gridItem.GetLevel();
    int record = pView->m_gridItem.GetRecord();
    int item = pView->m_gridItem.GetItem();
    long row = pView->m_gridItem.GetCurrentRow();
    int vset = pView->m_gridItem.GetVSet(row);
    
    const CDictItem* dict_item = m_pDDDoc->GetDict()->GetLevel(level).GetRecord(record)->GetItem(item);

    m_pDDDoc->PushUndo(*dict_item, level, record, item, vset, row);
}


void DictPropertyGridValueSetManager::SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    AddGeneralSection<DictValueSet>(property_grid_ctrl);
}
