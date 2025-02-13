#include "StdAfx.h"
#include "DictPropertyGridItemManager.h"
#include <zDesignerF/PropertyGrid/ListPropertyBuilder.h>


DictPropertyGridItemManager::DictPropertyGridItemManager(CDDDoc* pDDDoc, CDictItem& dict_item)
    :   DictPropertyGridBaseManager(pDDDoc, dict_item, ( dict_item.GetItemType() == ItemType::Subitem ) ? _T("Subitem") : _T("Item")),
        m_dictItem(dict_item)
{
}


void DictPropertyGridItemManager::PushUndo()
{
    // push the entire dictionary (only pushing the item led to problems with ID items)
    m_pDDDoc->PushUndo(*m_pDDDoc->GetDict(), m_pDDDoc->GetLevel(), m_pDDDoc->GetRec(), m_pDDDoc->GetItem());
}


void DictPropertyGridItemManager::SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    AddGeneralSection<CDictItem>(property_grid_ctrl);

    // Item heading
    auto item_heading_property = new PropertyGrid::HeadingProperty(_T("Item"));
    property_grid_ctrl.AddProperty(item_heading_property);


    // Default Capture Type (list)
    PropertyGrid::ListPropertyBuilder<CaptureType> capture_type_list_property_builder(
        _T("Default Capture Type"),
        _T("The default capture type used to collect data in a data entry application."),
        m_dictItem.GetCaptureInfo().GetCaptureType());

    capture_type_list_property_builder.AddOption(CaptureType::Unspecified, _T(""));

    for( CaptureType capture_type : CaptureInfo::GetPossibleCaptureTypes(m_dictItem, CaptureInfo::CaptureTypeSortOrder::Name) )
        capture_type_list_property_builder.AddOption(capture_type, CaptureInfo::GetCaptureTypeName(capture_type, true));
    
    capture_type_list_property_builder.SetOnUpdate([&](CaptureType capture_type)
        {
            m_dictItem.GetCaptureInfo().SetCaptureType(capture_type);

            // redraw the property grid given that there may be sections specific to
            // the capture type that must be displayed
            RedrawPropertyGrid();
        });

    item_heading_property->AddSubItem(capture_type_list_property_builder.Create());


    // Occurrence Labels (read only string with editing dialog)
    if( m_dictItem.GetOccurs() > 1 )
        item_heading_property->AddSubItem(CreateOccurrenceLabelsProperty<CDictItem>());


    // add type-specific sections
    AddContentTypeSections(property_grid_ctrl);
    AddCaptureTypeSections(property_grid_ctrl);
}



void DictPropertyGridItemManager::AddContentTypeSections(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    switch( m_dictItem.GetContentType() )
    {
        case ContentType::Numeric:
            AddNumericContentTypeProperties(property_grid_ctrl);
            break;
    }
}


void DictPropertyGridItemManager::AddNumericContentTypeProperties(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    // Numeric Item heading
    auto numeric_heading_property = new PropertyGrid::HeadingProperty(_T("Numeric Item"));
    property_grid_ctrl.AddProperty(numeric_heading_property);

    // Zero Fill property (bool)
    numeric_heading_property->AddSubItem(
        PropertyGrid::PropertyBuilder<bool>(_T("Zero Fill"),
                                            _T("Pads numerical values with left-justified zeros."),
                                            m_dictItem.GetZeroFill())
        .SetOnUpdate([&](const bool& value)
            {
                m_dictItem.SetZeroFill(value);
            })
        .Create());
}



void DictPropertyGridItemManager::AddCaptureTypeSections(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    switch( m_dictItem.GetCaptureInfo().GetCaptureType() )
    {
        case CaptureType::Date:
            AddDateCaptureTypeProperties(property_grid_ctrl);
            break;
    }
}


void DictPropertyGridItemManager::AddDateCaptureTypeProperties(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    // Date Capture Type heading
    auto date_heading_property = new PropertyGrid::HeadingProperty(_T("Date Capture Type"));
    property_grid_ctrl.AddProperty(date_heading_property);

    // Format (list)
    PropertyGrid::ListPropertyBuilder<CString> format_list_property_builder(
        _T("Format"),
        _T("The format used to store a date while collecting data in a data entry application."));

    std::vector<const TCHAR*> formats = DateCaptureInfo::GetPossibleFormats(m_dictItem);
    const CString& current_format = m_dictItem.GetCaptureInfo().GetExtended<DateCaptureInfo>().GetFormat();

    // select the format, or if it is invalid (or unset), select a blank string
    if( std::find_if(formats.cbegin(), formats.cend(),
        [&](const TCHAR* format) { return ( current_format == format ); }) != formats.cend() )
    {
        format_list_property_builder.SetValue(current_format);
    }

    else
    {
        format_list_property_builder.SetValue(CString());
    }

    format_list_property_builder.AddOption(CString(), _T(""));

    for( const TCHAR* format : formats )
        format_list_property_builder.AddOption(format, format);
    
    format_list_property_builder.SetOnUpdate([&](const CString& format)
        {
            m_dictItem.GetCaptureInfo().GetExtended<DateCaptureInfo>().SetFormat(format);
        });

    date_heading_property->AddSubItem(format_list_property_builder.Create());
}
