#pragma once

#include <zDictF/DictPropertyGridBaseManager.h>


class DictPropertyGridRecordManager : public DictPropertyGridBaseManager
{
public:
    DictPropertyGridRecordManager(CDDDoc* pDDDoc, CDictRecord& dict_record);

    void SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl) override;

protected:
    void PushUndo() override;

private:
    CDictRecord& m_dictRecord;
};
