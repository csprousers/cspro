#include "StdAfx.h"
#include "QSFEditToolbar.h"
#include "QSFEditToolBarComboBoxButton.h"
#include "QSFEditToolBarStyledComboBoxButton.h"
#include "QSFEditNumericSortToolBarComboBoxButton.h"
#include "TableToolbarButton.h"
#include <zCapiO/CapiStyle.h>


IMPLEMENT_DYNCREATE(QSFEditToolbar, CMFCToolBar)

BEGIN_MESSAGE_MAP(QSFEditToolbar, CMFCToolBar)
    ON_MESSAGE(WM_IDLEUPDATECMDUI, OnIdleUpdateCmdUI)
END_MESSAGE_MAP()


namespace {
    static CSize GetBaseUnits(CFont* pFont)
    {
        ASSERT(pFont != NULL);
        if (pFont != NULL)
        {
            ASSERT(pFont->GetSafeHandle() != NULL);
            CDC dc_screen;
            dc_screen.Attach(::GetDC(NULL));
            CFont* old_font = dc_screen.SelectObject(pFont);
            TEXTMETRIC tm;
            dc_screen.GetTextMetrics(&tm);
            dc_screen.SelectObject(old_font);
            return CSize(tm.tmAveCharWidth, tm.tmHeight);
        }

        return CSize(0, 0);
    }
}


QSFEditToolbar::QSFEditToolbar()
{
}

void QSFEditToolbar::SetStyles(const std::vector<HtmlEditorCtrl::Style>& styles)
{
    auto style_combo = DYNAMIC_DOWNCAST(QSFEditToolBarStyledComboBoxButton, GetButton(CommandToIndex(IDC_STYLE)));
    if (style_combo) {
        auto current_selection = m_styles[style_combo->GetCurSel() >= 0 ? style_combo->GetCurSel() : 0];
        m_styles = styles;
        style_combo->RemoveAllItems();
        for (const HtmlEditorCtrl::Style& style : m_styles) {
            std::optional<COLORREF> color = CssStyleParser::TextColor(style.css);
            style_combo->AddItem(style.name.c_str(), CssStyleParser::ToLogfont(style.css), color.value_or(GetSysColor(COLOR_WINDOWTEXT)));
        }
        auto new_selection = std::find_if(m_styles.begin(), m_styles.end(), [&current_selection](const HtmlEditorCtrl::Style& s) { return s.className == current_selection.className; });
        if (new_selection != m_styles.end()) {
            style_combo->SelectItem(std::distance(new_selection, m_styles.begin()), false);
        }
        else {
            style_combo->SelectItem(0, false);
        }
    }
    else {
        m_styles = styles;
    }
}

const HtmlEditorCtrl::Style& QSFEditToolbar::GetSelectedStyle() const
{
    auto button = DYNAMIC_DOWNCAST(QSFEditToolBarStyledComboBoxButton, GetButton(CommandToIndex(IDC_STYLE)));
    return m_styles[button->GetCurSel()];
}

void QSFEditToolbar::SetSelectedStyle(const HtmlEditorCtrl::Style& style)
{
    auto button = DYNAMIC_DOWNCAST(QSFEditToolBarStyledComboBoxButton, GetButton(CommandToIndex(IDC_STYLE)));
    int num_items = button->GetCount();
    for (int i = 0; i < num_items; ++i) {
        if (button->GetItem(i) == style.name) {
            if (button->GetCurSel() != i) {
                button->SelectItem(i, TRUE);
            }
            return;
        }
    }
}

void QSFEditToolbar::SetButtonVisible(UINT id, BOOL visible)
{
    const int nIndex = CommandToIndex(id);
    CMFCToolBarButton* pButton = GetButton(nIndex);
    pButton->SetVisible(visible);
    AdjustSizeImmediate();
}

// To hold the colours and their names
typedef struct {
    COLORREF crColour;
    TCHAR* szName;
} ColourTableEntry;

#define MAX_COLOURS      100

static ColourTableEntry crColours[] =
{
     { RGB(0x00, 0x00, 0x00), _T("Black")},
     { RGB(0x42, 0x42, 0x42), _T("Tundora")},
     { RGB(0x63, 0x63, 0x63), _T("Dove Gray")},
     { RGB(0x9C, 0x9C, 0x94), _T("Star Dust")},
     { RGB(0xCE, 0xC6, 0xCE), _T("Pale Slate")},
     { RGB(0xEF, 0xEF, 0xEF), _T("Gallery")},
     { RGB(0xF7, 0xF7, 0xF7), _T("Alabaster")},
     { RGB(0xFF, 0xFF, 0xFF), _T("White")},
     { RGB(0xFF, 0x00, 0x00), _T("Red")},
     { RGB(0xFF, 0x9C, 0x00), _T("Orange Peel")},
     { RGB(0xFF, 0xFF, 0x00), _T("Yellow")},
     { RGB(0x00, 0xFF, 0x00), _T("Green")},
     { RGB(0x00, 0xFF, 0xFF), _T("Cyan")},
     { RGB(0x00, 0x00, 0xFF), _T("Blue")},
     { RGB(0x9C, 0x00, 0xFF), _T("Electric Violet")},
     { RGB(0xFF, 0x00, 0xFF), _T("Magenta")},
     { RGB(0xF7, 0xC6, 0xCE), _T("Azalea")},
     { RGB(0xFF, 0xE7, 0xCE), _T("Karry")},
     { RGB(0xFF, 0xEF, 0xC6), _T("Egg White")},
     { RGB(0xD6, 0xEF, 0xD6), _T("Zanah")},
     { RGB(0xCE, 0xDE, 0xE7), _T("Botticelli")},
     { RGB(0xCE, 0xE7, 0xF7), _T("Tropical Blue")},
     { RGB(0xD6, 0xD6, 0xE7), _T("Mischka")},
     { RGB(0xE7, 0xD6, 0xDE), _T("Twilight")},
     { RGB(0xE7, 0x9C, 0x9C), _T("Tonys Pink")},
     { RGB(0xFF, 0xC6, 0x9C), _T("Peach Orange")},
     { RGB(0xFF, 0xE7, 0x9C), _T("Cream Brulee")},
     { RGB(0xB5, 0xD6, 0xA5), _T("Sprout")},
     { RGB(0xA5, 0xC6, 0xCE), _T("Casper")},
     { RGB(0x9C, 0xC6, 0xEF), _T("Perano")},
     { RGB(0xB5, 0xA5, 0xD6), _T("Cold Purple")},
     { RGB(0xD6, 0xA5, 0xBD), _T("Careys Pink")},
     { RGB(0xE7, 0x63, 0x63), _T("Mandy")},
     { RGB(0xF7, 0xAD, 0x6B), _T("Rajah")},
     { RGB(0xFF, 0xD6, 0x63), _T("Dandelion")},
     { RGB(0x94, 0xBD, 0x7B), _T("Olivine")},
     { RGB(0x73, 0xA5, 0xAD), _T("Gulf Stream")},
     { RGB(0x6B, 0xAD, 0xDE), _T("Viking")},
     { RGB(0x8C, 0x7B, 0xC6), _T("Blue Marguerite")},
     { RGB(0xC6, 0x7B, 0xA5), _T("Puce")},
     { RGB(0xCE, 0x00, 0x00), _T("Guardsman Red")},
     { RGB(0xE7, 0x94, 0x39), _T("Fire Bush")},
     { RGB(0xEF, 0xC6, 0x31), _T("Golden Dream")},
     { RGB(0x6B, 0xA5, 0x4A), _T("Chelsea Cucumber")},
     { RGB(0x4A, 0x7B, 0x8C), _T("Smalt Blue")},
     { RGB(0x39, 0x84, 0xC6), _T("Boston Blue")},
     { RGB(0x63, 0x4A, 0xA5), _T("Butterfly Bush")},
     { RGB(0xA5, 0x4A, 0x7B), _T("Cadillac")},
     { RGB(0x9C, 0x00, 0x00), _T("Sangria")},
     { RGB(0xB5, 0x63, 0x08), _T("Mai Tai")},
     { RGB(0xBD, 0x94, 0x00), _T("Buddha Gold")},
     { RGB(0x39, 0x7B, 0x21), _T("Forest Green")},
     { RGB(0x10, 0x4A, 0x5A), _T("Eden")},
     { RGB(0x08, 0x52, 0x94), _T("Venice Blue")},
     { RGB(0x31, 0x18, 0x73), _T("Meteorite")},
     { RGB(0x73, 0x18, 0x42), _T("Claret")},
     { RGB(0x63, 0x00, 0x00), _T("Rosewood")},
     { RGB(0x7B, 0x39, 0x00), _T("Cinnamon")},
     { RGB(0x84, 0x63, 0x00), _T("Olive")},
     { RGB(0x29, 0x52, 0x18), _T("Parsley")},
     { RGB(0x08, 0x31, 0x39), _T("Tiber")},
     { RGB(0x00, 0x31, 0x63), _T("Midnight Blue")},
     { RGB(0x21, 0x10, 0x4A), _T("Valentino")},
     { RGB(0x4A, 0x10, 0x31), _T("Loulou")}
};

CMFCColorMenuButton* QSFEditToolbar::CreateColorButton()
{
    if (m_palColorPicker.GetSafeHandle() == NULL)
    {
        m_nNumColours = sizeof(crColours) / sizeof(ColourTableEntry);
        ASSERT(m_nNumColours <= MAX_COLOURS);
        if (m_nNumColours > MAX_COLOURS)
            m_nNumColours = MAX_COLOURS;

        // Create the palette
        struct
        {
            LOGPALETTE    LogPalette;
            PALETTEENTRY  PalEntry[MAX_COLOURS];
        }pal;

        LOGPALETTE* pLogPalette = (LOGPALETTE*)&pal;
        pLogPalette->palVersion = 0x300;
        pLogPalette->palNumEntries = (WORD)m_nNumColours;

        for (int i = 0; i < m_nNumColours; i++)
        {
            pLogPalette->palPalEntry[i].peRed = GetRValue(crColours[i].crColour);
            pLogPalette->palPalEntry[i].peGreen = GetGValue(crColours[i].crColour);
            pLogPalette->palPalEntry[i].peBlue = GetBValue(crColours[i].crColour);
            pLogPalette->palPalEntry[i].peFlags = 0;
        }

        m_palColorPicker.CreatePalette(pLogPalette);
    }

    CMFCColorMenuButton* pColorButton = new
        CMFCColorMenuButton(ID_FORMAT_COLOR, _T("Text Color..."), &m_palColorPicker);

    pColorButton->EnableOtherButton(_T("More Colors..."));
    pColorButton->SetColumnsNumber(8);

    // Initialize color names:
    for (int i = 0; i < m_nNumColours; i++)
        CMFCColorMenuButton::SetColorName(crColours[i].crColour, crColours[i].szName);

    return pColorButton;
}

void QSFEditToolbar::SetFontFace(NullTerminatedString font_name)
{
    QSFEditToolBarStyledComboBoxButton* button = DYNAMIC_DOWNCAST(QSFEditToolBarStyledComboBoxButton, GetButton(CommandToIndex(IDC_FONTFACE)));
    int num_items = button->GetCount();
    for (int i = 0; i < num_items; ++i) {
        if (SO::Equals(button->GetItem(i), font_name)) {
            if (button->GetCurSel() != i) {
                button->SelectItem(i, TRUE);
            }
            return;
        }
    }

    // not found - add to list
    LOGFONT logfont;
    memset(&logfont, 0, sizeof(LOGFONT));
    _tcscpy(logfont.lfFaceName, font_name.c_str());
    button->AddItem(font_name.c_str(), logfont);
    button->SelectItem(num_items, TRUE);
}

CString QSFEditToolbar::GetFontFace() const
{
    auto button = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, GetButton(CommandToIndex(IDC_FONTFACE)));
    return button->GetItem(button->GetCurSel());
}

void QSFEditToolbar::SetFontSize(int font_size)
{
    ASSERT(font_size > 0);
    CString size_str;
    size_str.Format(_T("%d"), font_size);

    auto button = DYNAMIC_DOWNCAST(QSFEditNumericSortToolBarComboBoxButton, GetButton(CommandToIndex(IDC_FONTSIZE)));
    int num_items = button->GetCount();
    for (int i = 0; i < num_items; ++i) {
        if (button->GetItem(i) == size_str) {
            if (button->GetCurSel() != i) {
                button->SelectItem(i, TRUE);
            }
            return;
        }
    }

    // not found - add to list
    button->AddSortedItem(size_str);
    button->Invalidate(); // for some reason need to force redraw of combo box here or it doesn't update
}

int QSFEditToolbar::GetFontSize() const
{
    auto button = DYNAMIC_DOWNCAST(QSFEditNumericSortToolBarComboBoxButton, GetButton(CommandToIndex(IDC_FONTSIZE)));
    return _ttoi(button->GetItem(button->GetCurSel()));
}

COLORREF QSFEditToolbar::GetForeColor() const
{
    auto button = DYNAMIC_DOWNCAST(CMFCColorMenuButton, GetButton(CommandToIndex(ID_FORMAT_COLOR)));
    return button->GetColor();
}

CSize QSFEditToolbar::GetTableDimensions() const
{
    auto button = DYNAMIC_DOWNCAST(TableToolbarButton, GetButton(CommandToIndex(ID_INSERT_TABLE)));
    return button->GetDimensions();
}

void QSFEditToolbar::SetLanguages(const std::vector<Language>& languages)
{
    auto button = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, GetButton(CommandToIndex(IDC_EDIT_LANG)));
    button->RemoveAllItems();
    for (const auto& language : languages)
        button->AddItem(language.GetLabel().c_str());
}

void QSFEditToolbar::SetLanguage(const Language& language)
{
    auto button = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, GetButton(CommandToIndex(IDC_EDIT_LANG)));
    int num_items = button->GetCount();
    for (int i = 0; i < num_items; ++i) {
        if (button->GetItem(i) == language.GetLabel()) {
            if (button->GetCurSel() != i) {
                button->SelectItem(i, TRUE);
            }
            return;
        }
    }
}

CString QSFEditToolbar::GetLanguageLabel() const
{
    auto button = DYNAMIC_DOWNCAST(CMFCToolBarComboBoxButton, GetButton(CommandToIndex(IDC_EDIT_LANG)));
    return button->GetItem(button->GetCurSel());
}

BOOL QSFEditToolbar::OnUserToolTip(CMFCToolBarButton* pButton, CString& strTTText) const
{
    if (pButton->m_nID == ID_TOGGLE_QN) {
        if (pButton->m_nStyle & TBBS_CHECKED)
            strTTText = "Help Text";
        else 
            strTTText = "Question Text";
        return TRUE;
    }
    return CMFCToolBar::OnUserToolTip(pButton, strTTText);
}

int QSFEditToolbar::GetImageIndex(UINT /*command*/)
{
    return GetButton(CommandToIndex(ID_INSERT_TABLE))->GetImage();
}

void QSFEditToolbar::OnReset()
{
    CFont fnt;
    fnt.Attach(GetStockObject(DEFAULT_GUI_FONT));
    CSize base_units = GetBaseUnits(&fnt);

    QSFEditToolBarStyledComboBoxButton style_combo(IDC_STYLE, GetImageIndex(IDC_STYLE), CBS_DROPDOWN);
    for (const HtmlEditorCtrl::Style& style : m_styles) {
        std::optional<COLORREF> color = CssStyleParser::TextColor(style.css);
        style_combo.AddItem(style.name.c_str(), CssStyleParser::ToLogfont(style.css), color.value_or(GetSysColor(COLOR_WINDOWTEXT)));
    }
    ReplaceButton(IDC_STYLE, style_combo);

    QSFEditToolBarStyledComboBoxButton font_name_combo(IDC_FONTFACE, GetImageIndex(IDC_FONTFACE),
        CBS_DROPDOWNLIST, (2 * LF_FACESIZE * base_units.cx) / 2);
    LOGFONT logfont;
    memset(&logfont, 0, sizeof(LOGFONT));
    for (auto font_name : CapiStyle::DefaultFontNames) {
        _tcscpy(logfont.lfFaceName, font_name);
        font_name_combo.AddItem(font_name, logfont);
    }
    ReplaceButton(IDC_FONTFACE, font_name_combo);

    QSFEditNumericSortToolBarComboBoxButton font_size_combo(IDC_FONTSIZE, GetImageIndex(IDC_FONTSIZE),
        CBS_DROPDOWNLIST, 10 * base_units.cx + 10);
    for (auto font_size : CapiStyle::DefaultFontSizes)
        font_size_combo.AddSortedItem(IntToString(font_size));
    font_size_combo.SelectItem(0);
    ReplaceButton(IDC_FONTSIZE, font_size_combo);

    CMFCColorMenuButton* pColorButton = CreateColorButton();
    ReplaceButton(ID_FORMAT_COLOR, *pColorButton);
    delete pColorButton;

    QSFEditToolBarComboBoxButton language_combo(IDC_EDIT_LANG, GetImageIndex(IDC_EDIT_LANG));
    ReplaceButton(IDC_EDIT_LANG, language_combo);

    TableToolbarButton table_button(ID_INSERT_TABLE, GetImageIndex(ID_INSERT_TABLE), _T("Table"));
    ReplaceButton(ID_INSERT_TABLE, table_button);
}


LRESULT QSFEditToolbar::OnIdleUpdateCmdUI(WPARAM /*wParam*/, LPARAM lParam)
{
    if (IsWindowVisible())
    {
        return __super::OnIdleUpdateCmdUI(0, lParam);
    }
    return 0L;
}
