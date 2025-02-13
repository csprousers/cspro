#pragma once

#include <zHtml/HtmlEditorCtrl.h>
#include <afxtoolbar.h>

class CMFCColorMenuButton;
class CMFCToolBarFontComboBox;


class QSFEditToolbar : public CMFCToolBar
{
    DECLARE_DYNCREATE(QSFEditToolbar)

public:

    QSFEditToolbar();

    void SetStyles(const std::vector<HtmlEditorCtrl::Style>& styles);
    const HtmlEditorCtrl::Style& GetSelectedStyle() const;
    void SetSelectedStyle(const HtmlEditorCtrl::Style& style);

    void SetButtonVisible(UINT id, BOOL visible);
    CMFCColorMenuButton* CreateColorButton();

    void SetFontFace(NullTerminatedString font_name);
    CString GetFontFace() const;

    void SetFontSize(int font_size);
    int GetFontSize() const;

    COLORREF GetForeColor() const;

    CSize GetTableDimensions() const;

    void SetLanguages(const std::vector<Language>& languages);
    void SetLanguage(const Language& language);
    CString GetLanguageLabel() const;

    BOOL OnUserToolTip(CMFCToolBarButton* pButton, CString& strTTText) const override;

protected:
    CPalette m_palColorPicker;
    int	m_nNumColours;
    std::vector<HtmlEditorCtrl::Style> m_styles;

    int GetImageIndex(UINT command);

protected:
    void OnReset() override;

    // Generated message map functions
    //{{AFX_MSG(QSFEditToolbar)
    afx_msg LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
   
};
