#pragma once

#include <zUtilO/CommonStore.h>


class CommonStoreDlg : public CDialog
{
public:
    CommonStoreDlg(CWnd* pParent = nullptr);

protected:
    DECLARE_MESSAGE_MAP()

    BOOL OnInitDialog() override;
    void DoDataExchange(CDataExchange* pDX) override;

public:
    afx_msg void OnBnClickedRadioCommonStoreAdd();
    afx_msg void OnLvnItemchangedListCommonStore(NMHDR *pNMHDR,LRESULT *pResult);
    afx_msg void OnBnClickedButtonCommonStoreModify();
    afx_msg void OnBnClickedButtonCommonStoreDelete();
    afx_msg void OnCbnSelchangeComboCommonStoreType();

private:
    int AddSetting(const TCHAR* attribute, const TCHAR* value);
    void UpdateSelections();

private:
    CommonStore m_commonStore;
    int m_selectedItem;
    CListCtrl* m_pListCtrl;
    CComboBox* m_pComboBoxType;
    CString m_attribute;
    CString m_value;
};
