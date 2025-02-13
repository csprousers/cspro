#pragma once

#include <zDictF/DictPropertyGridBaseManager.h>


class DictPropertyGridDictionaryManager : public DictPropertyGridBaseManager
{
public:
    DictPropertyGridDictionaryManager(CDDDoc* pDDDoc, CDataDict& dictionary);

    void SetupProperties(CMFCPropertyGridCtrl& property_grid_ctrl) override;

protected:
    void PushUndo() override;

private:
    CDataDict& m_dictionary;
};
