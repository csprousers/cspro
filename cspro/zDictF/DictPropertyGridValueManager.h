#pragma once

#include <zDictF/DictPropertyGridBaseManager.h>


class DictPropertyGridValueManager : public DictPropertyGridBaseManager
{
public:
    DictPropertyGridValueManager(CDDDoc* pDDDoc, DictValue& dict_value);

    void SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl) override;

protected:
    void PushUndo() override;

    void SetModified() override;

private:
    DictValue& m_dictValue;
};
