#include "Stdafx.h"
#include "DataViewerSettings.h"


namespace
{
    const int MaxNumberRecentFilesList = 10;

    System::String^ GetSettingsFilename()
    {
        auto directory = System::IO::Path::Combine(System::Environment::GetFolderPath(System::Environment::SpecialFolder::ApplicationData),
            "CSPro", "DataViewer");

        System::IO::Directory::CreateDirectory(directory);

        return System::IO::Path::Combine(directory, "Settings.dat");
    }
}


CSPro::Data::DataViewerSettings::DataViewerSettings()
    :   m_windowState(System::Windows::Forms::FormWindowState::Normal),
        m_caseIterationMethod(CaseIterationMethod::KeyOrder),
        m_caseIterationOrder(CaseIterationOrder::Ascending),
        m_caseIterationCaseStatus(CaseIterationCaseStatus::NotDeletedOnly),
        m_keyPrefix(nullptr),
        m_showCaseLabels(true),
        m_showKeyPrefixPanel(false),
        m_statuses(CaseToHtmlConverter::Statuses::ShowIfNotDefault),
        m_caseConstructionErrors(CaseToHtmlConverter::CaseConstructionErrors::Show),
        m_nameDisplay(CaseToHtmlConverter::NameDisplay::Label),
        m_recordOrientation(CaseToHtmlConverter::RecordOrientation::Horizontal),
        m_occurrenceDisplay(CaseToHtmlConverter::OccurrenceDisplay::Label),
        m_itemTypeDisplay(CaseToHtmlConverter::ItemTypeDisplay::ItemSubitem),
        m_blankValues(CaseToHtmlConverter::BlankValues::Show),
        m_caseItemPrinterFormat(CaseItemPrinter::Format::CaseTree),
        m_languageName(nullptr),
        m_exportOneFilePerRecord(false)
{
}


CSPro::Data::DataViewerSettings^ CSPro::Data::DataViewerSettings::Load()
{
    System::IO::FileStream^ file_stream = nullptr;

    try
    {
        file_stream = gcnew System::IO::FileStream(GetSettingsFilename(), System::IO::FileMode::Open, System::IO::FileAccess::Read);
        auto binary_formatter = gcnew System::Runtime::Serialization::Formatters::Binary::BinaryFormatter();
        return (DataViewerSettings^)binary_formatter->Deserialize(file_stream);
    }

    catch(...)
    {
        return gcnew DataViewerSettings();
    }

    finally
    {
        delete file_stream;
    }
}


CSPro::Data::DataViewerSettings::!DataViewerSettings()
{
    // save the settings
    System::IO::FileStream^ file_stream = nullptr;

    try
    {
        file_stream = gcnew System::IO::FileStream(GetSettingsFilename(), System::IO::FileMode::Create, System::IO::FileAccess::Write);
        auto binary_formatter = gcnew System::Runtime::Serialization::Formatters::Binary::BinaryFormatter();
        binary_formatter->Serialize(file_stream, this);
    }

    catch(...) { }

    finally
    {
        delete file_stream;
    }
}


void CSPro::Data::DataViewerSettings::ApplyCaseToHtmlConverterSettings(CaseToHtmlConverter& case_to_html_converter)
{
    case_to_html_converter.SetStatuses(m_statuses);
    case_to_html_converter.SetCaseConstructionErrors(m_caseConstructionErrors);
    case_to_html_converter.SetNameDisplay(m_nameDisplay);
    case_to_html_converter.SetRecordOrientation(m_recordOrientation);
    case_to_html_converter.SetOccurrenceDisplay(m_occurrenceDisplay);
    case_to_html_converter.SetItemTypeDisplay(m_itemTypeDisplay);
    case_to_html_converter.SetBlankValues(m_blankValues);
    case_to_html_converter.SetCaseItemPrinterFormat(m_caseItemPrinterFormat);
    case_to_html_converter.SetLanguage(ToWS(m_languageName));
}


namespace
{
    enum class MenuItemType
    {
        CaseIterationMethod,
        CaseIterationOrder,
        CaseIterationCaseStatus,
        CaseToHtmlConverterStatuses,
        CaseToHtmlConverterCaseConstructionErrors,
        CaseToHtmlConverterNameDisplay,
        CaseToHtmlConverterRecordOrientation,
        CaseToHtmlConverterOccurrenceDisplay,
        CaseToHtmlConverterItemTypeDisplay,
        CaseToHtmlConverterBlankValues,
        CaseItemPrinterFormat
    };

    struct MenuItemLink
    {
        MenuItemType Type;
        const TCHAR* MenuItemName;
        int EnumValue;
    };

    static const MenuItemLink MenuItemLinks[]
    {
        MenuItemType::CaseIterationMethod, L"menuItemListingIndexed",    (int)CaseIterationMethod::KeyOrder,
        MenuItemType::CaseIterationMethod, L"menuItemListingSequential", (int)CaseIterationMethod::SequentialOrder,

        MenuItemType::CaseIterationOrder, L"menuItemListingAscending",  (int)CaseIterationOrder::Ascending,
        MenuItemType::CaseIterationOrder, L"menuItemListingDescending", (int)CaseIterationOrder::Descending,

        MenuItemType::CaseIterationCaseStatus, L"menuItemListingAll",        (int)CaseIterationCaseStatus::All,
        MenuItemType::CaseIterationCaseStatus, L"menuItemListingNotDeleted", (int)CaseIterationCaseStatus::NotDeletedOnly,
        MenuItemType::CaseIterationCaseStatus, L"menuItemListingPartial",    (int)CaseIterationCaseStatus::PartialsOnly,
        MenuItemType::CaseIterationCaseStatus, L"menuItemListingDuplicate",  (int)CaseIterationCaseStatus::DuplicatesOnly,

        MenuItemType::CaseToHtmlConverterStatuses, L"menuItemCaseCaseDetailsAll",     (int)CaseToHtmlConverter::Statuses::Show,
        MenuItemType::CaseToHtmlConverterStatuses, L"menuItemCaseCaseDetailsDefault", (int)CaseToHtmlConverter::Statuses::ShowIfNotDefault,

        MenuItemType::CaseToHtmlConverterCaseConstructionErrors, L"menuItemCaseCaseShowCaseErrors", (int)CaseToHtmlConverter::CaseToHtmlConverter::CaseConstructionErrors::Show,
        MenuItemType::CaseToHtmlConverterCaseConstructionErrors, L"menuItemCaseCaseHideCaseErrors", (int)CaseToHtmlConverter::CaseToHtmlConverter::CaseConstructionErrors::Hide,

        MenuItemType::CaseToHtmlConverterNameDisplay, L"menuItemCaseDictionaryDisplayLabels",      (int)CaseToHtmlConverter::CaseToHtmlConverter::NameDisplay::Label,
        MenuItemType::CaseToHtmlConverterNameDisplay, L"menuItemCaseDictionaryDisplayNames",       (int)CaseToHtmlConverter::CaseToHtmlConverter::NameDisplay::Name,
        MenuItemType::CaseToHtmlConverterNameDisplay, L"menuItemCaseDictionaryDisplayNamesLabels", (int)CaseToHtmlConverter::CaseToHtmlConverter::NameDisplay::NameLabel,

        MenuItemType::CaseToHtmlConverterRecordOrientation, L"menuItemCaseRecordDisplayHorizonal", (int)CaseToHtmlConverter::CaseToHtmlConverter::RecordOrientation::Horizontal,
        MenuItemType::CaseToHtmlConverterRecordOrientation, L"menuItemCaseRecordDisplayVertical",  (int)CaseToHtmlConverter::CaseToHtmlConverter::RecordOrientation::Vertical,

        MenuItemType::CaseToHtmlConverterOccurrenceDisplay, L"menuItemCaseRecordDisplayOccurrenceNumbers", (int)CaseToHtmlConverter::CaseToHtmlConverter::OccurrenceDisplay::Number,
        MenuItemType::CaseToHtmlConverterOccurrenceDisplay, L"menuItemCaseRecordDisplayOccurrenceLabels",  (int)CaseToHtmlConverter::CaseToHtmlConverter::OccurrenceDisplay::Label,

        MenuItemType::CaseToHtmlConverterItemTypeDisplay, L"menuItemCaseDictionaryDisplayItems",         (int)CaseToHtmlConverter::CaseToHtmlConverter::ItemTypeDisplay::Item,
        MenuItemType::CaseToHtmlConverterItemTypeDisplay, L"menuItemCaseDictionaryDisplaySubitems",      (int)CaseToHtmlConverter::CaseToHtmlConverter::ItemTypeDisplay::Subitem,
        MenuItemType::CaseToHtmlConverterItemTypeDisplay, L"menuItemCaseDictionaryDisplayItemsSubitems", (int)CaseToHtmlConverter::CaseToHtmlConverter::ItemTypeDisplay::ItemSubitem,

        MenuItemType::CaseToHtmlConverterBlankValues, L"menuItemCaseValueDisplayShowBlanks", (int)CaseToHtmlConverter::CaseToHtmlConverter::BlankValues::Show,
        MenuItemType::CaseToHtmlConverterBlankValues, L"menuItemCaseValueDisplayHideBlanks", (int)CaseToHtmlConverter::CaseToHtmlConverter::BlankValues::Hide,

        MenuItemType::CaseItemPrinterFormat, L"menuItemCaseValueDisplayCaseTree",    (int)CaseItemPrinter::Format::CaseTree,
        MenuItemType::CaseItemPrinterFormat, L"menuItemCaseValueDisplayLabels",      (int)CaseItemPrinter::Format::Label,
        MenuItemType::CaseItemPrinterFormat, L"menuItemCaseValueDisplayCodes",       (int)CaseItemPrinter::Format::Code,
        MenuItemType::CaseItemPrinterFormat, L"menuItemCaseValueDisplayLabelsCodes", (int)CaseItemPrinter::Format::LabelCode,
        MenuItemType::CaseItemPrinterFormat, L"menuItemCaseValueDisplayCodesLabels", (int)CaseItemPrinter::Format::CodeLabel,
    };

    const MenuItemLink* GetMenuItemLink(System::Windows::Forms::ToolStripMenuItem^ menu_item)
    {
        CString menu_item_name(menu_item->Name);

        for( const MenuItemLink* menu_item_link = MenuItemLinks + _countof(MenuItemLinks) - 1; menu_item_link >= MenuItemLinks; --menu_item_link )
        {
            if( menu_item_name.Compare(menu_item_link->MenuItemName) == 0 )
                return menu_item_link;
        }

        return nullptr;
    }

    template<typename T>
    T ChangeOrCycleSetting(const MenuItemLink* menu_item_link, bool called_via_accelerator, T current_value)
    {
        if( called_via_accelerator )
        {
            // if cycling, find the first value of this type
            const MenuItemLink* first_menu_item_link_of_type = menu_item_link;
            const MenuItemLink* last_menu_item_link_of_all_types = MenuItemLinks + _countof(MenuItemLinks) - 1;

            while( first_menu_item_link_of_type > MenuItemLinks && ( first_menu_item_link_of_type - 1 )->Type == menu_item_link->Type )
                --first_menu_item_link_of_type;

            // now find the current value
            menu_item_link = first_menu_item_link_of_type;

            while( menu_item_link->EnumValue != (int)current_value )
            {
                ++menu_item_link;

                if( menu_item_link > last_menu_item_link_of_all_types )
                {
                    // in case the settings values are corrupt
                    ASSERT(false);
                    menu_item_link = first_menu_item_link_of_type;
                    break;
                }
            }

            // if at the end of the values of this type, use the first value
            ++menu_item_link;

            if( menu_item_link > last_menu_item_link_of_all_types || menu_item_link->Type != first_menu_item_link_of_type->Type )
                menu_item_link = first_menu_item_link_of_type;
        }

        return (T)menu_item_link->EnumValue;
    }
}


void CSPro::Data::DataViewerSettings::ChangeSetting(System::Windows::Forms::ToolStripMenuItem^ menu_item, bool called_via_accelerator)
{
    const MenuItemLink* menu_item_link = GetMenuItemLink(menu_item);

    if( menu_item_link == nullptr )
        return;

    if( menu_item_link->Type == MenuItemType::CaseIterationMethod )
        m_caseIterationMethod = ChangeOrCycleSetting<CaseIterationMethod>(menu_item_link, called_via_accelerator, m_caseIterationMethod);

    else if( menu_item_link->Type == MenuItemType::CaseIterationOrder )
        m_caseIterationOrder = (CaseIterationOrder)menu_item_link->EnumValue;

    else if( menu_item_link->Type == MenuItemType::CaseIterationCaseStatus )
        m_caseIterationCaseStatus = (CaseIterationCaseStatus)menu_item_link->EnumValue;

    else if( menu_item_link->Type == MenuItemType::CaseToHtmlConverterStatuses )
        m_statuses = (CaseToHtmlConverter::Statuses)menu_item_link->EnumValue;

    else if( menu_item_link->Type == MenuItemType::CaseToHtmlConverterCaseConstructionErrors )
        m_caseConstructionErrors = (CaseToHtmlConverter::CaseConstructionErrors)menu_item_link->EnumValue;

    else if( menu_item_link->Type == MenuItemType::CaseToHtmlConverterNameDisplay )
        m_nameDisplay = ChangeOrCycleSetting<CaseToHtmlConverter::NameDisplay>(menu_item_link, called_via_accelerator, m_nameDisplay);

    else if( menu_item_link->Type == MenuItemType::CaseToHtmlConverterRecordOrientation )
        m_recordOrientation = (CaseToHtmlConverter::RecordOrientation)menu_item_link->EnumValue;

    else if( menu_item_link->Type == MenuItemType::CaseToHtmlConverterOccurrenceDisplay )
        m_occurrenceDisplay = (CaseToHtmlConverter::OccurrenceDisplay)menu_item_link->EnumValue;

    else if( menu_item_link->Type == MenuItemType::CaseToHtmlConverterItemTypeDisplay )
        m_itemTypeDisplay = (CaseToHtmlConverter::ItemTypeDisplay)menu_item_link->EnumValue;

    else if( menu_item_link->Type == MenuItemType::CaseToHtmlConverterBlankValues )
        m_blankValues = (CaseToHtmlConverter::BlankValues)menu_item_link->EnumValue;

    else if( menu_item_link->Type == MenuItemType::CaseItemPrinterFormat )
        m_caseItemPrinterFormat = ChangeOrCycleSetting<CaseItemPrinter::Format>(menu_item_link, called_via_accelerator, m_caseItemPrinterFormat);
}


void CSPro::Data::DataViewerSettings::UpdateMenuChecks(System::Collections::Generic::List<System::Windows::Forms::ToolStripMenuItem^>^ menu_items)
{
    for( int i = menu_items->Count - 1; i >= 0; --i )
    {
        const MenuItemLink* menu_item_link = GetMenuItemLink(menu_items[i]);

        // remove non-setting menu items
        if( menu_item_link == nullptr )
            menu_items->RemoveAt(i);

        else
        {
            int enum_value =
                ( menu_item_link->Type == MenuItemType::CaseIterationMethod )                       ? (int)m_caseIterationMethod :
                ( menu_item_link->Type == MenuItemType::CaseIterationOrder )                        ? (int)m_caseIterationOrder :
                ( menu_item_link->Type == MenuItemType::CaseIterationCaseStatus )                   ? (int)m_caseIterationCaseStatus :
                ( menu_item_link->Type == MenuItemType::CaseToHtmlConverterStatuses )               ? (int)m_statuses :
                ( menu_item_link->Type == MenuItemType::CaseToHtmlConverterCaseConstructionErrors ) ? (int)m_caseConstructionErrors :
                ( menu_item_link->Type == MenuItemType::CaseToHtmlConverterNameDisplay )            ? (int)m_nameDisplay :
                ( menu_item_link->Type == MenuItemType::CaseToHtmlConverterRecordOrientation )      ? (int)m_recordOrientation :
                ( menu_item_link->Type == MenuItemType::CaseToHtmlConverterOccurrenceDisplay )      ? (int)m_occurrenceDisplay :
                ( menu_item_link->Type == MenuItemType::CaseToHtmlConverterItemTypeDisplay )        ? (int)m_itemTypeDisplay :
                ( menu_item_link->Type == MenuItemType::CaseToHtmlConverterBlankValues )            ? (int)m_blankValues :
                ( menu_item_link->Type == MenuItemType::CaseItemPrinterFormat )                     ? (int)m_caseItemPrinterFormat :
                                                                                                      INT_MAX;

            menu_items[i]->Checked = ( enum_value == menu_item_link->EnumValue );
        }
    }
}


System::Collections::Generic::List<System::String^>^ CSPro::Data::DataViewerSettings::RecentFiles::get()
{
    if( m_recentFilesList == nullptr )
        m_recentFilesList = gcnew System::Collections::Generic::List<System::String^>;

    return m_recentFilesList;
}

void CSPro::Data::DataViewerSettings::AddToRecentFilesList(System::String^ filename)
{
    if( !System::String::IsNullOrEmpty(filename) )
    {
        // make sure that the most recent file is at the top of the list
        int index = RecentFiles->IndexOf(filename);

        if( index != 0 )
        {
            if( index > 0 )
                RecentFiles->RemoveAt(index);

            RecentFiles->Insert(0, filename);

            while( RecentFiles->Count > MaxNumberRecentFilesList )
                RecentFiles->RemoveAt(RecentFiles->Count - 1);
        }
    }
}


System::Collections::Generic::List<System::String^>^ CSPro::Data::DataViewerSettings::ExportFormatSelections::get()
{
    if( m_exportFormatSelections == nullptr )
        m_exportFormatSelections = gcnew System::Collections::Generic::List<System::String^>;

    return m_exportFormatSelections;
}
