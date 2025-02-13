#pragma once

#include <zCapiO/CapiStyle.h>
#include <zUtilF/ColorPickerComboBox.h>
#include <zUtilF/StyledComboBox.h>


class QSFEditStyleDlg : public CDialog
{
    DECLARE_DYNAMIC(QSFEditStyleDlg)

public:

    QSFEditStyleDlg(std::vector<CapiStyle> styles,
                    CWnd* pParent = NULL);

    enum { IDD = IDD_QSFSTYLEDLG };

    std::vector<CapiStyle> m_styles;

    afx_msg void OnLbnSelchangeStyle();
    afx_msg void UpdateSelectedStyle();
    afx_msg void OnStyleNameKillFocus();
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void OnAddStyle();
    afx_msg void OnDeleteStyle();

protected:
    virtual void OnOK();
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();

    DECLARE_MESSAGE_MAP()

private:
    CapiStyle& GetSelectedStyle();
    void UpdateControlsToMatchToStyle(const CapiStyle& style);
    CapiStyle StyleFromControls();
    void UpdateSample(const CapiStyle& style);

    CString GetSelectedFontName() const;
    CString GetSelectedFontStyle() const;
    std::optional<int> GetSelectedFontSize() const;
    bool IsBoldSelected() const;
    bool IsItalicSelected() const;

    CListBox m_style_list;
    StyledComboBox m_font_name_combo;
    CComboBox m_font_style_combo;
    CComboBox m_font_size_combo;
    CButton m_underline_button;
    ColorPickerComboBox m_color_combo;
    CStatic m_sample;
    CEdit m_edit_style_name;
    CMFCButton m_add_button;
    CMFCButton m_delete_button;
};

