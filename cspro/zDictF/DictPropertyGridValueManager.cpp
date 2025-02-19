#include "StdAfx.h"
#include "DictPropertyGridValueManager.h"


DictPropertyGridValueManager::DictPropertyGridValueManager(CDDDoc* const pDDDoc, DictValue& dict_value)
    :   DictPropertyGridBaseManager(pDDDoc, dict_value, L"Value"),
        m_dictValue(dict_value)
{
}


void DictPropertyGridValueManager::PushUndo()
{
    // push the entire item
    POSITION pos = m_pDDDoc->GetFirstViewPosition();
    CDDGView* const pView = assert_cast<CDDGView*>(m_pDDDoc->GetNextView(pos));

    const int level = pView->m_gridItem.GetLevel();
    const int record = pView->m_gridItem.GetRecord();
    const int item = pView->m_gridItem.GetItem();
    const long row = pView->m_gridItem.GetCurrentRow();
    const int vset = pView->m_gridItem.GetVSet(row);

    const CDictItem* const dict_item = m_pDDDoc->GetDict()->GetLevel(level).GetRecord(record)->GetItem(item);

    m_pDDDoc->PushUndo(*dict_item, level, record, item, vset, row);
}


void DictPropertyGridValueManager::SetModified()
{
    DictPropertyGridBaseManager::SetModified();

    // update the values in any value sets linked to this value
    POSITION pos = m_pDDDoc->GetFirstViewPosition();
    CDDGView* const pView = assert_cast<CDDGView*>(m_pDDDoc->GetNextView(pos));

    const int level = pView->m_gridItem.GetLevel();
    const int record = pView->m_gridItem.GetRecord();
    const int item = pView->m_gridItem.GetItem();
    const long row = pView->m_gridItem.GetCurrentRow();
    const int vset = pView->m_gridItem.GetVSet(row);

    CDictItem* const dict_item = m_pDDDoc->GetDict()->GetLevel(level).GetRecord(record)->GetItem(item);
    DictValueSet& dict_value_set = dict_item->GetValueSet(vset);

    // sync any linked value sets
    if( dict_value_set.IsLinkedValueSet() )
        m_pDDDoc->GetDict()->SyncLinkedValueSets(&dict_value_set);
}


void DictPropertyGridValueManager::SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    AddGeneralSection<DictValue>(property_grid_ctrl);

    // Appearance heading
    auto appearance_heading_property = new PropertyGrid::HeadingProperty(L"Appearance");
    property_grid_ctrl.AddProperty(appearance_heading_property);


    // Image property (string with file dialog)
    PropertyGrid::Type::ImageFilePath initial_image_file_path
    {
        UTF8_TODO::GetCString(m_dictValue.GetImageFilePath()),
        UTF8_TODO::GetCString(m_pDDDoc->GetDict()->GetFilePath())
    };

    appearance_heading_property->AddSubItem(
        PropertyGrid::PropertyBuilder<PropertyGrid::Type::ImageFilePath>(L"Image",
                                                                         L"The image to be displayed alongside the label in data entry applications.",
                                                                         initial_image_file_path)
        .SetOnUpdate([&](const PropertyGrid::Type::ImageFilePath& image_file_path)
            {
                m_dictValue.SetImageFilePath(UTF8_TODO::GetUtf8(image_file_path.filename));
                m_pDDDoc->UpdateAllViews(nullptr);
            })
        .Create());


    // Text Color property (color with color picking dialog)
    appearance_heading_property->AddSubItem(
        PropertyGrid::PropertyBuilder<PortableColor>(L"Text Color",
                                                     L"The text color of this value label when displayed in data entry applications.",
                                                     m_dictValue.GetTextColor())
        .SetOnUpdate([&](const PortableColor& text_color)
            {
                m_dictValue.SetTextColor(text_color);
                m_pDDDoc->UpdateAllViews(nullptr);
            })
        .Create());
}
