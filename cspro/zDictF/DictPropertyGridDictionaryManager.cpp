#include "StdAfx.h"
#include "DictPropertyGridDictionaryManager.h"


DictPropertyGridDictionaryManager::DictPropertyGridDictionaryManager(CDDDoc* const pDDDoc, CDataDict& dictionary)
    :   DictPropertyGridBaseManager(pDDDoc, dictionary, L"Dictionary"),
        m_dictionary(dictionary)
{
}


void DictPropertyGridDictionaryManager::PushUndo()
{
    m_pDDDoc->PushUndo(m_dictionary);
}


void DictPropertyGridDictionaryManager::SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl)
{
    AddGeneralSection<CDataDict>(property_grid_ctrl);

    // Advanced heading
    auto advanced_heading_property = new PropertyGrid::HeadingProperty(L"Advanced");
    property_grid_ctrl.AddProperty(advanced_heading_property);


    // Read Optimization property
    advanced_heading_property->AddSubItem(
        PropertyGrid::PropertyBuilder<bool>(
            L"Read Optimization",
            L"If enabled, only items used in logic are read, resulting in the faster reading of data files.",
            m_dictionary.GetReadOptimization()
        )
        .SetOnUpdate([&](const bool& read_optimization)
            {
                m_dictionary.SetReadOptimization(read_optimization);
            })
        .Create());


    // Syncable Name Override property
    advanced_heading_property->AddSubItem(
        PropertyGrid::PropertyBuilder<std::string>(
            L"Syncable Name Override",
            L"When defined, data will be sent to synchronization services using the provided name rather than the dictionary name.",
            m_dictionary.GetSyncableName(false)
        )
        .SetOnFormat([&](const std::string& syncable_name)
            {
                return TC::ToWide<CString>(SO::ToUpper(SO::Trim(syncable_name)));
            })
        .SetOnValidate([&](const std::string& syncable_name)
            {
                const std::string_view trimmed_syncable_name_sv = SO::Trim(syncable_name);

                if( !trimmed_syncable_name_sv.empty() && !CIMSAString::IsName(trimmed_syncable_name_sv) )
                {
                    throw PropertyGrid::PropertyValidationException(
                        std::string(),
                        "'%s' is not a valid CSPro name",
                        std::string(trimmed_syncable_name_sv).c_str()
                    );
                }
            })
        .SetOnUpdate([&](const std::string& syncable_name)
            {
                m_dictionary.SetSyncableName(SO::ToUpper(SO::Trim(syncable_name)));
            })
        .Create());
}
