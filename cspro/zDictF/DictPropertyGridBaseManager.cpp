#include "StdAfx.h"
#include "DictPropertyGridBaseManager.h"
#include "OccDlg.h"


DictPropertyGridBaseManager::DictPropertyGridBaseManager(CDDDoc* pDDDoc, DictBase& dict_base, const CString& type_name)
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
    AfxGetMainWnd()->PostMessage(UWM::Designer::RedrawPropertyGrid, (WPARAM)&m_dictBase);
}


template<typename T>
void DictPropertyGridBaseManager::AddGeneralSection(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    // General heading
    auto general_heading_property = new PropertyGrid::HeadingProperty(_T("General"));
    property_grid_ctrl.AddProperty(general_heading_property);

    // Type property (read only)
    general_heading_property->AddSubItem(
        new PropertyGrid::ReadOnlyTextProperty(_T("Type"),
                                               nullptr,
                                               m_typeName));

    // Label property (read only)
    general_heading_property->AddSubItem(
        new PropertyGrid::ReadOnlyTextProperty(_T("Label"),
                                               nullptr,
                                               m_dictBase.GetLabel()));

    if constexpr(!std::is_same_v<T, DictValue>)
    {
        // Name property (read only)
        general_heading_property->AddSubItem(
            new PropertyGrid::ReadOnlyTextProperty(_T("Name"),
                                                   nullptr,
                                                   assert_cast<T&>(m_dictBase).GetName()));

        // Alias property (string)
        general_heading_property->AddSubItem(CreateAliasesProperty<T>());
    }


    // Note property (string with editing dialog)
    general_heading_property->AddSubItem(CreateNoteProperty<T>());
}


namespace
{
    std::set<CString> SingleStringToAliases(wstring_view aliases_text)
    {
        std::set<CString> aliases;

        // remove any trailing comma and capitalize 
        std::wstring capitalized_aliases_text = SO::ToUpper(SO::TrimRight(SO::TrimRight(aliases_text), ','));

        for( wstring_view alias : SO::SplitString(capitalized_aliases_text, ',') )
            aliases.insert(alias);

        return aliases;
    }
}

template<typename T>
CMFCPropertyGridProperty* DictPropertyGridBaseManager::CreateAliasesProperty()
{
    return PropertyGrid::PropertyBuilder<CString>(_T("Aliases"),
                                                  FormatText(_T("Alternative names for the %s. Specify valid names, ")
                                                             _T("separating names by commas if defining more than one."),
                                                             SO::ToLower(m_typeName).c_str()),
                                                  WS2CS(SO::CreateSingleString(assert_cast<T&>(m_dictBase).GetAliases())))
        .SetOnFormat([&](const CString& /*aliases_text*/)
            {
                return WS2CS(SO::CreateSingleString(assert_cast<T&>(m_dictBase).GetAliases()));
            })
        .SetOnValidate([&](const CString& aliases_text)
            {
                try
                {
                    std::set<CString> new_aliases = SingleStringToAliases(aliases_text);
                    m_pDDDoc->GetDictionaryValidator()->CheckAliases(assert_cast<T&>(m_dictBase), true, &new_aliases);
                }

                catch( const CSProException& exception )
                {
                    throw PropertyGrid::PropertyValidationException<CString>(
                        WS2CS(SO::CreateSingleString(assert_cast<T&>(m_dictBase).GetAliases())),
                        WS2CS(exception.GetErrorMessage()));
                }
            })
        .SetOnUpdate([&](const CString& aliases_text)
            {
                assert_cast<T&>(m_dictBase).SetAliases(SingleStringToAliases(aliases_text));
                m_pDDDoc->GetDict()->BuildNameList();
            })
        .Create();
}


template<typename T>
CMFCPropertyGridProperty* DictPropertyGridBaseManager::CreateNoteProperty()
{
    return PropertyGrid::PropertyBuilder<CString>(_T("Note"),
                                                  FormatText(_T("A note associated with the %s."),
                                                             SO::ToLower(m_typeName).c_str()),
                                                  m_dictBase.GetNote())
        .SetOnFormat([](const CString& note)
            {
                // show newlines as spaces
                CString note_copy = note;
                note_copy.Replace(_T("\r\n"), _T(" "));

                return note_copy;
            })
        .SetOnUpdate([&](const CString& note)
            {
                m_dictBase.SetNote(note);
                m_pDDDoc->UpdateAllViews(NULL);
            })
        .SetOnButtonClick([&]() -> std::optional<CString>
            {
                CNoteDlg note_dlg;
                note_dlg.SetTitle(FormatText(_T("%s: %s (Note)"), (LPCTSTR)m_typeName, (LPCTSTR)m_dictBase.GetLabel()));
                note_dlg.SetNote(m_dictBase.GetNote());

                if( note_dlg.DoModal() == IDOK && m_dictBase.GetNote() != note_dlg.GetNote() )
                    return note_dlg.GetNote();

                else
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
                defined_occurrence_labels.Append(defined_occurrence_labels.IsEmpty() ? _T("...") : _T(", ..."));
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
                    defined_occurrence_labels.Append(_T(", "));

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
    return PropertyGrid::PropertyBuilder<CString>(_T("Occurrence Labels"),
                                                  _T("The occurrence labels to appear when using a roster."),
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
