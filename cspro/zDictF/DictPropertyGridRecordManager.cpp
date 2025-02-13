#include "StdAfx.h"
#include "DictPropertyGridRecordManager.h"


DictPropertyGridRecordManager::DictPropertyGridRecordManager(CDDDoc* pDDDoc, CDictRecord& dict_record)
    :   DictPropertyGridBaseManager(pDDDoc, dict_record, _T("Record")),
        m_dictRecord(dict_record)
{
}

void DictPropertyGridRecordManager::PushUndo()
{
    // the record number is not set in the level editor so derive the record number from the grid
    POSITION pos = m_pDDDoc->GetFirstViewPosition();
    CDDGView* pView = (CDDGView*)m_pDDDoc->GetNextView(pos);
    int record = pView->m_gridLevel.GetCurrentRow() - pView->m_gridLevel.GetFirstRow();

    m_pDDDoc->PushUndo(m_dictRecord, m_pDDDoc->GetLevel(), record);
}


void DictPropertyGridRecordManager::SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    AddGeneralSection<CDictRecord>(property_grid_ctrl);

    // Record heading
    auto record_heading_property = new PropertyGrid::HeadingProperty(_T("Record"));
    property_grid_ctrl.AddProperty(record_heading_property);

    // Occurrence Labels (read only string with editing dialog)
    if( m_dictRecord.GetMaxRecs() > 1 )
        record_heading_property->AddSubItem(CreateOccurrenceLabelsProperty<CDictRecord>());
}
