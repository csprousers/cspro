#include "StdAfx.h"
#include "DictPropertyGridValueManager.h"


DictPropertyGridValueManager::DictPropertyGridValueManager(CDDDoc* pDDDoc, DictValue& dict_value)
    :   DictPropertyGridBaseManager(pDDDoc, dict_value, _T("Value")),
        m_dictValue(dict_value)
{
}


void DictPropertyGridValueManager::PushUndo()
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


void DictPropertyGridValueManager::SetModified()
{
    DictPropertyGridBaseManager::SetModified();

    // update the values in any value sets linked to this value
    POSITION pos = m_pDDDoc->GetFirstViewPosition();
    CDDGView* pView = (CDDGView*)m_pDDDoc->GetNextView(pos);

    int level = pView->m_gridItem.GetLevel();
    int record = pView->m_gridItem.GetRecord();
    int item = pView->m_gridItem.GetItem();
    long row = pView->m_gridItem.GetCurrentRow();
    int vset = pView->m_gridItem.GetVSet(row);
    
    CDictItem* pItem = m_pDDDoc->GetDict()->GetLevel(level).GetRecord(record)->GetItem(item);
    DictValueSet& dict_value_set = pItem->GetValueSet(vset);

    // sync any linked value sets
    if( dict_value_set.IsLinkedValueSet() )
        m_pDDDoc->GetDict()->SyncLinkedValueSets(&dict_value_set);
}


void DictPropertyGridValueManager::SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    AddGeneralSection<DictValue>(property_grid_ctrl);

    // Appearance heading
    auto appearance_heading_property = new PropertyGrid::HeadingProperty(_T("Appearance"));
    property_grid_ctrl.AddProperty(appearance_heading_property);


    // Image property (string with file dialog)
    PropertyGrid::Type::ImageFilename initial_image_filename
    {
        m_dictValue.GetImageFilename(),
        m_pDDDoc->GetDict()->GetFullFileName()
    };

    appearance_heading_property->AddSubItem(
        PropertyGrid::PropertyBuilder<PropertyGrid::Type::ImageFilename>(_T("Image"),
                                                                         _T("The image to be displayed alongside the label in data entry applications."),
                                                                         initial_image_filename)
        .SetOnUpdate([&](const PropertyGrid::Type::ImageFilename& image_filename)
            {
                m_dictValue.SetImageFilename(image_filename.filename);
                m_pDDDoc->UpdateAllViews(nullptr);
            })
        .Create());


    // Text Color property (color with color picking dialog)
    appearance_heading_property->AddSubItem(
        PropertyGrid::PropertyBuilder<PortableColor>(_T("Text Color"),
                                                     _T("The text color of this value label when displayed in data entry applications."),
                                                     m_dictValue.GetTextColor())
        .SetOnUpdate([&](const PortableColor& text_color)
            {
                m_dictValue.SetTextColor(text_color);
                m_pDDDoc->UpdateAllViews(nullptr);
            })
        .Create());
}
