#pragma once

#include <zDictF/DictPropertyGridBaseManager.h>


class DictPropertyGridLevelManager : public DictPropertyGridBaseManager
{
public:
    DictPropertyGridLevelManager(CDDDoc* pDDDoc, DictLevel& dict_level);

    void SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl) override;

protected:
    void PushUndo() override;

private:
    DictLevel& m_dictLevel;
};
