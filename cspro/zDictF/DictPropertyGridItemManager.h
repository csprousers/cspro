#pragma once

#include <zDictF/DictPropertyGridBaseManager.h>


class DictPropertyGridItemManager : public DictPropertyGridBaseManager
{
public:
    DictPropertyGridItemManager(CDDDoc* pDDDoc, CDictItem& dict_item);

    void SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl) override;

protected:
    void PushUndo() override;

private:
    void AddContentTypeSections(CMFCPropertyGridCtrl& property_grid_ctrl);
    void AddNumericContentTypeProperties(CMFCPropertyGridCtrl& property_grid_ctrl);

    void AddCaptureTypeSections(CMFCPropertyGridCtrl& property_grid_ctrl);
    void AddDateCaptureTypeProperties(CMFCPropertyGridCtrl& property_grid_ctrl);

private:
    CDictItem& m_dictItem;
};
