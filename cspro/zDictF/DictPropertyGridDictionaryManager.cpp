#include "StdAfx.h"
#include "DictPropertyGridDictionaryManager.h"


DictPropertyGridDictionaryManager::DictPropertyGridDictionaryManager(CDDDoc* pDDDoc, CDataDict& dictionary)
    :   DictPropertyGridBaseManager(pDDDoc, dictionary, _T("Dictionary")),
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
    auto advanced_heading_property = new PropertyGrid::HeadingProperty(_T("Advanced"));
    property_grid_ctrl.AddProperty(advanced_heading_property);

    // Read Optimization property
    advanced_heading_property->AddSubItem(
        PropertyGrid::PropertyBuilder<bool>(_T("Read Optimization"),
                                            _T("If enabled, only items used in logic are read, resulting in the faster reading of data files."),
                                            m_dictionary.GetReadOptimization())
        .SetOnUpdate([&](const bool& read_optimization)
            {
                m_dictionary.SetReadOptimization(read_optimization);
            })
        .Create());
}
