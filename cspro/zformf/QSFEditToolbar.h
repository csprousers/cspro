#pragma once

#include <zHtml/HtmlEditorCtrl.h>
#include <afxtoolbar.h>

class CMFCColorMenuButton;
class CMFCToolBarFontComboBox;


class QSFEditToolbar : public CMFCToolBar
{
public:
    void SetStyles(const std::vector<HtmlEditorCtrl::Style>& styles);
    const HtmlEditorCtrl::Style& GetSelectedStyle() const;
    void SetSelectedStyle(const HtmlEditorCtrl::Style& style);

    void SetButtonVisible(UINT id, BOOL visible);
    CMFCColorMenuButton* CreateColorButton();

    void SetFontFace(const std::wstring& font_name);
    std::string GetFontFace() const;

    void SetFontSize(int font_size);
    int GetFontSize() const;

    COLORREF GetForeColor() const;

    CSize GetTableDimensions() const;

    void SetLanguages(const std::vector<Language>& languages);
    void SetLanguage(const Language& language);
    std::string GetLanguageLabel() const;

protected:
    DECLARE_MESSAGE_MAP()

    void OnReset() override;

    LRESULT OnIdleUpdateCmdUI(WPARAM wParam, LPARAM);

private:
    static CSize GetBaseUnits(CFont* pFont);

    int GetImageIndex(UINT command);

private:
    CPalette m_palColorPicker;
    std::vector<HtmlEditorCtrl::Style> m_styles;
};
