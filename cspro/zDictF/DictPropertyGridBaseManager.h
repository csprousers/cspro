#pragma once

#include <zDictF/Dddoc.h>
#include <zDesignerF/PropertyGrid/CustomProperties.h>
#include <zDesignerF/PropertyGrid/PropertyBuilder.h>
#include <zDesignerF/PropertyGrid/PropertyManager.h>


class DictPropertyGridBaseManager : public PropertyGrid::PropertyManager
{
protected:
    DictPropertyGridBaseManager(CDDDoc* pDDDoc, DictBase& dict_base, const CString& type_name);

public:
    virtual void SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl) = 0;

protected:
    void SetModified() override;

    void RedrawPropertyGrid();

    template<typename T>
    void AddGeneralSection(CMFCPropertyGridCtrl& property_grid_ctrl);

    template<typename T>
    CMFCPropertyGridProperty* CreateOccurrenceLabelsProperty();

private:
    template<typename T>
    CMFCPropertyGridProperty* CreateAliasesProperty();

    template<typename T>
    CMFCPropertyGridProperty* CreateNoteProperty();

protected:
    CDDDoc* m_pDDDoc;

private:
    DictBase& m_dictBase;
    CString m_typeName;
};
