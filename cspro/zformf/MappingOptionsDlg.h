#pragma once

#include <zAppO/MappingDefines.h>


class MappingOptionsDlg : public CDialog
{
    DECLARE_DYNAMIC(MappingOptionsDlg)

public:
    MappingOptionsDlg(AppMappingOptions& mapping_options, const CDataDict& dictionary, CWnd* pParent = nullptr);

    enum { IDD = IDD_MAPPING_OPTIONS };

protected:
    DECLARE_MESSAGE_MAP()

    void DoDataExchange(CDataExchange* pDX) override;
    BOOL OnInitDialog() override;

    void OnOK() override;

    afx_msg void OnBnClickedEnableMappingCaseList();

private:
    void UpdateControlsEnabled();

private:
    AppMappingOptions& m_mappingOptions;
    const CDataDict& m_dictionary;

    BOOL m_enabled;
    CComboBox m_comboLatitude;
    CComboBox m_comboLongitude;
};
