#include "StdAfx.h"
#include "DictPropertyGridBaseManager.h"
#include "OccDlg.h"


DictPropertyGridBaseManager::DictPropertyGridBaseManager(CDDDoc* const pDDDoc, DictBase& dict_base, const CString& type_name)
    :   m_pDDDoc(pDDDoc),
        m_dictBase(dict_base),
        m_typeName(type_name)
{
    ASSERT(m_pDDDoc != nullptr);
}


void DictPropertyGridBaseManager::SetModified()
{
    m_pDDDoc->SetModified(true);
}


void DictPropertyGridBaseManager::RedrawPropertyGrid()
{
    WindowsDesktopMessage::Post(UWM::Designer::RedrawPropertyGrid, &m_dictBase);
}


template<typename T>
void DictPropertyGridBaseManager::AddGeneralSection(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    // General heading
    auto general_heading_property = new PropertyGrid::HeadingProperty(L"General");
    property_grid_ctrl.AddProperty(general_heading_property);

    // Type property (read only)
    general_heading_property->AddSubItem(
        new PropertyGrid::ReadOnlyTextProperty(L"Type",
                                               nullptr,
                                               m_typeName));

    // Label property (read only)
    general_heading_property->AddSubItem(
        new PropertyGrid::ReadOnlyTextProperty(L"Label",
                                               nullptr,
                                               m_dictBase.GetLabel()));

    if constexpr(!std::is_same_v<T, DictValue>)
    {
        // Name property (read only)
        general_heading_property->AddSubItem(
            new PropertyGrid::ReadOnlyTextProperty(L"Name",
                                                   nullptr,
                                                   TC::ToWide<CString>(assert_cast<T&>(m_dictBase).GetName())));

        // Alias property (string)
        general_heading_property->AddSubItem(CreateAliasesProperty<T>());
    }


    // Note property (string with editing dialog)
    general_heading_property->AddSubItem(CreateNoteProperty<T>());
}


std::set<std::string> DictPropertyGridBaseManager::SingleStringToAliases(const std::string_view aliases_text_sv)
{
    std::set<std::string> aliases;

    // remove any trailing comma and capitalize
    std::string capitalized_aliases_text = SO::ToUpper(SO::TrimRight(SO::TrimRight(aliases_text_sv), ','));

    for( std::string alias : SO::SplitString(capitalized_aliases_text, ',') )
        aliases.insert(std::move(alias));

    return aliases;
}


template<typename T>
CMFCPropertyGridProperty* DictPropertyGridBaseManager::CreateAliasesProperty()
{
    return PropertyGrid::PropertyBuilder<std::string>(L"Aliases",
                                                      FormatText(L"Alternative names for the %s. Specify valid names, "
                                                                 L"separating names by commas if defining more than one.",
                                                                 SO::ToLower(wstring_view(m_typeName)).c_str()),
                                                      SO::CreateSingleString(assert_cast<T&>(m_dictBase).GetAliases()))
        .SetOnFormat([&](const std::string& /*aliases_text*/)
            {
                return TC::ToWide<CString>(SO::CreateSingleString(assert_cast<T&>(m_dictBase).GetAliases()));
            })
        .SetOnValidate([&](const std::string& aliases_text)
            {
                try
                {
                    const std::set<std::string> new_aliases = SingleStringToAliases(aliases_text);
                    m_pDDDoc->GetDictionaryValidator()->CheckAliases(assert_cast<T&>(m_dictBase), true, &new_aliases);
                }

                catch( const CSProException& exception )
                {
                    throw PropertyGrid::PropertyValidationException(SO::CreateSingleString(assert_cast<T&>(m_dictBase).GetAliases()),
                                                                    exception.what());
                }
            })
        .SetOnUpdate([&](const std::string& aliases_text)
            {
                assert_cast<T&>(m_dictBase).SetAliases(SingleStringToAliases(aliases_text));
                m_pDDDoc->GetDict()->BuildNameList();
            })
        .Create();
}


template<typename T>
CMFCPropertyGridProperty* DictPropertyGridBaseManager::CreateNoteProperty()
{
    return PropertyGrid::PropertyBuilder<std::string>(L"Note",
                                                      FormatText(L"A note associated with the %s.",
                                                                 SO::ToLower(wstring_view(m_typeName)).c_str()),
                                                      m_dictBase.GetNote())
        .SetOnFormat([](const std::string& note)
            {
                // show newlines as spaces
                CString note_copy = UTF8_TODO::GetCString(note);
                note_copy.Replace(L"\r\n", L" ");

                return note_copy;
            })
        .SetOnUpdate([&](const std::string& note)
            {
                m_dictBase.SetNote(note);
                m_pDDDoc->UpdateAllViews(NULL);
            })
        .SetOnButtonClick([&]() -> std::optional<std::string>
            {
                CNoteDlg note_dlg;
                note_dlg.SetTitle(FormatText(L"%s: %s (Note)", m_typeName.GetString(), m_dictBase.GetLabel().GetString()));
                note_dlg.SetNote(m_dictBase.GetNote());

                if( note_dlg.DoModal() == IDOK && m_dictBase.GetNote() != note_dlg.GetNote() )
                    return note_dlg.ReleaseNote();

                return std::nullopt;
            })
        .Create();
}


template void DictPropertyGridBaseManager::AddGeneralSection<CDataDict>(CMFCPropertyGridCtrl&);
template void DictPropertyGridBaseManager::AddGeneralSection<DictLevel>(CMFCPropertyGridCtrl&);
template void DictPropertyGridBaseManager::AddGeneralSection<CDictRecord>(CMFCPropertyGridCtrl&);
template void DictPropertyGridBaseManager::AddGeneralSection<CDictItem>(CMFCPropertyGridCtrl&);
template void DictPropertyGridBaseManager::AddGeneralSection<DictValueSet>(CMFCPropertyGridCtrl&);
template void DictPropertyGridBaseManager::AddGeneralSection<DictValue>(CMFCPropertyGridCtrl&);


namespace
{
    template<typename T>
    unsigned GetNumberOccurrenceLabels(const T& dict_element)
    {
        if constexpr(std::is_same_v<T, CDictRecord>)
            return dict_element.GetMaxRecs();

        else
            return dict_element.GetItemSubitemOccurs();
    }

    template<typename T>
    CString CreateDefinedOccurrenceLabelsString(const T& dict_element)
    {
        const auto& occurrence_labels = dict_element.GetOccurrenceLabels();
        CString defined_occurrence_labels;
        bool previous_occurrence_label_was_empty = false;

        // use an ellipsis to show that some occurrence labels are not defined
        auto process_undefined_occurrence_labels = [&]()
        {
            if( previous_occurrence_label_was_empty )
                defined_occurrence_labels.Append(defined_occurrence_labels.IsEmpty() ? L"..." : L", ...");
        };

        for( unsigned i = 0; i < GetNumberOccurrenceLabels(dict_element); ++i )
        {
            const CString& occurrence_label = occurrence_labels.GetLabel(i);

            if( occurrence_label.IsEmpty() )
            {
                previous_occurrence_label_was_empty = true;
            }

            else
            {
                process_undefined_occurrence_labels();
                previous_occurrence_label_was_empty = false;

                if( !defined_occurrence_labels.IsEmpty() )
                    defined_occurrence_labels.Append(L", ");

                defined_occurrence_labels.Append(occurrence_label);
            }
        }

        if( !defined_occurrence_labels.IsEmpty() )
            process_undefined_occurrence_labels();

        return defined_occurrence_labels;
    }
}


template<typename T>
CMFCPropertyGridProperty* DictPropertyGridBaseManager::CreateOccurrenceLabelsProperty()
{
    return PropertyGrid::PropertyBuilder<CString>(L"Occurrence Labels",
                                                  L"The occurrence labels to appear when using a roster.",
                                                  CreateDefinedOccurrenceLabelsString(assert_cast<T&>(m_dictBase)))
    .DisableDirectEdit()
    .SetOnButtonClick([&]() -> std::optional<CString>
        {
            COccDlg occurrence_dlg;
            occurrence_dlg.m_pDoc = m_pDDDoc;
            occurrence_dlg.m_sLabel = assert_cast<T&>(m_dictBase).GetLabel();

            auto& occurrence_labels = assert_cast<T&>(m_dictBase).GetOccurrenceLabels();
            unsigned occurrences = GetNumberOccurrenceLabels(assert_cast<T&>(m_dictBase));

            for( unsigned i = 0; i < occurrences; ++i )
                occurrence_dlg.m_OccGrid.m_Labels.Add(occurrence_labels.GetLabel(i));

            bool occurrence_labels_modified = false;

            if( occurrence_dlg.DoModal() == IDOK )
            {
                for( unsigned i = 0; i < occurrences; ++i )
                {
                    if( occurrence_labels_modified || occurrence_labels.GetLabel(i) != occurrence_dlg.m_OccGrid.m_Labels[i] )
                    {
                        if( !occurrence_labels_modified )
                        {
                            PushUndo();
                            SetModified();
                            occurrence_labels_modified = true;
                        }

                        occurrence_labels.SetLabel(i, occurrence_dlg.m_OccGrid.m_Labels[i]);
                    }
                }
            }

            if( occurrence_labels_modified )
                return CreateDefinedOccurrenceLabelsString(assert_cast<T&>(m_dictBase));

            else
                return std::nullopt;
        })
    .Create();
}

template CMFCPropertyGridProperty* DictPropertyGridBaseManager::CreateOccurrenceLabelsProperty<CDictRecord>();
template CMFCPropertyGridProperty* DictPropertyGridBaseManager::CreateOccurrenceLabelsProperty<CDictItem>();
