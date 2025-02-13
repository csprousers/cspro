#pragma once

#include <zDictF/DictPropertyGridBaseManager.h>


class DictPropertyGridValueSetManager : public DictPropertyGridBaseManager
{
public:
    DictPropertyGridValueSetManager(CDDDoc* pDDDoc, DictValueSet& dict_value_set);

    void SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl) override;

protected:
    void PushUndo() override;

private:
    DictValueSet& m_dictValueSet;
};
