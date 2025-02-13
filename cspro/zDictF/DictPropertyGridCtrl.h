#pragma once

#include <zDictF/zDictF.h>
#include <zDictF/Dddoc.h>
#include <afxpropertygridctrl.h>

class DictPropertyGridBaseManager;


class CLASS_DECL_ZDICTF CDictPropertyGridCtrl : public CMFCPropertyGridCtrl
{
    DECLARE_DYNAMIC(CDictPropertyGridCtrl)

public:
    CDictPropertyGridCtrl();
    ~CDictPropertyGridCtrl();

    void Initialize(CDDDoc* pDDDoc, DictBase* dict_base);

protected:
    void OnPropertyChanged(CMFCPropertyGridProperty* pProp) const override;
    void OnClickButton(CPoint point) override;

protected:
    mutable bool m_updateControl;

private:
    std::unique_ptr<DictPropertyGridBaseManager> m_propertyManager;
};
